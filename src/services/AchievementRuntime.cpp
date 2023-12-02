#include "AchievementRuntime.hh"

#include "Exports.hh"
#include "RA_Defs.h"
#include "RA_Log.h"
#include "RA_StringUtils.h"

#include "RA_md5factory.h"

#include "data\context\EmulatorContext.hh"
#include "data\context\GameContext.hh"
#include "data\context\SessionTracker.hh"
#include "data\context\UserContext.hh"

#include "services\FrameEventQueue.hh"
#include "services\Http.hh"
#include "services\IAudioSystem.hh"
#include "services\IConfiguration.hh"
#include "services\IFileSystem.hh"
#include "services\IHttpRequester.hh"
#include "services\ILocalStorage.hh"
#include "services\ServiceLocator.hh"
#include "services\impl\JsonFileConfiguration.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\OverlayManager.hh"
#include "ui\viewmodels\PopupMessageViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

#include <rcheevos\include\rc_api_runtime.h>
#include <rcheevos\src\rapi\rc_api_common.h> // for parsing cached patchdata response
#include <rcheevos\src\rcheevos\rc_internal.h>
#include <rcheevos\src\rc_client_internal.h>
#include <rcheevos\src\rhash\md5.h>

namespace ra {
namespace services {

static std::map<uint32_t, int> s_mAchievementPopups;

static int CanSubmit()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    if (pGameContext.GetMode() == ra::data::context::GameContext::Mode::CompatibilityTest)
        return 0;

    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (pConfiguration.IsFeatureEnabled(ra::services::Feature::Offline))
        return 0;

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    if (pEmulatorContext.WasMemoryModified())
        return 0;

    if (pEmulatorContext.IsMemoryInsecure())
        return 0;

    return 1;
}

static int CanSubmitAchievementUnlock(uint32_t nAchievementId, rc_client_t*)
{
    if (!CanSubmit())
        return 0;

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto* vmAchievement = pGameContext.Assets().FindAchievement(nAchievementId);
    if (vmAchievement == nullptr ||
        vmAchievement->GetChanges() != ra::data::models::AssetChanges::None ||
        vmAchievement->GetCategory() != ra::data::models::AssetCategory::Core)
    {
        return 0;
    }

    return 1;
}

static int CanSubmitLeaderboardEntry(uint32_t nLeaderboardId, rc_client_t*)
{
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (!pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
        return false;

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto* pLeaderboard = pGameContext.Assets().FindLeaderboard(nLeaderboardId);
    if (pLeaderboard == nullptr ||
        pLeaderboard->GetChanges() != ra::data::models::AssetChanges::None ||
        pLeaderboard->GetCategory() != ra::data::models::AssetCategory::Core)
    {
        return 0;
    }

    return CanSubmit();
}

static int RichPresenceOverride(rc_client_t*, char buffer[], size_t buffer_size)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();

    const auto& pWindowManager = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>();
    const bool bIsInspectingMemory = pWindowManager.MemoryBookmarks.IsVisible() ||
                                     pWindowManager.MemoryInspector.IsVisible() ||
                                     pWindowManager.AssetEditor.IsVisible();

    if (bIsInspectingMemory)
    {
        if (pGameContext.GetMode() == ra::data::context::GameContext::Mode::CompatibilityTest)
            return snprintf(buffer, buffer_size, "Testing Compatibility");

        if (_RA_HardcoreModeIsActive())
            return snprintf(buffer, buffer_size, "Inspecting Memory in Hardcore mode");

        const char* sMessage = "Inspecting Memory";
        for (gsl::index i = 0; i < gsl::narrow_cast<gsl::index>(pGameContext.Assets().Count()); ++i)
        {
            const auto* pAsset = pGameContext.Assets().GetItemAt(i);
            if (pAsset)
            {
                if (!pAsset->IsShownInList())
                    continue;

                if (pAsset->GetCategory() == ra::data::models::AssetCategory::Local)
                {
                    sMessage = "Developing Achievements";
                    break;
                }

                if (pAsset->GetChanges() != ra::data::models::AssetChanges::None)
                    sMessage = "Fixing Achievements";
            }
        }

        return snprintf(buffer, buffer_size, "%s", sMessage);
    }

    // don't send text from modified rich presence to server
    const auto* pRichPresence = pGameContext.Assets().FindRichPresence();
    if (pRichPresence && pRichPresence->GetChanges() != ra::data::models::AssetChanges::None)
        return snprintf(buffer, buffer_size, "Playing %s", ra::Narrow(pGameContext.GameTitle()).c_str());

    // allow default behavior
    return 0;
}

AchievementRuntime::AchievementRuntime()
{
    m_pClient.reset(rc_client_create(AchievementRuntime::ReadMemory, AchievementRuntime::ServerCallAsync));

    m_pClient->callbacks.can_submit_achievement_unlock = CanSubmitAchievementUnlock;
    m_pClient->callbacks.can_submit_leaderboard_entry = CanSubmitLeaderboardEntry;
    m_pClient->callbacks.rich_presence_override = RichPresenceOverride;

#ifndef RA_UTEST
    rc_client_enable_logging(m_pClient.get(), RC_CLIENT_LOG_LEVEL_VERBOSE, AchievementRuntime::LogMessage);

    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    const auto bHardcore = pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore);
    rc_client_set_hardcore_enabled(m_pClient.get(), bHardcore ? 1 : 0);
#endif

    rc_client_set_event_handler(m_pClient.get(), EventHandler);

    rc_client_set_unofficial_enabled(m_pClient.get(), 1);
}

AchievementRuntime::~AchievementRuntime() { Shutdown(); }

static void DummyEventHandler(const rc_client_event_t*, rc_client_t*) noexcept
{
}

void AchievementRuntime::Shutdown() noexcept
{
    if (m_pClient != nullptr)
    {
        // don't need to handle events while shutting down
        rc_client_set_event_handler(m_pClient.get(), DummyEventHandler);

        rc_client_destroy(m_pClient.release());
    }
}

void AchievementRuntime::LogMessage(const char* sMessage, const rc_client_t*)
{
    const auto& pLogger = ra::services::ServiceLocator::Get<ra::services::ILogger>();
    if (pLogger.IsEnabled(ra::services::LogLevel::Info))
        pLogger.LogMessage(ra::services::LogLevel::Info, sMessage);
}

uint32_t AchievementRuntime::ReadMemory(uint32_t nAddress, uint8_t* pBuffer, uint32_t nBytes, rc_client_t*)
{
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    return pEmulatorContext.ReadMemory(nAddress, pBuffer, nBytes);
}

static void ConvertHttpResponseToApiServerResponse(rc_api_server_response_t& pResponse,
                                                   const ra::services::Http::Response& httpResponse,
                                                   std::string& sErrorBuffer)
{
    memset(&pResponse, 0, sizeof(pResponse));
    pResponse.http_status_code = ra::etoi(httpResponse.StatusCode());
    pResponse.body = httpResponse.Content().c_str();
    pResponse.body_length = httpResponse.Content().length();

    if (pResponse.http_status_code > 599 || pResponse.body_length == 0)
    {
        const auto& pHttpRequestService = ra::services::ServiceLocator::Get<ra::services::IHttpRequester>();
        sErrorBuffer = pHttpRequestService.GetStatusCodeText(pResponse.http_status_code);
        pResponse.body = sErrorBuffer.c_str();
        pResponse.body_length = sErrorBuffer.length();
        pResponse.http_status_code = pHttpRequestService.IsRetryable(pResponse.http_status_code) ?
            RC_API_SERVER_RESPONSE_RETRYABLE_CLIENT_ERROR : RC_API_SERVER_RESPONSE_CLIENT_ERROR;
    }
}

void AchievementRuntime::ServerCallAsync(const rc_api_request_t* pRequest, rc_client_server_callback_t fCallback,
                                     void* pCallbackData, rc_client_t*)
{
    ra::services::Http::Request httpRequest(pRequest->url);
    httpRequest.SetPostData(pRequest->post_data);
    httpRequest.SetContentType(pRequest->content_type);

    std::string sApi;
    auto nIndex = httpRequest.GetPostData().find("r=", 0, 2);
    if (nIndex != std::string::npos)
    {
        nIndex += 2;
        for (; nIndex < httpRequest.GetPostData().length(); ++nIndex)
        {
            const char c = httpRequest.GetPostData().at(nIndex);
            if (c == '&')
                break;

            sApi.push_back(c);
        }

        std::string sParams;
        bool redacted = false;
        for (const char c : httpRequest.GetPostData())
        {
            if (c == '=')
            {
                const auto param = sParams.back();
                redacted = (param == 't') || (param == 'p' && ra::StringStartsWith(sApi, "login"));

                if (redacted)
                {
                    sParams.append("=[redacted]");
                    continue;
                }
            }
            else if (c == '&')
            {
                redacted = false;
            }
            else if (redacted)
            {
                continue;
            }

            sParams.push_back(c);
        }
        RA_LOG_INFO(">> %s request: %s", sApi.c_str(), sParams.c_str());
    }

    httpRequest.CallAsync([fCallback, pCallbackData, sApi](const ra::services::Http::Response& httpResponse) {
        rc_api_server_response_t pResponse;
        std::string sErrorBuffer;
        ConvertHttpResponseToApiServerResponse(pResponse, httpResponse, sErrorBuffer);

        RA_LOG_INFO("<< %s response (%d): %s", sApi.c_str(), ra::etoi(httpResponse.StatusCode()),
                    httpResponse.Content().c_str());

        fCallback(&pResponse, pCallbackData);
    });
}

/* ---- ClientSynchronizer ----- */

class AchievementRuntime::ClientSynchronizer
{
public:
    ClientSynchronizer() = default;
    virtual ~ClientSynchronizer()
    {
        m_pPublishedSubset = nullptr;

        for (auto* pMemory : m_vAllocatedMemory)
            free(pMemory);
    }
    ClientSynchronizer(const ClientSynchronizer&) noexcept = delete;
    ClientSynchronizer& operator=(const ClientSynchronizer&) noexcept = delete;
    ClientSynchronizer(ClientSynchronizer&&) noexcept = delete;
    ClientSynchronizer& operator=(ClientSynchronizer&&) noexcept = delete;

private:
    rc_client_subset_info_t* m_pPublishedSubset = nullptr;
    rc_client_t* m_pClient = nullptr;
    rc_memref_t** m_pNextMemref = nullptr;
    rc_value_t** m_pNextVariable = nullptr;

    typedef struct rc_client_subset_wrapper_t
    {
        rc_client_subset_info_t pCoreSubset{};
        rc_client_subset_info_t pLocalSubset{};
        rc_buffer_t pBuffer{};
        std::vector<void*> vAllocatedMemory;
    } SubsetWrapper;
    std::unique_ptr<SubsetWrapper> m_pSubsetWrapper;

    std::vector<void*> m_vAllocatedMemory;

    static bool DetachMemory(std::vector<void*>& pAllocatedMemory, void* pMemory) noexcept
    {
        for (auto pIter = pAllocatedMemory.begin(); pIter != pAllocatedMemory.end(); ++pIter)
        {
            if (*pIter == pMemory)
            {
                pAllocatedMemory.erase(pIter);
                return true;
            }
        }

        return false;
    }

    bool AllocatedMemrefs() noexcept
    {
        bool bAllocatedMemref = false;

        /* if at least one memref was allocated within the object, we can't free the buffer when the object is
         * deactivated */
        if (*m_pNextMemref != nullptr)
        {
            bAllocatedMemref = true;
            /* advance through the new memrefs so we're ready for the next allocation */
            do
            {
                m_pNextMemref = &(*m_pNextMemref)->next;
            } while (*m_pNextMemref != nullptr);
        }

        /* if at least one variable was allocated within the object, we can't free the buffer when the object is
         * deactivated */
        if (*m_pNextVariable != nullptr)
        {
            bAllocatedMemref = true;
            /* advance through the new variables so we're ready for the next allocation */
            do
            {
                m_pNextVariable = &(*m_pNextVariable)->next;
            } while (*m_pNextVariable != nullptr);
        }

        return bAllocatedMemref;
    }

    static rc_client_leaderboard_info_t* FindLeaderboard(rc_client_subset_info_t* pSubset, uint32_t nId)
    {
        Expects(pSubset != nullptr);

        auto* pLeaderboard = pSubset->leaderboards;
        const auto* pLeaderboardStop = pLeaderboard + pSubset->public_.num_leaderboards;
        for (; pLeaderboard < pLeaderboardStop; ++pLeaderboard)
        {
            if (pLeaderboard->public_.id == nId)
                return pLeaderboard;
        }

        return nullptr;
    }

    static rc_client_achievement_info_t* FindAchievement(rc_client_subset_info_t* pSubset, uint32_t nId)
    {
        Expects(pSubset != nullptr);

        auto* pAchievement = pSubset->achievements;
        const auto* pAchievementStop = pAchievement + pSubset->public_.num_achievements;
        for (; pAchievement < pAchievementStop; ++pAchievement)
        {
            if (pAchievement->public_.id == nId)
                return pAchievement;
        }

        return nullptr;
    }

    void SyncSubset(rc_client_subset_info_t* pSubset, SubsetWrapper* pSubsetWrapper,
                    std::vector<ra::data::models::AchievementModel*>& vAchievements,
                    std::vector<ra::data::models::LeaderboardModel*>& vLeaderboards)
    {
        pSubset->active = true;

        if (!vAchievements.empty())
        {
            pSubset->achievements = static_cast<rc_client_achievement_info_t*>(
                rc_buffer_alloc(&pSubsetWrapper->pBuffer, vAchievements.size() * sizeof(*pSubset->achievements)));
            memset(pSubset->achievements, 0, vAchievements.size() * sizeof(*pSubset->achievements));

            rc_client_achievement_info_t* pAchievement = pSubset->achievements;
            rc_client_achievement_info_t* pSrcAchievement = nullptr;
            for (auto* vmAchievement : vAchievements)
            {
                Expects(vmAchievement != nullptr);

                const auto nAchievementId = vmAchievement->GetID();
                if (vmAchievement->GetChanges() == ra::data::models::AssetChanges::None)
                {
                    rc_client_subset_info_t* pPublishedSubset = m_pPublishedSubset;
                    for (; pPublishedSubset; pPublishedSubset = pPublishedSubset->next)
                    {
                        pSrcAchievement = FindAchievement(pPublishedSubset, nAchievementId);
                        if (pSrcAchievement != nullptr)
                        {
                            memcpy(pAchievement, pSrcAchievement, sizeof(*pAchievement));
                            vmAchievement->ReplaceAttached(*pAchievement++);
                            break;
                        }
                    }
                }
                else
                {
                    pSrcAchievement = nullptr;
                    if (m_pSubsetWrapper != nullptr)
                    {
                        if (vmAchievement->GetCategory() == ra::data::models::AssetCategory::Local)
                            pSrcAchievement = FindAchievement(&m_pSubsetWrapper->pLocalSubset, nAchievementId);
                        else
                            pSrcAchievement = FindAchievement(&m_pSubsetWrapper->pCoreSubset, nAchievementId);
                    }

                    if (pSrcAchievement != nullptr && pSrcAchievement == vmAchievement->GetAttached())
                    {
                        // transfer the trigger ownership to the new SubsetWrapper
                        if (pSrcAchievement->trigger && DetachMemory(m_pSubsetWrapper->vAllocatedMemory, pSrcAchievement->trigger))
                            pSubsetWrapper->vAllocatedMemory.push_back(pSrcAchievement->trigger);

                        memcpy(pAchievement, pSrcAchievement, sizeof(*pAchievement));
                        vmAchievement->ReplaceAttached(*pAchievement++);
                    }
                    else
                    {
                        if (pSrcAchievement != nullptr && pSrcAchievement->trigger)
                        {
                            const auto& sTrigger = vmAchievement->GetTrigger();
                            uint8_t md5[16];
                            rc_runtime_checksum(sTrigger.c_str(), md5);
                            if (memcmp(pSrcAchievement->md5, md5, sizeof(md5)) == 0)
                            {
                                memcpy(pAchievement->md5, pSrcAchievement->md5, sizeof(md5));
                                pAchievement->trigger = pSrcAchievement->trigger;

                                // transfer the trigger ownership to the new SubsetWrapper
                                if (DetachMemory(m_pSubsetWrapper->vAllocatedMemory, pSrcAchievement->trigger))
                                    pSubsetWrapper->vAllocatedMemory.push_back(pSrcAchievement->trigger);
                            }
                        }

                        // no rc_client_achievement_t found for this achievement, populate from the model
                        vmAchievement->AttachAndInitialize(*pAchievement++);
                    }
                }
            }

            pSubset->public_.num_achievements = gsl::narrow_cast<uint32_t>(pAchievement - pSubset->achievements);
        }

        if (!vLeaderboards.empty())
        {
            pSubset->leaderboards = static_cast<rc_client_leaderboard_info_t*>(
                rc_buffer_alloc(&pSubsetWrapper->pBuffer, vLeaderboards.size() * sizeof(*pSubset->leaderboards)));
            memset(pSubset->leaderboards, 0, vLeaderboards.size() * sizeof(*pSubset->leaderboards));

            rc_client_leaderboard_info_t* pLeaderboard = pSubset->leaderboards;
            rc_client_leaderboard_info_t* pSrcLeaderboard = nullptr;
            for (auto* vmLeaderboard : vLeaderboards)
            {
                Expects(vmLeaderboard != nullptr);

                const auto nLeaderboardId = vmLeaderboard->GetID();
                if (vmLeaderboard->GetChanges() == ra::data::models::AssetChanges::None)
                {
                    rc_client_subset_info_t* pPublishedSubset = m_pPublishedSubset;
                    for (; pPublishedSubset; pPublishedSubset = pPublishedSubset->next)
                    {
                        pSrcLeaderboard = FindLeaderboard(pPublishedSubset, nLeaderboardId);
                        if (pSrcLeaderboard != nullptr)
                        {
                            memcpy(pLeaderboard, pSrcLeaderboard, sizeof(*pLeaderboard));
                            vmLeaderboard->ReplaceAttached(*pLeaderboard++);
                            break;
                        }
                    }
                }
                else
                {
                    pSrcLeaderboard = nullptr;
                    if (m_pSubsetWrapper != nullptr && vmLeaderboard->GetAttached() != nullptr)
                    {
                        if (vmLeaderboard->GetCategory() == ra::data::models::AssetCategory::Local)
                            pSrcLeaderboard = FindLeaderboard(&m_pSubsetWrapper->pLocalSubset, nLeaderboardId);
                        else
                            pSrcLeaderboard = FindLeaderboard(&m_pSubsetWrapper->pCoreSubset, nLeaderboardId);
                    }

                    if (pSrcLeaderboard != nullptr && pSrcLeaderboard == vmLeaderboard->GetAttached())
                    {
                        // transfer the lboard ownership to the new SubsetWrapper
                        if (pSrcLeaderboard->lboard && DetachMemory(m_pSubsetWrapper->vAllocatedMemory, pSrcLeaderboard->lboard))
                            pSubsetWrapper->vAllocatedMemory.push_back(pSrcLeaderboard->lboard);

                        memcpy(pLeaderboard, pSrcLeaderboard, sizeof(*pLeaderboard));
                        vmLeaderboard->ReplaceAttached(*pLeaderboard++);
                    }
                    else
                    {
                        if (pSrcLeaderboard != nullptr && pSrcLeaderboard->lboard)
                        {
                            const auto& sTrigger = vmLeaderboard->GetDefinition();
                            uint8_t md5[16];
                            rc_runtime_checksum(sTrigger.c_str(), md5);
                            if (memcmp(pSrcLeaderboard->md5, md5, sizeof(md5)) == 0)
                            {
                                memcpy(pLeaderboard->md5, pSrcLeaderboard->md5, sizeof(md5));
                                pLeaderboard->lboard = pSrcLeaderboard->lboard;

                                // transfer the trigger ownership to the new SubsetWrapper
                                if (DetachMemory(m_pSubsetWrapper->vAllocatedMemory, pSrcLeaderboard->lboard))
                                    pSubsetWrapper->vAllocatedMemory.push_back(pSrcLeaderboard->lboard);
                            }
                        }

                        // no rc_client_leaderboard_t found for this leaderboard, populate from the model
                        vmLeaderboard->AttachAndInitialize(*pLeaderboard++);
                    }
                }
            }

            pSubset->public_.num_leaderboards = gsl::narrow_cast<uint32_t>(pLeaderboard - pSubset->leaderboards);
        }
    }

public:
    void SyncAssets(rc_client_t* pClient)
    {
        if (!pClient->game)
            return;

        auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
        auto& vAssets = pGameContext.Assets();

        if (m_pClient == nullptr)
        {
            m_pClient = pClient;
            m_pNextMemref = &pClient->game->runtime.memrefs;
            m_pNextVariable = &pClient->game->runtime.variables;
            m_pPublishedSubset = pClient->game->subsets;
            AllocatedMemrefs(); // advance pointers
        }

        std::vector<ra::data::models::AchievementModel*> vCoreAchievements;
        std::vector<ra::data::models::AchievementModel*> vLocalAchievements;
        std::vector<ra::data::models::LeaderboardModel*> vCoreLeaderboards;
        std::vector<ra::data::models::LeaderboardModel*> vLocalLeaderboards;

        for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(vAssets.Count()); ++nIndex)
        {
            auto* pAsset = vAssets.GetItemAt(nIndex);
            Expects(pAsset != nullptr);
            if (pAsset->GetChanges() == ra::data::models::AssetChanges::Deleted)
                continue;

            auto* vmAchievement = dynamic_cast<ra::data::models::AchievementModel*>(pAsset);
            if (vmAchievement != nullptr)
            {
                if (vmAchievement->GetCategory() == ra::data::models::AssetCategory::Local)
                    vLocalAchievements.push_back(vmAchievement);
                else
                    vCoreAchievements.push_back(vmAchievement);

                continue;
            }

            auto* vmLeaderboard = dynamic_cast<ra::data::models::LeaderboardModel*>(pAsset);
            if (vmLeaderboard != nullptr)
            {
                if (vmLeaderboard->GetCategory() == ra::data::models::AssetCategory::Local)
                    vLocalLeaderboards.push_back(vmLeaderboard);
                else
                    vCoreLeaderboards.push_back(vmLeaderboard);

                continue;
            }
        }

        std::unique_ptr<SubsetWrapper> pSubsetWrapper;
        pSubsetWrapper.reset(new SubsetWrapper);
        memset(&pSubsetWrapper->pCoreSubset, 0, sizeof(pSubsetWrapper->pCoreSubset));
        memset(&pSubsetWrapper->pLocalSubset, 0, sizeof(pSubsetWrapper->pLocalSubset));
        rc_buffer_init(&pSubsetWrapper->pBuffer);

        pSubsetWrapper->pCoreSubset.public_.id = pClient->game->public_.id;
        pSubsetWrapper->pCoreSubset.public_.title = pClient->game->public_.title;
        snprintf(pSubsetWrapper->pCoreSubset.public_.badge_name, sizeof(pSubsetWrapper->pCoreSubset.public_.badge_name),
                 "%s", pClient->game->public_.badge_name);

        SyncSubset(&pSubsetWrapper->pCoreSubset, pSubsetWrapper.get(), vCoreAchievements, vCoreLeaderboards);

        if (!vLocalAchievements.empty() || !vLocalLeaderboards.empty())
        {
            pSubsetWrapper->pLocalSubset.public_.id = ra::data::context::GameAssets::LocalSubsetId;
            pSubsetWrapper->pLocalSubset.public_.title = "Local";
            snprintf(pSubsetWrapper->pLocalSubset.public_.badge_name,
                     sizeof(pSubsetWrapper->pLocalSubset.public_.badge_name), "%s", pClient->game->public_.badge_name);

            SyncSubset(&pSubsetWrapper->pLocalSubset, pSubsetWrapper.get(), vLocalAchievements, vLocalLeaderboards);

            pSubsetWrapper->pCoreSubset.next = &pSubsetWrapper->pLocalSubset;
        }

        std::swap(pSubsetWrapper, m_pSubsetWrapper);

        if (pSubsetWrapper != nullptr)
        {
            for (auto* pMemory : pSubsetWrapper->vAllocatedMemory)
                free(pMemory);
        }

        rc_mutex_lock(&pClient->state.mutex);
        pClient->game->subsets = &m_pSubsetWrapper->pCoreSubset;
        rc_mutex_unlock(&pClient->state.mutex);
    }

    void AttachMemory(void* pMemory)
    {
        if (AllocatedMemrefs())
        {
            // if memrefs were allocated, we can't release this memory until the game is unloaded
            m_vAllocatedMemory.push_back(pMemory);
        }
        else
        {
            m_pSubsetWrapper->vAllocatedMemory.push_back(pMemory);
        }
    }

    bool DetachMemory(void* pMemory) noexcept
    {
        if (!m_pSubsetWrapper)
            return false;

        return DetachMemory(m_pSubsetWrapper->vAllocatedMemory, pMemory);
    }

};

void AchievementRuntime::SyncAssets()
{
    if (m_pClientSynchronizer == nullptr)
        m_pClientSynchronizer.reset(new ClientSynchronizer());
    m_pClientSynchronizer->SyncAssets(GetClient());
}

void AchievementRuntime::AttachMemory(void* pMemory)
{
    if (m_pClientSynchronizer != nullptr)
        m_pClientSynchronizer->AttachMemory(pMemory);
}

bool AchievementRuntime::DetachMemory(void* pMemory) noexcept
{
    if (m_pClientSynchronizer != nullptr)
        return m_pClientSynchronizer->DetachMemory(pMemory);

    return false;
}

static rc_client_achievement_info_t* GetAchievementInfo(rc_client_t* pClient, ra::AchievementID nId) noexcept
{
    rc_client_subset_info_t* subset = pClient->game->subsets;
    for (; subset; subset = subset->next)
    {
        rc_client_achievement_info_t* achievement = subset->achievements;
        const rc_client_achievement_info_t* stop = achievement + subset->public_.num_achievements;

        for (; achievement < stop; ++achievement)
        {
            if (achievement->public_.id == nId)
                return achievement;
        }
    }

    return nullptr;
}

rc_trigger_t* AchievementRuntime::GetAchievementTrigger(ra::AchievementID nId) const noexcept
{
    rc_client_achievement_info_t* achievement = GetAchievementInfo(GetClient(), nId);
    return (achievement != nullptr) ? achievement->trigger : nullptr;
}

std::string AchievementRuntime::GetAchievementBadge(const rc_client_achievement_t& pAchievement)
{
    std::string sBadgeName = pAchievement.badge_name;

    if (!sBadgeName.empty() && sBadgeName.front() == 'L')
    {
        // local image, get from model
        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
        auto* vmLocalAchievement = pGameContext.Assets().FindAchievement(pAchievement.id);
        if (vmLocalAchievement != nullptr)
            sBadgeName = ra::Narrow(vmLocalAchievement->GetBadge());
    }

    return sBadgeName;
}

void AchievementRuntime::UpdateActiveAchievements() noexcept
{
    auto* client = GetClient();
    if (client->game)
    {
        rc_mutex_lock(&client->state.mutex);
        if (client->game && (client->game->pending_events & RC_CLIENT_GAME_PENDING_EVENT_UPDATE_ACTIVE_ACHIEVEMENTS) == 0)
            rc_client_update_active_achievements(client->game);
        rc_mutex_unlock(&client->state.mutex);
    }
}

void AchievementRuntime::UpdateActiveLeaderboards() noexcept
{
    auto* client = GetClient();
    if (client->game)
    {
        rc_mutex_lock(&client->state.mutex);
        if (client->game)
            rc_client_update_active_leaderboards(client->game);
        rc_mutex_unlock(&client->state.mutex);
    }
}

static rc_client_leaderboard_info_t* GetLeaderboardInfo(rc_client_t* pClient, ra::LeaderboardID nId) noexcept
{
    rc_client_subset_info_t* subset = pClient->game->subsets;
    for (; subset; subset = subset->next)
    {
        rc_client_leaderboard_info_t* leaderboard = subset->leaderboards;
        const rc_client_leaderboard_info_t* stop = leaderboard + subset->public_.num_leaderboards;

        for (; leaderboard < stop; ++leaderboard)
        {
            if (leaderboard->public_.id == nId)
                return leaderboard;
        }
    }

    return nullptr;
}

rc_lboard_t* AchievementRuntime::GetLeaderboardDefinition(ra::LeaderboardID nId) const noexcept
{
    rc_client_leaderboard_info_t* leaderboard = GetLeaderboardInfo(GetClient(), nId);
    return (leaderboard != nullptr) ? leaderboard->lboard : nullptr;
}

void AchievementRuntime::ReleaseLeaderboardTracker(ra::LeaderboardID nId) noexcept
{
    rc_client_leaderboard_info_t* leaderboard = GetLeaderboardInfo(GetClient(), nId);
    if (leaderboard)
        rc_client_release_leaderboard_tracker(GetClient()->game, leaderboard);
}

bool AchievementRuntime::HasRichPresence() const noexcept
{
    return m_nRichPresenceParseResult != RC_OK || rc_client_has_rich_presence(GetClient());
}

std::wstring AchievementRuntime::GetRichPresenceDisplayString() const
{
    if (m_nRichPresenceParseResult != RC_OK)
    {
        return ra::StringPrintf(L"Parse error %d (line %d): %s", m_nRichPresenceParseResult,
            m_nRichPresenceErrorLine, rc_error_str(m_nRichPresenceParseResult));
    }

    if (!HasRichPresence())
        return L"No Rich Presence defined.";

    char sRichPresence[256];
    if (rc_client_get_rich_presence_message(GetClient(), sRichPresence, sizeof(sRichPresence)) > 0)
        return ra::Widen(sRichPresence);

    return L"";
}

bool AchievementRuntime::ActivateRichPresence(const std::string& sScript)
{
    auto* game = GetClient()->game;
    if (!game)
    {
        // game still loading - assume success
        return true;
    }
    auto* runtime = &game->runtime;
    Expects(runtime != nullptr);

    rc_mutex_lock(&GetClient()->state.mutex);

    if (sScript.empty())
    {
        m_nRichPresenceParseResult = RC_OK;
        if (runtime->richpresence != nullptr)
            runtime->richpresence->richpresence = nullptr;
    }
    else
    {
        m_nRichPresenceParseResult = rc_runtime_activate_richpresence(runtime, sScript.c_str(), nullptr, 0);
        if (m_nRichPresenceParseResult != RC_OK)
            m_nRichPresenceParseResult = rc_richpresence_size_lines(sScript.c_str(), &m_nRichPresenceErrorLine);
    }

    rc_mutex_unlock(&GetClient()->state.mutex);

    return (m_nRichPresenceParseResult == RC_OK);
}

/* ---- Login ----- */

void AchievementRuntime::BeginLoginWithPassword(const std::string& sUsername, const std::string& sPassword,
                                            rc_client_callback_t fCallback, void* pCallbackData)
{
    GSL_SUPPRESS_R3
    auto* pCallbackWrapper = new CallbackWrapper(m_pClient.get(), fCallback, pCallbackData);
    BeginLoginWithPassword(sUsername.c_str(), sPassword.c_str(), pCallbackWrapper);
}

rc_client_async_handle_t* AchievementRuntime::BeginLoginWithPassword(const char* sUsername, const char* sPassword,
                                                                 CallbackWrapper* pCallbackWrapper) noexcept
{
    return rc_client_begin_login_with_password(GetClient(), sUsername, sPassword, AchievementRuntime::LoginCallback,
                                               pCallbackWrapper);
}

void AchievementRuntime::BeginLoginWithToken(const std::string& sUsername, const std::string& sApiToken,
                                         rc_client_callback_t fCallback, void* pCallbackData)
{
    GSL_SUPPRESS_R3
    auto* pCallbackWrapper = new CallbackWrapper(m_pClient.get(), fCallback, pCallbackData);
    BeginLoginWithToken(sUsername.c_str(), sApiToken.c_str(), pCallbackWrapper);
}

rc_client_async_handle_t* AchievementRuntime::BeginLoginWithToken(const char* sUsername, const char* sApiToken,
                                                              CallbackWrapper* pCallbackWrapper) noexcept
{
    return rc_client_begin_login_with_token(GetClient(), sUsername, sApiToken, AchievementRuntime::LoginCallback,
                                            pCallbackWrapper);
}

GSL_SUPPRESS_CON3
void AchievementRuntime::LoginCallback(int nResult, const char* sErrorMessage, rc_client_t* pClient, void* pUserdata)
{
    if (nResult == RC_OK)
    {
        const auto* user = rc_client_get_user_info(pClient);
        if (!user)
        {
            nResult = RC_INVALID_STATE;
            sErrorMessage = "User data not available.";
        }
        else
        {
            // initialize the user context
            auto& pUserContext = ra::services::ServiceLocator::GetMutable<ra::data::context::UserContext>();
            if (pUserContext.IsLoginDisabled())
            {
                nResult = RC_INVALID_STATE;
                sErrorMessage = "Login has been disabled.";
            }
            else
            {
                pUserContext.Initialize(user->username, user->display_name, user->token);
                pUserContext.SetScore(user->score);
            }
        }
    }

    auto* wrapper = static_cast<CallbackWrapper*>(pUserdata);
    Expects(wrapper != nullptr);
    wrapper->DoCallback(nResult, sErrorMessage);

    delete wrapper;
}

/* ---- Load Game ----- */

void AchievementRuntime::BeginLoadGame(const std::string& sHash, unsigned id, rc_client_callback_t fCallback,
                                   void* pCallbackData)
{
    GSL_SUPPRESS_R3
    auto* pCallbackWrapper = new LoadGameCallbackWrapper(m_pClient.get(), fCallback, pCallbackData);
    BeginLoadGame(sHash.c_str(), id, pCallbackWrapper);
}

static void ExtractPatchData(const rc_api_server_response_t* server_response, uint32_t nGameId)
{
    // extract the PatchData and store a copy in the cache for offline mode
    rc_api_response_t api_response{};
    GSL_SUPPRESS_ES47
    rc_json_field_t fields[] = {RC_JSON_NEW_FIELD("Success"), RC_JSON_NEW_FIELD("Error"),
                                RC_JSON_NEW_FIELD("PatchData")};

    if (rc_json_parse_server_response(&api_response, server_response, fields, sizeof(fields) / sizeof(fields[0])) ==
            RC_OK &&
        fields[2].value_start && fields[2].value_end)
    {
        auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
        auto pData = pLocalStorage.WriteText(ra::services::StorageItemType::GameData, std::to_wstring(nGameId));
        if (pData != nullptr)
        {
            std::string sPatchData;
            sPatchData.append(fields[2].value_start, fields[2].value_end - fields[2].value_start);
            pData->Write(sPatchData);
        }
    }
}

GSL_SUPPRESS_CON3
void AchievementRuntime::PostProcessGameDataResponse(const rc_api_server_response_t* server_response,
                                                 struct rc_api_fetch_game_data_response_t* game_data_response,
                                                 rc_client_t* client, void* pUserdata)
{
    auto* wrapper = static_cast<LoadGameCallbackWrapper*>(pUserdata);
    Expects(wrapper != nullptr);

    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();

    auto pRichPresence = std::make_unique<ra::data::models::RichPresenceModel>();
    pRichPresence->SetScript(game_data_response->rich_presence_script);
    pRichPresence->CreateServerCheckpoint();
    pRichPresence->CreateLocalCheckpoint();
    pGameContext.Assets().Append(std::move(pRichPresence));

#ifndef RA_UTEST
    // prefetch the game icon
    if (client->game)
    {
        auto& pImageRepository = ra::services::ServiceLocator::GetMutable<ra::ui::IImageRepository>();
        pImageRepository.FetchImage(ra::ui::ImageType::Icon, client->game->public_.badge_name);
    }
#else
    (void*)client;
#endif

    const rc_api_achievement_definition_t* pAchievement = game_data_response->achievements;
    const rc_api_achievement_definition_t* pAchievementStop = pAchievement + game_data_response->num_achievements;
    for (; pAchievement < pAchievementStop; ++pAchievement)
        wrapper->m_mAchievementDefinitions[pAchievement->id] = pAchievement->definition;

    const rc_api_leaderboard_definition_t* pLeaderboard = game_data_response->leaderboards;
    const rc_api_leaderboard_definition_t* pLeaderboardStop = pLeaderboard + game_data_response->num_leaderboards;
    for (; pLeaderboard < pLeaderboardStop; ++pLeaderboard)
        wrapper->m_mLeaderboardDefinitions[pLeaderboard->id] = pLeaderboard->definition;

    ExtractPatchData(server_response, game_data_response->id);
}

rc_client_async_handle_t* AchievementRuntime::BeginLoadGame(const char* sHash, unsigned id,
                                                        CallbackWrapper* pCallbackWrapper) noexcept
{
    auto* client = GetClient();

    // unload the game and free any additional memory we allocated for local changes
    rc_client_unload_game(client);
    m_pClientSynchronizer.reset();
    s_mAchievementPopups.clear();

    // if an ID was provided, store the hash->id mapping
    if (id != 0)
    {
        auto* client_hash = rc_client_find_game_hash(client, sHash);
        if (ra::to_signed(client_hash->game_id) < 0)
            client_hash->game_id = id;
    }

    // start the load process
    client->callbacks.post_process_game_data_response = PostProcessGameDataResponse;

    return rc_client_begin_load_game(client, sHash, AchievementRuntime::LoadGameCallback, pCallbackWrapper);
}

rc_client_async_handle_t* AchievementRuntime::BeginIdentifyAndLoadGame(uint32_t console_id, const char* file_path,
                                                                       const uint8_t* data, size_t data_size,
                                                                       CallbackWrapper* pCallbackWrapper) noexcept
{
    auto* client = GetClient();

    // unload the game and free any additional memory we allocated for local changes
    rc_client_unload_game(client);
    m_pClientSynchronizer.reset();
    s_mAchievementPopups.clear();

    // start the load process
    client->callbacks.post_process_game_data_response = PostProcessGameDataResponse;

    return rc_client_begin_identify_and_load_game(client, console_id, file_path, data, data_size,
                                                  AchievementRuntime::LoadGameCallback, pCallbackWrapper);
}

GSL_SUPPRESS_CON3
void AchievementRuntime::LoadGameCallback(int nResult, const char* sErrorMessage, rc_client_t*, void* pUserdata)
{
    auto* wrapper = static_cast<LoadGameCallbackWrapper*>(pUserdata);
    Expects(wrapper != nullptr);

    if (nResult == RC_OK || nResult == RC_NO_GAME_LOADED)
    {
        // initialize the game context
        auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
        pGameContext.InitializeFromAchievementRuntime(wrapper->m_mAchievementDefinitions,
                                                      wrapper->m_mLeaderboardDefinitions);
    }
    else
    {
    }

    wrapper->DoCallback(nResult, sErrorMessage);

    delete wrapper;
}

rc_client_async_handle_t* AchievementRuntime::BeginChangeMedia(const char* file_path,
    const uint8_t* data, size_t data_size, CallbackWrapper* pCallbackWrapper) noexcept
{
    auto* client = GetClient();
    return rc_client_begin_change_media(client, file_path, data, data_size, AchievementRuntime::ChangeMediaCallback, pCallbackWrapper);
}

GSL_SUPPRESS_CON3
void AchievementRuntime::ChangeMediaCallback(int nResult, const char* sErrorMessage, rc_client_t*, void* pUserdata)
{
    auto* wrapper = static_cast<CallbackWrapper*>(pUserdata);
    Expects(wrapper != nullptr);

    if (nResult == RC_HARDCORE_DISABLED)
        ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>().DisableHardcoreMode();

    wrapper->DoCallback(nResult, sErrorMessage);

    delete wrapper;
}


/* ---- DoFrame ----- */

static void PrepareForPauseOnChangeEvents(rc_client_t* pClient,
    std::vector<rc_client_achievement_info_t*>& vAchievementsWithHits,
    std::vector<rc_client_achievement_info_t*>& vActiveAchievements,
    std::map<rc_client_leaderboard_info_t*, ra::data::models::LeaderboardModel::LeaderboardParts>& mLeaderboardsWithHits,
    std::map<rc_client_leaderboard_info_t*, ra::data::models::LeaderboardModel::LeaderboardParts>& mActiveLeaderboards)
{
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(pGameContext.Assets().Count()); ++nIndex)
    {
        auto* vmAsset = pGameContext.Assets().GetItemAt(nIndex);

        const auto* vmAchievement = dynamic_cast<ra::data::models::AchievementModel*>(vmAsset);
        if (vmAchievement != nullptr)
        {
            if (vmAchievement->IsPauseOnReset())
            {
                auto* pAchievement = GetAchievementInfo(pClient, vmAchievement->GetID());
                if (pAchievement && pAchievement->trigger && pAchievement->trigger->has_hits)
                    vAchievementsWithHits.push_back(pAchievement);
            }

            if (vmAchievement->IsPauseOnTrigger())
            {
                auto* pAchievement = GetAchievementInfo(pClient, vmAchievement->GetID());
                if (pAchievement && pAchievement->trigger)
                {
                    switch (pAchievement->trigger->state)
                    {
                        case RC_TRIGGER_STATE_DISABLED:
                        case RC_TRIGGER_STATE_INACTIVE:
                        case RC_TRIGGER_STATE_PAUSED:
                        case RC_TRIGGER_STATE_TRIGGERED:
                            break;

                        default:
                            vActiveAchievements.push_back(pAchievement);
                            break;
                    }
                }
            }

            continue;
        }

        const auto* vmLeaderboard = dynamic_cast<ra::data::models::LeaderboardModel*>(vmAsset);
        if (vmLeaderboard != nullptr)
        {
            using namespace ra::bitwise_ops;

            auto nPauseOnReset = vmLeaderboard->GetPauseOnReset();
            if (nPauseOnReset != ra::data::models::LeaderboardModel::LeaderboardParts::None)
            {
                auto* pLeaderboard = GetLeaderboardInfo(pClient, vmLeaderboard->GetID());
                if (pLeaderboard && pLeaderboard->lboard)
                {
                    if (!pLeaderboard->lboard->start.has_hits)
                        nPauseOnReset &= ~ra::data::models::LeaderboardModel::LeaderboardParts::Start;
                    if (!pLeaderboard->lboard->cancel.has_hits)
                        nPauseOnReset &= ~ra::data::models::LeaderboardModel::LeaderboardParts::Cancel;
                    if (!pLeaderboard->lboard->submit.has_hits)
                        nPauseOnReset &= ~ra::data::models::LeaderboardModel::LeaderboardParts::Submit;
                    if (!pLeaderboard->lboard->value.value.value)
                        nPauseOnReset &= ~ra::data::models::LeaderboardModel::LeaderboardParts::Value;

                    if (nPauseOnReset != ra::data::models::LeaderboardModel::LeaderboardParts::None)
                        mLeaderboardsWithHits[pLeaderboard] = nPauseOnReset;
                }
            }

            auto nPauseOnTrigger = vmLeaderboard->GetPauseOnTrigger();
            if (nPauseOnTrigger != ra::data::models::LeaderboardModel::LeaderboardParts::None)
            {
                auto* pLeaderboard = GetLeaderboardInfo(pClient, vmLeaderboard->GetID());
                if (pLeaderboard && pLeaderboard->lboard)
                {
                    if (pLeaderboard->lboard->start.state == RC_TRIGGER_STATE_TRIGGERED)
                        nPauseOnTrigger &= ~ra::data::models::LeaderboardModel::LeaderboardParts::Start;
                    if (pLeaderboard->lboard->cancel.state == RC_TRIGGER_STATE_TRIGGERED)
                        nPauseOnTrigger &= ~ra::data::models::LeaderboardModel::LeaderboardParts::Cancel;
                    if (pLeaderboard->lboard->submit.state == RC_TRIGGER_STATE_TRIGGERED)
                        nPauseOnTrigger &= ~ra::data::models::LeaderboardModel::LeaderboardParts::Submit;

                    if (nPauseOnTrigger != ra::data::models::LeaderboardModel::LeaderboardParts::None)
                        mActiveLeaderboards[pLeaderboard] = nPauseOnTrigger;
                }
            }
        }
    }
}

static void RaisePauseOnChangeEvents(std::vector<rc_client_achievement_info_t*>& vAchievementsWithHits,
                                     std::vector<rc_client_achievement_info_t*>& vActiveAchievements)
{
    for (const auto* pAchievement : vAchievementsWithHits)
    {
        if (pAchievement != nullptr && pAchievement->trigger && !pAchievement->trigger->has_hits)
        {
            auto& pFrameEventQueue = ra::services::ServiceLocator::GetMutable<ra::services::FrameEventQueue>();
            pFrameEventQueue.QueuePauseOnReset(ra::Widen(pAchievement->public_.title));
        }
    }

    for (const auto* pAchievement : vActiveAchievements)
    {
        if (pAchievement && pAchievement->trigger && pAchievement->trigger->state == RC_TRIGGER_STATE_TRIGGERED)
        {
            auto& pFrameEventQueue = ra::services::ServiceLocator::GetMutable<ra::services::FrameEventQueue>();
            pFrameEventQueue.QueuePauseOnTrigger(ra::Widen(pAchievement->public_.title));
        }
    }
}

static void RaisePauseOnChangeEvents(
    std::map<rc_client_leaderboard_info_t*, ra::data::models::LeaderboardModel::LeaderboardParts>& mLeaderboardsWithHits,
    std::map<rc_client_leaderboard_info_t*, ra::data::models::LeaderboardModel::LeaderboardParts>& mActiveLeaderboards)
{
    for (auto pair : mLeaderboardsWithHits)
    {
        if (pair.first && pair.first->lboard)
        {
            using namespace ra::bitwise_ops;

            auto nPauseOnReset = pair.second;
            const auto* pLeaderboard = pair.first->lboard;
            if (pLeaderboard->start.has_hits)
                nPauseOnReset &= ~ra::data::models::LeaderboardModel::LeaderboardParts::Start;
            if (pLeaderboard->cancel.has_hits)
                nPauseOnReset &= ~ra::data::models::LeaderboardModel::LeaderboardParts::Cancel;
            if (pLeaderboard->submit.has_hits)
                nPauseOnReset &= ~ra::data::models::LeaderboardModel::LeaderboardParts::Submit;
            if (pLeaderboard->value.value.value)
                nPauseOnReset &= ~ra::data::models::LeaderboardModel::LeaderboardParts::Value;

            if (nPauseOnReset != ra::data::models::LeaderboardModel::LeaderboardParts::None)
            {
                auto& pFrameEventQueue = ra::services::ServiceLocator::GetMutable<ra::services::FrameEventQueue>();

                if ((nPauseOnReset & ra::data::models::LeaderboardModel::LeaderboardParts::Start) !=
                    ra::data::models::LeaderboardModel::LeaderboardParts::None)
                {
                    pFrameEventQueue.QueuePauseOnReset(ra::StringPrintf(L"Start: %s", pair.first->public_.title));
                }

                if ((nPauseOnReset & ra::data::models::LeaderboardModel::LeaderboardParts::Cancel) !=
                    ra::data::models::LeaderboardModel::LeaderboardParts::None)
                {
                    pFrameEventQueue.QueuePauseOnReset(ra::StringPrintf(L"Cancel: %s", pair.first->public_.title));
                }

                if ((nPauseOnReset & ra::data::models::LeaderboardModel::LeaderboardParts::Submit) !=
                    ra::data::models::LeaderboardModel::LeaderboardParts::None)
                {
                    pFrameEventQueue.QueuePauseOnReset(ra::StringPrintf(L"Submit: %s", pair.first->public_.title));
                }

                if ((nPauseOnReset & ra::data::models::LeaderboardModel::LeaderboardParts::Value) !=
                    ra::data::models::LeaderboardModel::LeaderboardParts::None)
                {
                    pFrameEventQueue.QueuePauseOnReset(ra::StringPrintf(L"Value: %s", pair.first->public_.title));
                }
            }
        }
    }

    for (auto pair : mActiveLeaderboards)
    {
        if (pair.first && pair.first->lboard)
        {
            using namespace ra::bitwise_ops;

            auto nPauseOnTrigger = pair.second;
            const auto* pLeaderboard = pair.first->lboard;
            if (pLeaderboard->start.state != RC_TRIGGER_STATE_TRIGGERED)
                nPauseOnTrigger &= ~ra::data::models::LeaderboardModel::LeaderboardParts::Start;
            if (pLeaderboard->cancel.state != RC_TRIGGER_STATE_TRIGGERED)
                nPauseOnTrigger &= ~ra::data::models::LeaderboardModel::LeaderboardParts::Cancel;
            if (pLeaderboard->submit.state != RC_TRIGGER_STATE_TRIGGERED)
                nPauseOnTrigger &= ~ra::data::models::LeaderboardModel::LeaderboardParts::Submit;

            if (nPauseOnTrigger != ra::data::models::LeaderboardModel::LeaderboardParts::None)
            {
                auto& pFrameEventQueue = ra::services::ServiceLocator::GetMutable<ra::services::FrameEventQueue>();

                if ((nPauseOnTrigger & ra::data::models::LeaderboardModel::LeaderboardParts::Start) !=
                    ra::data::models::LeaderboardModel::LeaderboardParts::None)
                {
                    pFrameEventQueue.QueuePauseOnTrigger(ra::StringPrintf(L"Start: %s", pair.first->public_.title));
                }

                if ((nPauseOnTrigger & ra::data::models::LeaderboardModel::LeaderboardParts::Cancel) !=
                    ra::data::models::LeaderboardModel::LeaderboardParts::None)
                {
                    pFrameEventQueue.QueuePauseOnTrigger(ra::StringPrintf(L"Cancel: %s", pair.first->public_.title));
                }

                if ((nPauseOnTrigger & ra::data::models::LeaderboardModel::LeaderboardParts::Submit) !=
                    ra::data::models::LeaderboardModel::LeaderboardParts::None)
                {
                    pFrameEventQueue.QueuePauseOnTrigger(ra::StringPrintf(L"Submit: %s", pair.first->public_.title));
                }
            }
        }
    }
}

void AchievementRuntime::DoFrame()
{
    if (m_bPaused)
    {
        rc_client_idle(GetClient());
        return;
    }

    std::vector<rc_client_achievement_info_t*> vAchievementsWithHits;
    std::vector<rc_client_achievement_info_t*> vActiveAchievements;
    std::map<rc_client_leaderboard_info_t*, ra::data::models::LeaderboardModel::LeaderboardParts> mLeaderboardsWithHits;
    std::map<rc_client_leaderboard_info_t*, ra::data::models::LeaderboardModel::LeaderboardParts> mActiveLeaderboards;

    PrepareForPauseOnChangeEvents(GetClient(), vAchievementsWithHits, vActiveAchievements, mLeaderboardsWithHits, mActiveLeaderboards);

    rc_client_do_frame(GetClient());

    if (!vAchievementsWithHits.empty() || !vActiveAchievements.empty())
        RaisePauseOnChangeEvents(vAchievementsWithHits, vActiveAchievements);
    if (!mLeaderboardsWithHits.empty() || !mActiveLeaderboards.empty())
        RaisePauseOnChangeEvents(mLeaderboardsWithHits, mActiveLeaderboards);
}

void AchievementRuntime::Idle() noexcept
{
    rc_client_idle(GetClient());
}

void AchievementRuntime::InvalidateAddress(ra::ByteAddress nAddress) noexcept
{
    // this should only be called from DetectUnsupportedAchievements or indirectly via Process,
    // both of which aquire the lock, so we shouldn't try to acquire it here.
    // we can also avoid checking m_bInitialized
    rc_runtime_invalidate_address(&GetClient()->game->runtime, nAddress);
}

static void HandleAchievementTriggeredEvent(const rc_client_achievement_t& pAchievement)
{
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    auto* vmAchievement = pGameContext.Assets().FindAchievement(pAchievement.id);
    if (!vmAchievement)
    {
        RA_LOG_ERR("Received achievement triggered event for unknown achievement %u", pAchievement.id);
        return;
    }

    // immediately set the state to Triggered (instead of waiting for AssetListViewModel::DoFrame to do it).
    vmAchievement->SetState(ra::data::models::AssetState::Triggered);

    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    bool bTakeScreenshot = pConfiguration.IsFeatureEnabled(ra::services::Feature::AchievementTriggeredScreenshot);
    bool bSubmit = false;
    bool bIsError = false;

    std::unique_ptr<ra::ui::viewmodels::PopupMessageViewModel> vmPopup(new ra::ui::viewmodels::PopupMessageViewModel);
    vmPopup->SetDescription(ra::StringPrintf(L"%s (%u)", pAchievement.title, pAchievement.points));
    vmPopup->SetDetail(ra::Widen(pAchievement.description));
    vmPopup->SetImage(ra::ui::ImageType::Badge, AchievementRuntime::GetAchievementBadge(pAchievement));
    vmPopup->SetPopupType(ra::ui::viewmodels::Popup::AchievementTriggered);

    switch (vmAchievement->GetCategory())
    {
        case ra::data::models::AssetCategory::Local:
            vmPopup->SetTitle(L"Local Achievement Unlocked");
            bSubmit = false;
            bTakeScreenshot = false;
            break;

        case ra::data::models::AssetCategory::Unofficial:
            vmPopup->SetTitle(L"Unofficial Achievement Unlocked");
            bSubmit = false;
            break;

        default:
            vmPopup->SetTitle(L"Achievement Unlocked");
            bSubmit = true;
            break;
    }

    if (vmAchievement->IsModified() ||                                                    // actual modifications
        (bSubmit && vmAchievement->GetChanges() != ra::data::models::AssetChanges::None)) // unpublished changes
    {
        auto sHeader = vmPopup->GetTitle();
        sHeader.insert(0, L"Modified ");
        if (vmAchievement->GetCategory() != ra::data::models::AssetCategory::Local)
            sHeader.append(L" LOCALLY");
        vmPopup->SetTitle(sHeader);

        RA_LOG_INFO("Achievement %u not unlocked - %s", pAchievement.id,
                    vmAchievement->IsModified() ? "modified" : "unpublished");
        bIsError = true;
        bSubmit = false;
        bTakeScreenshot = false;
    }

    if (bSubmit && pGameContext.GetMode() == ra::data::context::GameContext::Mode::CompatibilityTest)
    {
        auto sHeader = vmPopup->GetTitle();
        sHeader.insert(0, L"Test ");
        vmPopup->SetTitle(sHeader);

        RA_LOG_INFO("Achievement %u not unlocked - %s", pAchievement.id, "test compatibility mode");
        bSubmit = false;
    }

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    if (bSubmit && pEmulatorContext.WasMemoryModified())
    {
        vmPopup->SetTitle(L"Achievement Unlocked LOCALLY");
        vmPopup->SetErrorDetail(L"Error: RAM tampered with");

        RA_LOG_INFO("Achievement %u not unlocked - %s", pAchievement.id, "RAM tampered with");
        bIsError = true;
        bSubmit = false;
    }

    if (bSubmit && _RA_HardcoreModeIsActive() && pEmulatorContext.IsMemoryInsecure())
    {
        vmPopup->SetTitle(L"Achievement Unlocked LOCALLY");
        vmPopup->SetErrorDetail(L"Error: RAM insecure");

        RA_LOG_INFO("Achievement %u not unlocked - %s", pAchievement.id, "RAM insecure");
        bIsError = true;
        bSubmit = false;
    }

    if (bSubmit && pConfiguration.IsFeatureEnabled(ra::services::Feature::Offline))
    {
        vmPopup->SetTitle(L"Offline Achievement Unlocked");
        bSubmit = false;
    }

#ifndef NDEBUG
    if (!bSubmit)
        Expects(!CanSubmitAchievementUnlock(pAchievement.id, nullptr));
#endif

    ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(bIsError ? L"Overlay\\acherror.wav"
                                                                                           : L"Overlay\\unlock.wav");

    if (pConfiguration.GetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered) !=
        ra::ui::viewmodels::PopupLocation::None)
    {
        auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
        const auto nPopupId = pOverlayManager.QueueMessage(vmPopup);

        if (bTakeScreenshot)
        {
            std::wstring sPath =
                ra::StringPrintf(L"%s%u.png", pConfiguration.GetScreenshotDirectory(), pAchievement.id);
            pOverlayManager.CaptureScreenshot(nPopupId, sPath);
        }

        s_mAchievementPopups[pAchievement.id] = nPopupId;
    }
}

static void HandleChallengeIndicatorShowEvent(const rc_client_achievement_t& pAchievement)
{
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    const auto* vmAchievement = pGameContext.Assets().FindAchievement(pAchievement.id);
    if (!vmAchievement)
    {
        RA_LOG_ERR("Received achievement triggered event for unknown achievement %u", pAchievement.id);
        return;
    }

    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (pConfiguration.GetPopupLocation(ra::ui::viewmodels::Popup::Challenge) !=
        ra::ui::viewmodels::PopupLocation::None)
    {
        auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
        pOverlayManager.AddChallengeIndicator(vmAchievement->GetID(), ra::ui::ImageType::Badge,
                                              ra::Narrow(vmAchievement->GetBadge()));
    }
}

static void HandleChallengeIndicatorHideEvent(const rc_client_achievement_t& pAchievement)
{
    auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
    pOverlayManager.RemoveChallengeIndicator(pAchievement.id);
}

static void HandleProgressIndicatorUpdateEvent(const rc_client_achievement_t& pAchievement)
{
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    const auto* vmAchievement = pGameContext.Assets().FindAchievement(pAchievement.id);
    if (!vmAchievement)
    {
        RA_LOG_ERR("Received achievement triggered event for unknown achievement %u", pAchievement.id);
        return;
    }

    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (pConfiguration.GetPopupLocation(ra::ui::viewmodels::Popup::Progress) != ra::ui::viewmodels::PopupLocation::None)
    {
        auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();

        // use locked badge if available
        auto sBadgeName = ra::Narrow(vmAchievement->GetBadge());
        if (!ra::StringStartsWith(sBadgeName, "local\\"))
            sBadgeName += "_lock";

        pOverlayManager.UpdateProgressTracker(ra::ui::ImageType::Badge, sBadgeName,
                                              ra::Widen(pAchievement.measured_progress));
    }
}

static void HandleProgressIndicatorShowEvent(const rc_client_achievement_t& pAchievement)
{
    HandleProgressIndicatorUpdateEvent(pAchievement);
}

static void HandleProgressIndicatorHideEvent()
{
    auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
    pOverlayManager.UpdateProgressTracker(ra::ui::ImageType::None, "", L"");
}

static void HandleGameCompletedEvent(const rc_client_t& pClient)
{
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (pConfiguration.GetPopupLocation(ra::ui::viewmodels::Popup::Mastery) == ra::ui::viewmodels::PopupLocation::None)
        return;

    const bool bHardcore = pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore);
    const auto* pGame = rc_client_get_game_info(&pClient);
    Expects(pGame != nullptr);

    rc_client_user_game_summary_t summary;
    rc_client_get_user_game_summary(&pClient, &summary);

    const auto nPlayTimeSeconds =
        ra::services::ServiceLocator::Get<ra::data::context::SessionTracker>().GetTotalPlaytime(pGame->id);
    const auto nPlayTimeMinutes = std::chrono::duration_cast<std::chrono::minutes>(nPlayTimeSeconds).count();

    std::unique_ptr<ra::ui::viewmodels::PopupMessageViewModel> vmMessage(new ra::ui::viewmodels::PopupMessageViewModel);
    vmMessage->SetTitle(ra::StringPrintf(L"%s %s", bHardcore ? L"Mastered" : L"Completed", pGame->title));
    vmMessage->SetDescription(ra::StringPrintf(L"%u achievements, %u points", summary.num_core_achievements, summary.points_core));
    vmMessage->SetDetail(ra::StringPrintf(L"%s | Play time: %dh%02dm", rc_client_get_user_info(&pClient)->display_name,
                                            nPlayTimeMinutes / 60, nPlayTimeMinutes % 60));
    vmMessage->SetImage(ra::ui::ImageType::Icon, pGame->badge_name);
    vmMessage->SetPopupType(ra::ui::viewmodels::Popup::Mastery);

    ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\unlock.wav");
    auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
    const auto nPopupId = pOverlayManager.QueueMessage(vmMessage);

    if (pConfiguration.IsFeatureEnabled(ra::services::Feature::MasteryNotificationScreenshot))
    {
        std::wstring sPath = ra::StringPrintf(L"%sGame%u.png", pConfiguration.GetScreenshotDirectory(), pGame->id);
        pOverlayManager.CaptureScreenshot(nPopupId, sPath);
    }
}

static void HandleLeaderboardStartedEvent(const rc_client_leaderboard_t& pLeaderboard)
{
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    const auto* vmLeaderboard = pGameContext.Assets().FindLeaderboard(pLeaderboard.id);
    if (!vmLeaderboard)
    {
        RA_LOG_ERR("Received leaderboard started event for unknown leaderboard %u", pLeaderboard.id);
        return;
    }

    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (pConfiguration.GetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardStarted) !=
        ra::ui::viewmodels::PopupLocation::None)
    {
        ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\lb.wav");
        auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
        pOverlayManager.QueueMessage(ra::ui::viewmodels::Popup::LeaderboardStarted, L"Leaderboard attempt started",
                                     ra::Widen(pLeaderboard.title), ra::Widen(pLeaderboard.description));
    }
}

static void HandleLeaderboardFailedEvent(const rc_client_leaderboard_t& pLeaderboard)
{
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    const auto* vmLeaderboard = pGameContext.Assets().FindLeaderboard(pLeaderboard.id);
    if (!vmLeaderboard)
    {
        RA_LOG_ERR("Received leaderboard started event for unknown leaderboard %u", pLeaderboard.id);
        return;
    }

    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (pConfiguration.GetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardCanceled) !=
        ra::ui::viewmodels::PopupLocation::None)
    {
        ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\lbcancel.wav");
        auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
        pOverlayManager.QueueMessage(ra::ui::viewmodels::Popup::LeaderboardCanceled, L"Leaderboard attempt failed",
                                     ra::Widen(pLeaderboard.title), ra::Widen(pLeaderboard.description));
    }
}

static void ShowSimplifiedScoreboard(const rc_client_leaderboard_t& pLeaderboard)
{
    auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (pConfiguration.GetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard) ==
        ra::ui::viewmodels::PopupLocation::None)
    {
        return;
    }

    ra::ui::viewmodels::ScoreboardViewModel vmScoreboard;
    vmScoreboard.SetHeaderText(ra::Widen(pLeaderboard.title));

    const auto& pUserName = ra::services::ServiceLocator::Get<ra::data::context::UserContext>().GetDisplayName();
    auto& pEntryViewModel = vmScoreboard.Entries().Add();
    pEntryViewModel.SetRank(0);
    pEntryViewModel.SetScore(ra::Widen(pLeaderboard.tracker_value));
    pEntryViewModel.SetUserName(ra::Widen(pUserName));
    pEntryViewModel.SetHighlighted(true);

    ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueScoreboard(
        pLeaderboard.id, std::move(vmScoreboard));
}

static void HandleLeaderboardSubmittedEvent(const rc_client_leaderboard_t& pLeaderboard)
{
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    const auto* vmLeaderboard = pGameContext.Assets().FindLeaderboard(pLeaderboard.id);
    if (!vmLeaderboard)
    {
        RA_LOG_ERR("Received leaderboard started event for unknown leaderboard %u", pLeaderboard.id);
        return;
    }

    std::unique_ptr<ra::ui::viewmodels::PopupMessageViewModel> vmPopup(new ra::ui::viewmodels::PopupMessageViewModel);
    vmPopup->SetDescription(ra::Widen(pLeaderboard.title));
    std::wstring sTitle = L"Leaderboard Submitted";
    bool bSubmit = true;

    switch (vmLeaderboard->GetCategory())
    {
        case ra::data::models::AssetCategory::Local:
            sTitle.insert(0, L"Local ");
            vmPopup->SetDetail(L"Local leaderboards are not submitted.");
            bSubmit = false;
            break;

        case ra::data::models::AssetCategory::Unofficial:
            sTitle.insert(0, L"Unofficial ");
            vmPopup->SetDetail(L"Unofficial leaderboards are not submitted.");
            bSubmit = false;
            break;

        default:
            break;
    }

    if (vmLeaderboard->IsModified() ||                                                    // actual modifications
        (bSubmit && vmLeaderboard->GetChanges() != ra::data::models::AssetChanges::None)) // unpublished changes
    {
        sTitle.insert(0, L"Modified ");
        vmPopup->SetDetail(L"Modified leaderboards are not submitted.");
        bSubmit = false;
    }

    if (bSubmit && pGameContext.GetMode() == ra::data::context::GameContext::Mode::CompatibilityTest)
    {
        vmPopup->SetDetail(L"Leaderboards are not submitted in compatibility test mode.");
        bSubmit = false;
    }

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    if (bSubmit && pEmulatorContext.WasMemoryModified())
    {
        vmPopup->SetErrorDetail(L"Error: RAM tampered with");
        bSubmit = false;
    }

    if (bSubmit)
    {
        const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
        if (!pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
        {
            vmPopup->SetErrorDetail(L"Submission requires Hardcore mode");
            bSubmit = false;
        }
        else if (pEmulatorContext.IsMemoryInsecure())
        {
            vmPopup->SetErrorDetail(L"Error: RAM insecure");
            bSubmit = false;
        }
        else if (pConfiguration.IsFeatureEnabled(ra::services::Feature::Offline))
        {
            vmPopup->SetDetail(L"Leaderboards are not submitted in offline mode.");
            bSubmit = false;
        }
    }

    if (!bSubmit)
    {
        vmPopup->SetTitle(sTitle);
        ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\info.wav");
        ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(vmPopup);

#ifndef NDEBUG
        Expects(!CanSubmitLeaderboardEntry(pLeaderboard.id, nullptr));
#endif

        ShowSimplifiedScoreboard(pLeaderboard);
        return;
    }

    // no popup shown here, the scoreboard will be shown instead
}

static void HandleLeaderboardTrackerUpdateEvent(const rc_client_leaderboard_tracker_t& pTracker)
{
    auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
    auto* pScoreTracker = pOverlayManager.GetScoreTracker(pTracker.id);
    if (pScoreTracker)
        pScoreTracker->SetDisplayText(ra::Widen(pTracker.display));
}

static void HandleLeaderboardTrackerShowEvent(const rc_client_leaderboard_tracker_t& pTracker)
{
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (pConfiguration.GetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardTracker) !=
        ra::ui::viewmodels::PopupLocation::None)
    {
        auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
        auto& pScoreTracker = pOverlayManager.AddScoreTracker(pTracker.id);
        pScoreTracker.SetDisplayText(ra::Widen(pTracker.display));
    }
}

static void HandleLeaderboardTrackerHideEvent(const rc_client_leaderboard_tracker_t& pTracker)
{
    auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
    pOverlayManager.RemoveScoreTracker(pTracker.id);
}

static void HandleLeaderboardScoreboardEvent(const rc_client_leaderboard_scoreboard_t& pScoreboard,
                                             const rc_client_leaderboard_t& pLeaderboard)
{
    auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (pConfiguration.GetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard) ==
        ra::ui::viewmodels::PopupLocation::None)
    {
        return;
    }

    ra::ui::viewmodels::ScoreboardViewModel vmScoreboard;
    vmScoreboard.SetHeaderText(ra::Widen(pLeaderboard.title));

    const auto& pUserName = ra::services::ServiceLocator::Get<ra::data::context::UserContext>().GetDisplayName();
    constexpr uint32_t nEntriesDisplayed = 7; // display is currently hard-coded to show 7 entries
    bool bSeenPlayer = false;

    const auto* pEntry = pScoreboard.top_entries;
    const auto* pStop = pEntry + std::min(pScoreboard.num_top_entries, nEntriesDisplayed);
    for (; pEntry < pStop; ++pEntry)
    {
        auto& pEntryViewModel = vmScoreboard.Entries().Add();
        pEntryViewModel.SetRank(pEntry->rank);
        pEntryViewModel.SetScore(ra::Widen(pEntry->score));
        pEntryViewModel.SetUserName(ra::Widen(pEntry->username));

        if (pEntry->username == pUserName)
        {
            bSeenPlayer = true;
            pEntryViewModel.SetHighlighted(true);

            if (strcmp(pScoreboard.best_score, pScoreboard.submitted_score) != 0)
                pEntryViewModel.SetScore(ra::StringPrintf(L"(%s) %s", pScoreboard.submitted_score, pScoreboard.best_score));
        }
    }

    if (!bSeenPlayer)
    {
        auto* pEntryViewModel = vmScoreboard.Entries().GetItemAt(6);
        if (pEntryViewModel != nullptr)
        {
            pEntryViewModel->SetRank(pScoreboard.new_rank);

            if (strcmp(pScoreboard.best_score, pScoreboard.submitted_score) != 0)
                pEntryViewModel->SetScore(
                    ra::StringPrintf(L"(%s) %s", pScoreboard.submitted_score, pScoreboard.best_score));
            else
                pEntryViewModel->SetScore(ra::Widen(pScoreboard.best_score));

            pEntryViewModel->SetUserName(ra::Widen(pUserName));
            pEntryViewModel->SetHighlighted(true);
        }
    }

    ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueScoreboard(
        pLeaderboard.id, std::move(vmScoreboard));
}

static void HandleServerError(const rc_client_server_error_t& pServerError)
{
    if (strcmp(pServerError.api, "award_achievement"))
    {
        const auto nAchievementId = pServerError.related_id;
        const auto nPopupId = s_mAchievementPopups[nAchievementId];

        const auto sErrorMessage =
            pServerError.error_message ? ra::Widen(pServerError.error_message) : L"Error submitting unlock";
        auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
        auto pPopup = pOverlayManager.GetMessage(nPopupId);
        if (pPopup != nullptr)
        {
            pPopup->SetTitle(L"Achievement Unlock FAILED");
            pPopup->SetErrorDetail(sErrorMessage);
            pPopup->RebuildRenderImage();
        }
        else
        {
            std::unique_ptr<ra::ui::viewmodels::PopupMessageViewModel> vmPopup(
                new ra::ui::viewmodels::PopupMessageViewModel);
            vmPopup->SetTitle(L"Achievement Unlock FAILED");
            vmPopup->SetErrorDetail(sErrorMessage);
            vmPopup->SetPopupType(ra::ui::viewmodels::Popup::AchievementTriggered);

            const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
            const auto* pAchievement = pGameContext.Assets().FindAchievement(nAchievementId);
            if (pAchievement != nullptr)
            {
                vmPopup->SetDescription(
                    ra::StringPrintf(L"%s (%u)", pAchievement->GetName(), pAchievement->GetPoints()));
                vmPopup->SetImage(ra::ui::ImageType::Badge, ra::Narrow(pAchievement->GetBadge()));
            }
            else
            {
                vmPopup->SetDescription(ra::StringPrintf(L"Achievement %u", nAchievementId));
            }

            ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\acherror.wav");
            pOverlayManager.QueueMessage(vmPopup);
        }

        return;
    }

    if (strcmp(pServerError.api, "submit_lboard_entry"))
    {}

    ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(
        ra::StringPrintf(L"%s:%u %s", pServerError.api, pServerError.related_id, pServerError.error_message));
}

static void HandleResetEvent()
{
    auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    pEmulatorContext.Reset();
}

GSL_SUPPRESS_CON3
void AchievementRuntime::EventHandler(const rc_client_event_t* pEvent, rc_client_t* pClient)
{
    Expects(pClient != nullptr);
    Expects(pEvent != nullptr);
    switch (pEvent->type)
    {
        case RC_CLIENT_EVENT_LEADERBOARD_TRACKER_UPDATE:
            HandleLeaderboardTrackerUpdateEvent(*pEvent->leaderboard_tracker);
            break;

        case RC_CLIENT_EVENT_LEADERBOARD_TRACKER_SHOW:
            HandleLeaderboardTrackerShowEvent(*pEvent->leaderboard_tracker);
            break;

        case RC_CLIENT_EVENT_LEADERBOARD_TRACKER_HIDE:
            HandleLeaderboardTrackerHideEvent(*pEvent->leaderboard_tracker);
            break;

        case RC_CLIENT_EVENT_ACHIEVEMENT_CHALLENGE_INDICATOR_SHOW:
            HandleChallengeIndicatorShowEvent(*pEvent->achievement);
            break;

        case RC_CLIENT_EVENT_ACHIEVEMENT_CHALLENGE_INDICATOR_HIDE:
            HandleChallengeIndicatorHideEvent(*pEvent->achievement);
            break;

        case RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_SHOW:
            HandleProgressIndicatorShowEvent(*pEvent->achievement);
            break;

        case RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_HIDE:
            HandleProgressIndicatorHideEvent();
            break;

        case RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_UPDATE:
            HandleProgressIndicatorUpdateEvent(*pEvent->achievement);
            break;

        case RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED:
            HandleAchievementTriggeredEvent(*pEvent->achievement);
            break;

        case RC_CLIENT_EVENT_LEADERBOARD_STARTED:
            HandleLeaderboardStartedEvent(*pEvent->leaderboard);
            break;

        case RC_CLIENT_EVENT_LEADERBOARD_FAILED:
            HandleLeaderboardFailedEvent(*pEvent->leaderboard);
            break;

        case RC_CLIENT_EVENT_LEADERBOARD_SUBMITTED:
            HandleLeaderboardSubmittedEvent(*pEvent->leaderboard);
            break;

        case RC_CLIENT_EVENT_LEADERBOARD_SCOREBOARD:
            HandleLeaderboardScoreboardEvent(*pEvent->leaderboard_scoreboard, *pEvent->leaderboard);
            break;

        case RC_CLIENT_EVENT_GAME_COMPLETED:
            HandleGameCompletedEvent(*pClient);
            break;

        case RC_CLIENT_EVENT_RESET:
            HandleResetEvent();
            break;

        case RC_CLIENT_EVENT_SERVER_ERROR:
            HandleServerError(*pEvent->server_error);
            break;

        default:
            RA_LOG_WARN("Unhandled rc_client_event %u", pEvent->type);
            break;
    }
}

/* ---- Runtime State ----- */

void AchievementRuntime::ResetRuntime() noexcept
{
    rc_client_reset(GetClient());
}

static void ProcessStateString(Tokenizer& pTokenizer, unsigned int nId, rc_trigger_t* pTrigger,
                               const std::string& sSalt, const std::string& sMemString)
{
    struct ConditionState
    {
        unsigned int nHits;
        unsigned int nSourceVal;
        unsigned int nSourcePrev;
        unsigned int nTargetVal;
        unsigned int nTargetPrev;
    };

    std::vector<ConditionState> vConditions;
    const size_t nStart = pTokenizer.CurrentPosition();

    // each group appears as an entry for the current nId
    while (!pTokenizer.EndOfString())
    {
        const size_t nPosition = pTokenizer.CurrentPosition();
        const unsigned int nGroupId = pTokenizer.ReadNumber();
        if (nGroupId != nId || pTokenizer.PeekChar() != ':')
        {
            pTokenizer.Seek(nPosition);
            break;
        }
        pTokenizer.Advance();

        const unsigned int nNumCond = pTokenizer.ReadNumber();
        pTokenizer.Advance();
        vConditions.reserve(vConditions.size() + nNumCond);

        for (size_t i = 0; i < nNumCond; ++i)
        {
            // Parse next condition state
            auto& cond = vConditions.emplace_back();
            cond.nHits = pTokenizer.ReadNumber();
            pTokenizer.Advance();
            cond.nSourceVal = pTokenizer.ReadNumber();
            pTokenizer.Advance();
            cond.nSourcePrev = pTokenizer.ReadNumber();
            pTokenizer.Advance();
            cond.nTargetVal = pTokenizer.ReadNumber();
            pTokenizer.Advance();
            cond.nTargetPrev = pTokenizer.ReadNumber();
            pTokenizer.Advance();
        }
    }

    const size_t nEnd = pTokenizer.CurrentPosition();

    // read the given md5s
    std::string sGivenMD5Progress = pTokenizer.ReadTo(':');
    pTokenizer.Advance();
    std::string sGivenMD5Achievement = pTokenizer.ReadTo(':');
    pTokenizer.Advance();

    if (!pTrigger)
    {
        // achievement wasn't found, ignore it. still had to parse to advance pIter
    }
    else
    {
        // recalculate the current achievement checksum
        std::string sMD5Achievement = RAGenerateMD5(sMemString);
        if (sGivenMD5Achievement != sMD5Achievement)
        {
            // achievement has been modified since data was captured, can't apply, just reset
            rc_reset_trigger(pTrigger);
        }
        else
        {
            // regenerate the md5 and see if it sticks
            const char* pStart = pTokenizer.GetPointer(nStart);
            std::string sModifiedProgressString =
                ra::StringPrintf("%s%.*s%s%u", sSalt, nEnd - nStart, pStart, sSalt, nId);
            std::string sMD5Progress = RAGenerateMD5(sModifiedProgressString);
            if (sMD5Progress != sGivenMD5Progress)
            {
                // state checksum fail - assume user tried to modify it and ignore, just reset
                rc_reset_trigger(pTrigger);
            }
            else
            {
                // compatible - merge
                size_t nCondition = 0;

                rc_condset_t* pGroup = pTrigger->requirement;
                while (pGroup != nullptr)
                {
                    rc_condition_t* pCondition = pGroup->conditions;
                    while (pCondition != nullptr)
                    {
                        const auto& condSource = vConditions.at(nCondition++);
                        pCondition->current_hits = condSource.nHits;
                        if (rc_operand_is_memref(&pCondition->operand1))
                        {
                            pCondition->operand1.value.memref->value.value = condSource.nSourceVal;
                            pCondition->operand1.value.memref->value.prior = condSource.nSourcePrev;
                            pCondition->operand1.value.memref->value.changed = 0;
                        }
                        if (rc_operand_is_memref(&pCondition->operand2))
                        {
                            pCondition->operand2.value.memref->value.value = condSource.nTargetVal;
                            pCondition->operand2.value.memref->value.prior = condSource.nTargetPrev;
                            pCondition->operand2.value.memref->value.changed = 0;
                        }

                        pCondition = pCondition->next;
                    }

                    if (pGroup == pTrigger->requirement)
                        pGroup = pTrigger->alternative;
                    else
                        pGroup = pGroup->next;
                }
            }
        }
    }
}

static bool LoadProgressV1(rc_client_t* pClient, const std::string& sProgress)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();

    Tokenizer pTokenizer(sProgress);

    while (!pTokenizer.EndOfString())
    {
        const unsigned int nId = pTokenizer.PeekNumber();

        auto* pAchievement = GetAchievementInfo(pClient, nId);
        if (pAchievement == nullptr)
        {
            // achievement not active, still have to process state string to skip over it
            std::string sEmpty;
            ProcessStateString(pTokenizer, nId, nullptr, sEmpty, sEmpty);
        }
        else
        {
            const auto* vmAchievement = pGameContext.Assets().FindAchievement(nId);
            std::string sMemString = vmAchievement ? vmAchievement->GetTrigger() : "";
            ProcessStateString(pTokenizer, nId, pAchievement->trigger, pUserContext.GetUsername(), sMemString);
            pAchievement->trigger->state = RC_TRIGGER_STATE_ACTIVE;
            pAchievement->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
        }
    }

    return true;
}

_NODISCARD static char ComparisonSizeFromPrefix(_In_ char cPrefix) noexcept
{
    char buffer[] = "0xH0";
    const char* ptr = buffer;
    buffer[2] = cPrefix;

    uint8_t size = RC_MEMSIZE_16_BITS;
    unsigned address = 0;
    rc_parse_memref(&ptr, &size, &address);
    return size;
}

static bool LoadProgressV2(rc_client_t* pClient, ra::services::TextReader& pFile)
{
    auto& pRuntime = pClient->game->runtime;

    std::string sLine;
    while (pFile.GetLine(sLine))
    {
        if (sLine.empty())
            continue;

        Tokenizer tokenizer(sLine);
        const auto c = tokenizer.PeekChar();
        tokenizer.Advance();

        if (c == '$') // memory reference
        {
            rc_memref_t pMemRef{};

            const auto cPrefix = tokenizer.PeekChar();
            tokenizer.Advance();
            pMemRef.value.size = ComparisonSizeFromPrefix(cPrefix);

            const auto sAddr = tokenizer.ReadTo(':');
            pMemRef.address = ra::ByteAddressFromString(sAddr);

            unsigned delta = 0;
            while (tokenizer.PeekChar() != '#' && !tokenizer.EndOfString())
            {
                tokenizer.Advance();
                const auto cField = tokenizer.PeekChar();
                tokenizer.Advance(2);
                const auto nValue = tokenizer.ReadNumber();

                switch (cField)
                {
                    case 'v':
                        pMemRef.value.value = nValue;
                        break;
                    case 'd':
                        delta = nValue;
                        break;
                    case 'p':
                        pMemRef.value.prior = nValue;
                        break;
                }
            }
            pMemRef.value.changed = (pMemRef.value.value != delta);

            if (tokenizer.PeekChar() == '#')
            {
                const auto sLineMD5 = RAGenerateMD5(sLine.substr(0, tokenizer.CurrentPosition()));
                tokenizer.Advance();
                if (tokenizer.PeekChar() == sLineMD5.at(0))
                {
                    tokenizer.Advance();
                    if (tokenizer.PeekChar() == sLineMD5.at(31))
                    {
                        // match! attempt to store it
                        rc_memref_t* pMemoryReference = pRuntime.memrefs;
                        while (pMemoryReference)
                        {
                            if (pMemoryReference->address == pMemRef.address &&
                                pMemoryReference->value.size == pMemRef.value.size)
                            {
                                pMemoryReference->value.value = pMemRef.value.value;
                                pMemoryReference->value.changed = pMemRef.value.changed;
                                pMemoryReference->value.prior = pMemRef.value.prior;
                                break;
                            }

                            pMemoryReference = pMemoryReference->next;
                        }
                    }
                }
            }
        }
        else if (c == 'A') // achievement
        {
            const auto nId = tokenizer.ReadNumber();
            tokenizer.Advance();

            rc_runtime_trigger_t* pRuntimeTrigger = nullptr;
            for (size_t i = 0; i < pRuntime.trigger_count; ++i)
            {
                if (pRuntime.triggers[i].id == nId && pRuntime.triggers[i].trigger)
                {
                    pRuntimeTrigger = &pRuntime.triggers[i];
                    break;
                }
            }

            if (pRuntimeTrigger == nullptr) // not active, ignore
                continue;
            auto* pTrigger = pRuntimeTrigger->trigger;

            const auto sChecksumMD5 = sLine.substr(tokenizer.CurrentPosition(), 32);
            tokenizer.Advance(32);

            const auto sMemStringMD5 = RAFormatMD5(pRuntimeTrigger->md5);
            if (sMemStringMD5 != sChecksumMD5) // modified since captured
            {
                rc_reset_trigger(pTrigger);
                continue;
            }

            pTrigger->has_hits = 0;

            rc_condition_t* pCondition = nullptr;
            if (pTrigger->requirement)
            {
                pCondition = pTrigger->requirement->conditions;
                while (pCondition)
                {
                    tokenizer.Advance();
                    pCondition->current_hits = tokenizer.ReadNumber();
                    pTrigger->has_hits |= (pCondition->current_hits != 0);

                    pCondition = pCondition->next;
                }
            }

            const rc_condset_t* pCondSet = pTrigger->alternative;
            while (pCondSet)
            {
                pCondition = pCondSet->conditions;
                while (pCondition)
                {
                    tokenizer.Advance();
                    pCondition->current_hits = tokenizer.ReadNumber();
                    pTrigger->has_hits |= (pCondition->current_hits != 0);

                    pCondition = pCondition->next;
                }

                pCondSet = pCondSet->next;
            }

            bool bValid = (tokenizer.PeekChar() == '#');
            if (bValid)
            {
                const auto sLineMD5 = RAGenerateMD5(sLine.substr(0, tokenizer.CurrentPosition()));
                tokenizer.Advance();
                if (tokenizer.PeekChar() != sLineMD5.at(0))
                {
                    bValid = false;
                }
                else
                {
                    tokenizer.Advance();
                    bValid = (tokenizer.PeekChar() == sLineMD5.at(31));
                }
            }

            if (bValid)
            {
                pTrigger->state = RC_TRIGGER_STATE_ACTIVE;
            }
            else
            {
                rc_reset_trigger(pTrigger);
            }
        }
    }

    return true;
}

bool AchievementRuntime::LoadProgressFromFile(const char* sLoadStateFilename)
{
    if (sLoadStateFilename == nullptr)
    {
        rc_client_deserialize_progress(GetClient(), nullptr);
        return false;
    }

    std::string sContents;

    std::wstring sAchievementStateFile = ra::Widen(sLoadStateFilename) + L".rap";
    const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
    auto pFile = pFileSystem.OpenTextFile(sAchievementStateFile);
    if (pFile == nullptr || !pFile->GetLine(sContents))
    {
        rc_client_deserialize_progress(GetClient(), nullptr);
        return false;
    }

    if (sContents == "RAP")
    {
        const auto nSize = pFile->GetSize();
        std::vector<unsigned char> pBuffer;
        pBuffer.resize(nSize);
        pFile->SetPosition({0});
        pFile->GetBytes(&pBuffer.front(), nSize);

        if (rc_client_deserialize_progress(GetClient(), &pBuffer.front()) == RC_OK)
        {
            RA_LOG_INFO("Runtime state loaded from %s", sLoadStateFilename);
        }
    }
    else if (sContents == "v2")
    {
        if (LoadProgressV2(GetClient(), *pFile))
        {
            RA_LOG_INFO("Runtime state (v2) loaded from %s", sLoadStateFilename);
        }
    }
    else
    {
        if (LoadProgressV1(GetClient(), sContents))
        {
            RA_LOG_INFO("Runtime state (v1) loaded from %s", sLoadStateFilename);
        }
    }

    return true;
}

bool AchievementRuntime::LoadProgressFromBuffer(const uint8_t* pBuffer)
{
    if (rc_client_deserialize_progress(GetClient(), pBuffer) == RC_OK)
    {
        RA_LOG_INFO("Runtime state loaded from buffer");
    }

    return true;
}

void AchievementRuntime::SaveProgressToFile(const char* sSaveStateFilename) const
{
    if (sSaveStateFilename == nullptr)
        return;

    std::wstring sAchievementStateFile = ra::Widen(sSaveStateFilename) + L".rap";
    auto pFile = ra::services::ServiceLocator::Get<ra::services::IFileSystem>().CreateTextFile(sAchievementStateFile);
    if (pFile == nullptr)
    {
        RA_LOG_WARN("Could not write %s", sAchievementStateFile);
        return;
    }

    const auto nSize = rc_client_progress_size(GetClient());
    std::string sSerialized;
    sSerialized.resize(nSize);
    GSL_SUPPRESS_TYPE1 const auto pData = reinterpret_cast<uint8_t*>(sSerialized.data());
    rc_client_serialize_progress(GetClient(), pData);
    pFile->Write(sSerialized);

    RA_LOG_INFO("Runtime state written to %s", sSaveStateFilename);
}

int AchievementRuntime::SaveProgressToBuffer(uint8_t* pBuffer, int nBufferSize) const
{
    const int nSize = gsl::narrow_cast<int>(rc_client_progress_size(GetClient()));
    if (nSize <= nBufferSize)
    {
        rc_client_serialize_progress(GetClient(), pBuffer);
        RA_LOG_INFO("Runtime state written to buffer (%d/%d bytes)", nSize, nBufferSize);
    }
    else if (nBufferSize > 0) // 0 size buffer indicates caller is asking for size, don't log - we'll capture the actual save soon.
    {
        RA_LOG_WARN("Runtime state not written to buffer (%d/%d bytes)", nSize, nBufferSize);
    }

    return nSize;
}

/* ---- Exports ----- */

#ifdef RC_CLIENT_EXPORTS_EXTERNAL

class AchievementRuntimeExports : private AchievementRuntime
{
public:
    static void destroy() noexcept
    {
        memset(&s_callbacks, 0, sizeof(s_callbacks));
    }

    static void enable_logging(rc_client_t* client, int level, rc_client_message_callback_t callback)
    {
        s_callbacks.log_callback = callback;
        s_callbacks.log_client = client;

        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        rc_client_enable_logging(pClient.GetClient(), level, AchievementRuntimeExports::LogMessageExternal);
    }

    static void set_event_handler(rc_client_t* client, rc_client_event_handler_t handler)
    {
        s_callbacks.event_handler = handler;
        s_callbacks.event_client = client;

        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        rc_client_set_event_handler(pClient.GetClient(), AchievementRuntimeExports::EventHandlerExternal);
    }

    static void set_read_memory(rc_client_t* client, rc_client_read_memory_func_t handler)
    {
        s_callbacks.read_memory_handler = handler;
        s_callbacks.read_memory_client = client;

        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        rc_client_set_read_memory_function(pClient.GetClient(), AchievementRuntimeExports::ReadMemoryExternal);

        auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
        pEmulatorContext.ClearMemoryBlocks();
        pEmulatorContext.AddMemoryBlock(0, 0xFFFFFFFF, nullptr, nullptr);
        pEmulatorContext.AddMemoryBlockReader(0, AchievementRuntimeExports::ReadMemoryBlock);
    }

    static void set_get_time_millisecs(rc_client_t* client, rc_get_time_millisecs_func_t handler)
    {
        s_callbacks.get_time_millisecs_handler = handler;
        s_callbacks.get_time_millisecs_client = client;

        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        rc_client_set_get_time_millisecs_function(pClient.GetClient(), AchievementRuntimeExports::GetTimeMillisecsExternal);
    }

    static void set_host(const char* value)
    {
        auto* pConfiguration = dynamic_cast<ra::services::impl::JsonFileConfiguration*>(
            &ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>());
        if (pConfiguration != nullptr)
            pConfiguration->SetHost(value);
    }

    static void set_hardcore_enabled(int value)
    {
        auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
        if (value)
            pEmulatorContext.EnableHardcoreMode(false);
        else
            pEmulatorContext.DisableHardcoreMode();
    }

    static int get_hardcore_enabled()
    {
        const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
        return pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore);
    }

    static void set_unofficial_enabled(int value)
    {
        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        rc_client_set_unofficial_enabled(pClient.GetClient(), value);
    }

    static int get_unofficial_enabled()
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_get_unofficial_enabled(pClient.GetClient());
    }

    static void set_encore_mode_enabled(int value)
    {
        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        rc_client_set_encore_mode_enabled(pClient.GetClient(), value);
    }

    static int get_encore_mode_enabled()
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_get_encore_mode_enabled(pClient.GetClient());
    }

    static void set_spectator_mode_enabled(int value)
    {
        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        rc_client_set_spectator_mode_enabled(pClient.GetClient(), value);
    }

    static int get_spectator_mode_enabled()
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_get_spectator_mode_enabled(pClient.GetClient());
    }

    static void abort_async(rc_client_async_handle_t* handle)
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        rc_client_abort_async(pClient.GetClient(), handle);
    }

    static rc_client_async_handle_t* begin_login_with_password(rc_client_t* client, const char* username,
                                                               const char* password, rc_client_callback_t callback,
                                                               void* callback_userdata)
    {
        GSL_SUPPRESS_R3
        auto* pCallbackData = new CallbackWrapper(client, callback, callback_userdata);

        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        return pClient.BeginLoginWithPassword(username, password, pCallbackData);
    }

    static rc_client_async_handle_t* begin_login_with_token(rc_client_t* client, const char* username,
                                                            const char* token, rc_client_callback_t callback,
                                                            void* callback_userdata)
    {
        GSL_SUPPRESS_R3
        auto* pCallbackWrapper = new LoadGameCallbackWrapper(client, callback, callback_userdata);

        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        return pClient.BeginLoginWithToken(username, token, pCallbackWrapper);
    }

    static const rc_client_user_t* get_user_info()
    {
        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        return rc_client_get_user_info(pClient.GetClient());
    }

    static void logout()
    {
        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        return rc_client_logout(pClient.GetClient());
    }

    static rc_client_async_handle_t* begin_identify_and_load_game(rc_client_t* client, uint32_t console_id,
                                                                  const char* file_path, const uint8_t* data,
                                                                  size_t data_size, rc_client_callback_t callback,
                                                                  void* callback_userdata)
    {
        GSL_SUPPRESS_R3
        auto* pCallbackWrapper = new LoadGameCallbackWrapper(client, callback, callback_userdata);

        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        return pClient.BeginIdentifyAndLoadGame(console_id, file_path, data, data_size, pCallbackWrapper);
    }

    static rc_client_async_handle_t* begin_load_game(rc_client_t* client, const char* hash,
                                                     rc_client_callback_t callback, void* callback_userdata)
    {
        GSL_SUPPRESS_R3
        auto* pCallbackData = new CallbackWrapper(client, callback, callback_userdata);

        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        return pClient.BeginLoadGame(hash, 0, pCallbackData);
    }

    static void unload_game()
    {
        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        return rc_client_unload_game(pClient.GetClient());
    }

    static const rc_client_game_t* get_game_info()
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_get_game_info(pClient.GetClient());
    }

    static const rc_client_subset_t* get_subset_info(uint32_t subset_id)
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_get_subset_info(pClient.GetClient(), subset_id);
    }

    static void get_user_game_summary(rc_client_user_game_summary_t* summary)
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_get_user_game_summary(pClient.GetClient(), summary);
    }

    static rc_client_async_handle_t* begin_change_media(rc_client_t* client, const char* file_path,
                                                        const uint8_t* data, size_t data_size,
                                                        rc_client_callback_t callback, void* callback_userdata)
    {
        GSL_SUPPRESS_R3
        auto* pCallbackData = new CallbackWrapper(client, callback, callback_userdata);

        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        return pClient.BeginChangeMedia(file_path, data, data_size, pCallbackData);
    }

    static rc_client_achievement_list_info_t* create_achievement_list(int category, int grouping)
    {
        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        GSL_SUPPRESS_TYPE1
        auto* list = reinterpret_cast<rc_client_achievement_list_info_t*>(
            rc_client_create_achievement_list(pClient.GetClient(), category, grouping));
        list->destroy_func = destroy_achievement_list;
        return list;
    }

    static int has_achievements()
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_has_achievements(pClient.GetClient());
    }

    static const rc_client_achievement_t* get_achievement_info(uint32_t id)
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_get_achievement_info(pClient.GetClient(), id);
    }

    static rc_client_leaderboard_list_info_t* create_leaderboard_list(int grouping)
    {
        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        GSL_SUPPRESS_TYPE1
        auto* list = reinterpret_cast<rc_client_leaderboard_list_info_t*>(
            rc_client_create_leaderboard_list(pClient.GetClient(), grouping));
        list->destroy_func = destroy_leaderboard_list;
        return list;
    }

    static int has_leaderboards()
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_has_leaderboards(pClient.GetClient());
    }

    static const rc_client_leaderboard_t* get_leaderboard_info(uint32_t id)
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_get_leaderboard_info(pClient.GetClient(), id);
    }

private:
    class LeaderboardEntriesListCallbackWrapper
    {
    public:
        LeaderboardEntriesListCallbackWrapper(rc_client_t* client,
                                              rc_client_fetch_leaderboard_entries_callback_t callback,
                                              void* callback_userdata) noexcept :
            m_pClient(client), m_fCallback(callback), m_pCallbackUserdata(callback_userdata)
        {}

        void DoCallback(int nResult, rc_client_leaderboard_entry_list_t* pList, const char* sErrorMessage) noexcept
        {
            m_fCallback(nResult, sErrorMessage, pList, m_pClient, m_pCallbackUserdata);
        }

        static void Dispatch(int nResult, const char* sErrorMessage, rc_client_leaderboard_entry_list_t* pList,
                             rc_client_t*, void* pUserdata)
        {
            auto* wrapper = static_cast<LeaderboardEntriesListCallbackWrapper*>(pUserdata);
            Expects(wrapper != nullptr);

            if (pList)
            {
                GSL_SUPPRESS_TYPE1
                reinterpret_cast<rc_client_leaderboard_entry_list_info_t*>(pList)->destroy_func = destroy_leaderboard_entry_list;
            }

            wrapper->DoCallback(nResult, pList, sErrorMessage);

            delete wrapper;
        }

    private:
        rc_client_t* m_pClient;
        rc_client_fetch_leaderboard_entries_callback_t m_fCallback;
        void* m_pCallbackUserdata;
    };

public:
    static rc_client_async_handle_t* begin_fetch_leaderboard_entries(rc_client_t* client,
        uint32_t leaderboard_id, uint32_t first_entry, uint32_t count,
        rc_client_fetch_leaderboard_entries_callback_t callback, void* callback_userdata)
    {
        GSL_SUPPRESS_R3
        auto* pCallbackData = new LeaderboardEntriesListCallbackWrapper(client, callback, callback_userdata);

        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        return rc_client_begin_fetch_leaderboard_entries(pClient.GetClient(), leaderboard_id, first_entry, count,
                                                         LeaderboardEntriesListCallbackWrapper::Dispatch, pCallbackData);
    }

    static rc_client_async_handle_t* begin_fetch_leaderboard_entries_around_user(rc_client_t* client,
        uint32_t leaderboard_id, uint32_t count,
        rc_client_fetch_leaderboard_entries_callback_t callback, void* callback_userdata)
    {
        GSL_SUPPRESS_R3
        auto* pCallbackData = new LeaderboardEntriesListCallbackWrapper(client, callback, callback_userdata);

        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        return rc_client_begin_fetch_leaderboard_entries_around_user(pClient.GetClient(), leaderboard_id, count,
                                                                     LeaderboardEntriesListCallbackWrapper::Dispatch, pCallbackData);
    }

    static size_t get_rich_presence_message(char buffer[], size_t buffer_size)
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_get_rich_presence_message(pClient.GetClient(), buffer, buffer_size);
    }

    static int has_rich_presence()
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_has_rich_presence(pClient.GetClient());
    }

    static void do_frame() noexcept
    {
        _RA_DoAchievementsFrame();
    }

    static void idle()
    {
        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        return rc_client_idle(pClient.GetClient());
    }

    static int is_processing_required()
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_is_processing_required(pClient.GetClient());
    }

    static int can_pause(uint32_t* frames_remaining)
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_can_pause(pClient.GetClient(), frames_remaining);
    }

    static void reset() noexcept
    {
#ifndef RA_UTEST
        _RA_OnReset();
#endif
    }

    static size_t progress_size()
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_progress_size(pClient.GetClient());
    }

    static int serialize_progress(uint8_t* buffer)
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_serialize_progress(pClient.GetClient(), buffer);
    }

    static int deserialize_progress(const uint8_t* buffer)
    {
        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        return rc_client_deserialize_progress(pClient.GetClient(), buffer);
    }

private:
    typedef struct ExternalClientCallbacks
    {
        rc_client_message_callback_t log_callback;
        rc_client_t* log_client;

        rc_client_event_handler_t event_handler;
        rc_client_t* event_client;

        rc_client_read_memory_func_t read_memory_handler;
        rc_client_t* read_memory_client;

        rc_get_time_millisecs_func_t get_time_millisecs_handler;
        rc_client_t* get_time_millisecs_client;

    } ExternalClientCallbacks;

    static ExternalClientCallbacks s_callbacks;

    static void LogMessageExternal(const char* sMessage, const rc_client_t*)
    {
        const auto& pLogger = ra::services::ServiceLocator::Get<ra::services::ILogger>();
        if (pLogger.IsEnabled(ra::services::LogLevel::Info))
            pLogger.LogMessage(ra::services::LogLevel::Info, sMessage);

        if (s_callbacks.log_callback)
            s_callbacks.log_callback(sMessage, s_callbacks.log_client);
    }

    static void EventHandlerExternal(const rc_client_event_t* event, rc_client_t*) noexcept(false)
    {
        if (s_callbacks.event_handler)
            s_callbacks.event_handler(event, s_callbacks.event_client);
    }

    static uint32_t ReadMemoryBlock(uint32_t address, uint8_t* buffer, uint32_t num_bytes) noexcept(false)
    {
        if (s_callbacks.read_memory_handler)
            return s_callbacks.read_memory_handler(address, buffer, num_bytes, s_callbacks.read_memory_client);

        return 0;
    }

    static uint32_t ReadMemoryExternal(uint32_t address, uint8_t* buffer, uint32_t num_bytes, rc_client_t*)  noexcept(false)
    {
        if (s_callbacks.read_memory_handler)
            return s_callbacks.read_memory_handler(address, buffer, num_bytes, s_callbacks.read_memory_client);

        return 0;
    }

    static rc_clock_t GetTimeMillisecsExternal(const rc_client_t*) noexcept(false)
    {
        if (s_callbacks.get_time_millisecs_handler)
            return s_callbacks.get_time_millisecs_handler(s_callbacks.get_time_millisecs_client);

        return 0;
    }

    static void destroy_achievement_list(rc_client_achievement_list_info_t* list) noexcept
    {
        if (list)
            free(list);
    }

    static void destroy_leaderboard_list(rc_client_leaderboard_list_info_t* list) noexcept
    {
        if (list)
            free(list);
    }

    static void destroy_leaderboard_entry_list(rc_client_leaderboard_entry_list_info_t* list) noexcept
    {
        if (list)
            free(list);
    }
};

AchievementRuntimeExports::ExternalClientCallbacks AchievementRuntimeExports::s_callbacks{};

#endif RC_CLIENT_EXPORTS_EXTERNAL

} // namespace services
} // namespace ra

extern "C" unsigned int rc_peek_callback(unsigned int nAddress, unsigned int nBytes, _UNUSED void* pData)
{
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    switch (nBytes)
    {
        case 1:
            return pEmulatorContext.ReadMemoryByte(nAddress);
        case 2:
            return pEmulatorContext.ReadMemory(nAddress, MemSize::SixteenBit);
        case 4:
            return pEmulatorContext.ReadMemory(nAddress, MemSize::ThirtyTwoBit);
        default:
            return 0U;
    }
}

#ifdef RC_CLIENT_EXPORTS_EXTERNAL

#include "Exports.hh"
#include "rcheevos/src/rc_client_external.h"

#ifdef __cplusplus
extern "C" {
#endif

static void GetExternalClientV1(rc_client_external_t* pClientExternal)
{
    pClientExternal->destroy = ra::services::AchievementRuntimeExports::destroy;

    pClientExternal->enable_logging = ra::services::AchievementRuntimeExports::enable_logging;
    pClientExternal->set_event_handler = ra::services::AchievementRuntimeExports::set_event_handler;
    pClientExternal->set_read_memory = ra::services::AchievementRuntimeExports::set_read_memory;
    pClientExternal->set_get_time_millisecs = ra::services::AchievementRuntimeExports::set_get_time_millisecs;
    pClientExternal->set_host = ra::services::AchievementRuntimeExports::set_host;

    pClientExternal->set_hardcore_enabled = ra::services::AchievementRuntimeExports::set_hardcore_enabled;
    pClientExternal->get_hardcore_enabled = ra::services::AchievementRuntimeExports::get_hardcore_enabled;
    pClientExternal->set_unofficial_enabled = ra::services::AchievementRuntimeExports::set_unofficial_enabled;
    pClientExternal->get_unofficial_enabled = ra::services::AchievementRuntimeExports::get_unofficial_enabled;
    pClientExternal->set_encore_mode_enabled = ra::services::AchievementRuntimeExports::set_encore_mode_enabled;
    pClientExternal->get_encore_mode_enabled = ra::services::AchievementRuntimeExports::get_encore_mode_enabled;
    pClientExternal->set_spectator_mode_enabled = ra::services::AchievementRuntimeExports::set_spectator_mode_enabled;
    pClientExternal->get_spectator_mode_enabled = ra::services::AchievementRuntimeExports::get_spectator_mode_enabled;

    pClientExternal->abort_async = ra::services::AchievementRuntimeExports::abort_async;

    pClientExternal->begin_login_with_password = ra::services::AchievementRuntimeExports::begin_login_with_password;
    pClientExternal->begin_login_with_token = ra::services::AchievementRuntimeExports::begin_login_with_token;
    pClientExternal->logout = ra::services::AchievementRuntimeExports::logout;
    pClientExternal->get_user_info = ra::services::AchievementRuntimeExports::get_user_info;

    pClientExternal->begin_identify_and_load_game = ra::services::AchievementRuntimeExports::begin_identify_and_load_game;
    pClientExternal->begin_load_game = ra::services::AchievementRuntimeExports::begin_load_game;
    pClientExternal->get_game_info = ra::services::AchievementRuntimeExports::get_game_info;
    pClientExternal->get_subset_info = ra::services::AchievementRuntimeExports::get_subset_info;
    pClientExternal->unload_game = ra::services::AchievementRuntimeExports::unload_game;
    pClientExternal->get_user_game_summary = ra::services::AchievementRuntimeExports::get_user_game_summary;
    pClientExternal->begin_change_media = ra::services::AchievementRuntimeExports::begin_change_media;

    pClientExternal->create_achievement_list = ra::services::AchievementRuntimeExports::create_achievement_list;
    pClientExternal->has_achievements = ra::services::AchievementRuntimeExports::has_achievements;
    pClientExternal->get_achievement_info = ra::services::AchievementRuntimeExports::get_achievement_info;

    pClientExternal->create_leaderboard_list = ra::services::AchievementRuntimeExports::create_leaderboard_list;
    pClientExternal->has_leaderboards = ra::services::AchievementRuntimeExports::has_leaderboards;
    pClientExternal->get_leaderboard_info = ra::services::AchievementRuntimeExports::get_leaderboard_info;
    pClientExternal->begin_fetch_leaderboard_entries = ra::services::AchievementRuntimeExports::begin_fetch_leaderboard_entries;
    pClientExternal->begin_fetch_leaderboard_entries_around_user =
        ra::services::AchievementRuntimeExports::begin_fetch_leaderboard_entries_around_user;

    pClientExternal->get_rich_presence_message = ra::services::AchievementRuntimeExports::get_rich_presence_message;
    pClientExternal->has_rich_presence = ra::services::AchievementRuntimeExports::has_rich_presence;

    pClientExternal->do_frame = ra::services::AchievementRuntimeExports::do_frame;
    pClientExternal->idle = ra::services::AchievementRuntimeExports::idle;
    pClientExternal->is_processing_required = ra::services::AchievementRuntimeExports::is_processing_required;
    pClientExternal->can_pause = ra::services::AchievementRuntimeExports::can_pause;
    pClientExternal->reset = ra::services::AchievementRuntimeExports::reset;

    pClientExternal->progress_size = ra::services::AchievementRuntimeExports::progress_size;
    pClientExternal->serialize_progress = ra::services::AchievementRuntimeExports::serialize_progress;
    pClientExternal->deserialize_progress = ra::services::AchievementRuntimeExports::deserialize_progress;
}

API int CCONV _Rcheevos_GetExternalClient(rc_client_external_t* pClientExternal, int nVersion)
{
    switch (nVersion)
    {
        default:
            RA_LOG_WARN("Unknown rc_client_external interface version: %s", nVersion);
            __fallthrough;

        case 1:
            GetExternalClientV1(pClientExternal);
            break;
    }

    return 1;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* RC_CLIENT_EXPORTS_EXTERNAL */
