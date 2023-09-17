#include "RcheevosClient.hh"

#include "data/context/EmulatorContext.hh"
#include "data/context/SessionTracker.hh"
#include "data/context/UserContext.hh"

#include "services/FrameEventQueue.hh"
#include "services/Http.hh"
#include "services/IAudioSystem.hh"
#include "services/IConfiguration.hh"
#include "services/ILocalStorage.hh"
#include "services/ServiceLocator.hh"

#include "ui/viewmodels/MessageBoxViewModel.hh"
#include "ui/viewmodels/OverlayManager.hh"
#include "ui/viewmodels/PopupMessageViewModel.hh"
#include "ui/viewmodels/WindowManager.hh"

#include "Exports.hh"
#include "RA_Log.h"
#include "RA_StringUtils.h"

#include <rcheevos\include\rc_api_runtime.h>
#include <rcheevos\src\rapi\rc_api_common.h> // for parsing cached patchdata response
#include <rcheevos\src\rcheevos\rc_internal.h>
#include <rcheevos\src\rcheevos\rc_client_internal.h>

#undef RC_CLIENT_SUPPORTS_EXTERNAL

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
    const auto* pAchievement = pGameContext.Assets().FindAchievement(nAchievementId);
    if (pAchievement == nullptr || pAchievement->GetChanges() != ra::data::models::AssetChanges::None)
        return 0;

    return 1;
}

static int CanSubmitLeaderboardEntry(uint32_t nLeaderboardId, rc_client_t*)
{
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (!pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
        return false;

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto* pLeaderboard = pGameContext.Assets().FindLeaderboard(nLeaderboardId);
    if (pLeaderboard == nullptr || pLeaderboard->GetChanges() != ra::data::models::AssetChanges::None)
        return 0;
    
    return CanSubmit();
}

RcheevosClient::RcheevosClient()
{
    m_pClient.reset(rc_client_create(RcheevosClient::ReadMemory, RcheevosClient::ServerCallAsync));

    m_pClient->callbacks.can_submit_achievement_unlock = CanSubmitAchievementUnlock;
    m_pClient->callbacks.can_submit_leaderboard_entry = CanSubmitLeaderboardEntry;

#ifndef RA_UTEST
    rc_client_enable_logging(m_pClient.get(), RC_CLIENT_LOG_LEVEL_VERBOSE, RcheevosClient::LogMessage);
#endif

    rc_client_set_event_handler(m_pClient.get(), EventHandler);

    rc_client_set_unofficial_enabled(m_pClient.get(), 1);
}

RcheevosClient::~RcheevosClient()
{
    Shutdown();
}

void RcheevosClient::Shutdown() noexcept
{
    if (m_pClient != nullptr)
        rc_client_destroy(m_pClient.release());
}

void RcheevosClient::LogMessage(const char* sMessage, const rc_client_t*)
{
    const auto& pLogger = ra::services::ServiceLocator::Get<ra::services::ILogger>();
    if (pLogger.IsEnabled(ra::services::LogLevel::Info))
        pLogger.LogMessage(ra::services::LogLevel::Info, sMessage);
}

uint32_t RcheevosClient::ReadMemory(uint32_t nAddress, uint8_t* pBuffer, uint32_t nBytes, rc_client_t*)
{
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    return pEmulatorContext.ReadMemory(nAddress, pBuffer, nBytes);
}

static void ConvertHttpResponseToApiServerResponse(rc_api_server_response_t& pResponse,
                                                   const ra::services::Http::Response& httpResponse) noexcept
{
    memset(&pResponse, 0, sizeof(pResponse));
    pResponse.http_status_code = ra::etoi(httpResponse.StatusCode());
    pResponse.body = httpResponse.Content().c_str();
    pResponse.body_length = httpResponse.Content().length();
}

void RcheevosClient::ServerCallAsync(const rc_api_request_t* pRequest,
                                     rc_client_server_callback_t fCallback, void* pCallbackData, rc_client_t*)
{
    ra::services::Http::Request httpRequest(pRequest->url);
    httpRequest.SetPostData(pRequest->post_data);
    httpRequest.SetContentType(pRequest->content_type);

    httpRequest.CallAsync([fCallback, pCallbackData](const ra::services::Http::Response& httpResponse) noexcept {
        rc_api_server_response_t pResponse;
        ConvertHttpResponseToApiServerResponse(pResponse, httpResponse);

        fCallback(&pResponse, pCallbackData);
    });
}

/* ---- ClientSynchronizer ----- */

class RcheevosClient::ClientSynchronizer
{
public:
    ClientSynchronizer() = default;
    virtual ~ClientSynchronizer()
    {
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
        rc_api_buffer_t pBuffer{};
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

    void SyncLeaderboard(rc_client_leaderboard_info_t* pLeaderboard, SubsetWrapper* pSubsetWrapper,
                         const ra::data::models::LeaderboardModel* vmLeaderboard)
    {
        Expects(vmLeaderboard != nullptr);

        const auto sTitle = ra::Narrow(vmLeaderboard->GetName());
        if (!pLeaderboard->public_.title || pLeaderboard->public_.title != sTitle)
            pLeaderboard->public_.title = rc_buf_strcpy(&pSubsetWrapper->pBuffer, sTitle.c_str());

        const auto sDescription = ra::Narrow(vmLeaderboard->GetDescription());
        if (!pLeaderboard->public_.description || pLeaderboard->public_.description != sDescription)
            pLeaderboard->public_.description = rc_buf_strcpy(&pSubsetWrapper->pBuffer, sDescription.c_str());

        pLeaderboard->public_.id = vmLeaderboard->GetID();

        const auto& sMemAddr = vmLeaderboard->GetDefinition();
        uint8_t md5[16];
        rc_runtime_checksum(sMemAddr.c_str(), md5);
        if (memcmp(pLeaderboard->md5, md5, sizeof(md5)) != 0)
        {
            memcpy(pLeaderboard->md5, md5, sizeof(md5));

            if (m_pSubsetWrapper && DetachMemory(m_pSubsetWrapper->vAllocatedMemory, pLeaderboard->lboard))
                free(pLeaderboard->lboard);

            const auto nSize = rc_lboard_size(sMemAddr.c_str());
            if (nSize > 0)
            {
                pLeaderboard->lboard = static_cast<rc_lboard_t*>(malloc(nSize));
                if (pLeaderboard->lboard)
                {
                    /* populate the item, using the communal memrefs pool */
                    rc_parse_state_t parse;
                    rc_init_parse_state(&parse, pLeaderboard->lboard, nullptr, 0);
                    parse.first_memref = &m_pClient->game->runtime.memrefs;
                    parse.variables = &m_pClient->game->runtime.variables;
                    rc_parse_lboard_internal(pLeaderboard->lboard, sMemAddr.c_str(), &parse);
                    rc_destroy_parse_state(&parse);

                    if (AllocatedMemrefs())
                    {
                        // if memrefs were allocated, we can't release this memory until the game is unloaded
                        m_vAllocatedMemory.push_back(pLeaderboard->lboard);
                    }
                    else
                    {
                        pSubsetWrapper->vAllocatedMemory.push_back(pLeaderboard->lboard);
                    }
                }
            }
        }

        SyncLeaderboardState(pLeaderboard, vmLeaderboard);
    }

    void SyncLeaderboardState(rc_client_leaderboard_info_t* pLeaderboard,
                              const ra::data::models::LeaderboardModel* vmLeaderboard)
    {
        switch (vmLeaderboard->GetState())
        {
            case ra::data::models::AssetState::Disabled:
                pLeaderboard->public_.state = RC_CLIENT_LEADERBOARD_STATE_DISABLED;
                if (pLeaderboard->lboard)
                    pLeaderboard->lboard->state = RC_LBOARD_STATE_DISABLED;
                break;
            case ra::data::models::AssetState::Inactive:
                pLeaderboard->public_.state = RC_CLIENT_LEADERBOARD_STATE_INACTIVE;
                if (pLeaderboard->lboard)
                    pLeaderboard->lboard->state = RC_LBOARD_STATE_INACTIVE;
                break;
            case ra::data::models::AssetState::Primed:
                pLeaderboard->public_.state = RC_CLIENT_LEADERBOARD_STATE_TRACKING;
                if (pLeaderboard->lboard)
                    pLeaderboard->lboard->state = RC_LBOARD_STATE_STARTED;
                break;
            case ra::data::models::AssetState::Waiting:
                pLeaderboard->public_.state = RC_CLIENT_LEADERBOARD_STATE_ACTIVE;
                if (pLeaderboard->lboard)
                    pLeaderboard->lboard->state = RC_LBOARD_STATE_WAITING;
                break;
            default:
                pLeaderboard->public_.state = RC_CLIENT_LEADERBOARD_STATE_ACTIVE;
                if (pLeaderboard->lboard)
                    pLeaderboard->lboard->state = RC_LBOARD_STATE_ACTIVE;
                break;
        }
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

    void SyncAchievementState(rc_client_achievement_info_t* pAchievement,
                              const ra::data::models::AchievementModel* vmAchievement)
    {
        switch (vmAchievement->GetState())
        {
            case ra::data::models::AssetState::Triggered:
                pAchievement->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED;
                if (pAchievement->trigger)
                    pAchievement->trigger->state = RC_TRIGGER_STATE_TRIGGERED;
                break;
            case ra::data::models::AssetState::Disabled:
                pAchievement->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_DISABLED;
                pAchievement->public_.bucket = RC_CLIENT_ACHIEVEMENT_BUCKET_UNSUPPORTED;
                break;
            case ra::data::models::AssetState::Inactive:
                pAchievement->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_INACTIVE;
                if (pAchievement->trigger)
                    pAchievement->trigger->state = RC_TRIGGER_STATE_INACTIVE;
                break;
            case ra::data::models::AssetState::Waiting:
                pAchievement->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
                if (pAchievement->trigger)
                    pAchievement->trigger->state = RC_TRIGGER_STATE_WAITING;
                break;
            case ra::data::models::AssetState::Active:
                pAchievement->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
                if (pAchievement->trigger)
                    pAchievement->trigger->state = RC_TRIGGER_STATE_ACTIVE;
                break;
            case ra::data::models::AssetState::Primed:
                pAchievement->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
                if (pAchievement->trigger)
                    pAchievement->trigger->state = RC_TRIGGER_STATE_PRIMED;
                break;
            default:
                pAchievement->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
                break;
        }
    }

    void SyncAchievement(rc_client_achievement_info_t* pAchievement, SubsetWrapper* pSubsetWrapper,
                         const ra::data::models::AchievementModel* vmAchievement)
    {
        Expects(pAchievement != nullptr);
        Expects(pSubsetWrapper != nullptr);
        Expects(vmAchievement != nullptr);

        const auto sTitle = ra::Narrow(vmAchievement->GetName());
        if (!pAchievement->public_.title || pAchievement->public_.title != sTitle)
            pAchievement->public_.title = rc_buf_strcpy(&pSubsetWrapper->pBuffer, sTitle.c_str());

        const auto sDescription = ra::Narrow(vmAchievement->GetDescription());
        if (!pAchievement->public_.description || pAchievement->public_.description != sDescription)
            pAchievement->public_.description = rc_buf_strcpy(&pSubsetWrapper->pBuffer, sDescription.c_str());

        const auto& sBadge = vmAchievement->GetBadge();
        if (ra::StringStartsWith(sBadge, L"local\\"))
        {
            // cannot fit "local/md5.png" into 8 byte buffer. also, client may not understand.
            // encode a value that we can intercept in rc_client_achievement_get_image_url.
            snprintf(pAchievement->public_.badge_name, sizeof(pAchievement->public_.badge_name), "L%06x", pAchievement->public_.id);
        }
        else
        {
            snprintf(pAchievement->public_.badge_name, sizeof(pAchievement->public_.badge_name), "%s", ra::Narrow(sBadge).c_str());
        }

        pAchievement->public_.id = vmAchievement->GetID();
        pAchievement->public_.points = vmAchievement->GetPoints();

        switch (vmAchievement->GetCategory())
        {
            case ra::data::models::AssetCategory::Core:
            case ra::data::models::AssetCategory::Local:
                pAchievement->public_.category = RC_CLIENT_ACHIEVEMENT_CATEGORY_CORE;
                break;

            default:
                pAchievement->public_.category = RC_CLIENT_ACHIEVEMENT_CATEGORY_UNOFFICIAL;
                break;
        }

        const auto& sTrigger = vmAchievement->GetTrigger();
        uint8_t md5[16];
        rc_runtime_checksum(sTrigger.c_str(), md5);
        if (memcmp(pAchievement->md5, md5, sizeof(md5)) != 0)
        {
            memcpy(pAchievement->md5, md5, sizeof(md5));

            if (m_pSubsetWrapper && DetachMemory(m_pSubsetWrapper->vAllocatedMemory, pAchievement->trigger))
                free(pAchievement->trigger);

            const auto nSize = rc_trigger_size(sTrigger.c_str());
            if (nSize > 0)
            {
                void* trigger_buffer = malloc(nSize);
                if (trigger_buffer)
                {
                    /* populate the item, using the communal memrefs pool */
                    rc_parse_state_t parse;
                    rc_init_parse_state(&parse, trigger_buffer, nullptr, 0);
                    parse.first_memref = &m_pClient->game->runtime.memrefs;
                    parse.variables = &m_pClient->game->runtime.variables;
                    const char* sMemaddr = sTrigger.c_str();
                    pAchievement->trigger = RC_ALLOC(rc_trigger_t, &parse);
                    rc_parse_trigger_internal(pAchievement->trigger, &sMemaddr, &parse);
                    rc_destroy_parse_state(&parse);

                    pAchievement->trigger->memrefs = nullptr;

                    if (AllocatedMemrefs())
                    {
                        // if memrefs were allocated, we can't release this memory until the game is unloaded
                        m_vAllocatedMemory.push_back(trigger_buffer);
                    }
                    else
                    {
                        pSubsetWrapper->vAllocatedMemory.push_back(trigger_buffer);
                    }
                }
            }
        }

        SyncAchievementState(pAchievement, vmAchievement);
    }

    void SyncSubset(rc_client_subset_info_t* pSubset, SubsetWrapper* pSubsetWrapper,
                    std::vector<const ra::data::models::AchievementModel*>& vAchievements,
                    std::vector<const ra::data::models::LeaderboardModel*>& vLeaderboards)
    {
        pSubset->active = true;

        if (!vAchievements.empty())
        {
            pSubset->achievements = static_cast<rc_client_achievement_info_t*>(rc_buf_alloc(
                &pSubsetWrapper->pBuffer, vAchievements.size() * sizeof(*pSubset->achievements)));
            memset(pSubset->achievements, 0, vAchievements.size() * sizeof(*pSubset->achievements));

            rc_client_achievement_info_t* pAchievement = pSubset->achievements;
            rc_client_achievement_info_t* pSrcAchievement = nullptr;
            for (const auto* vmAchievement : vAchievements)
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
                            memcpy(pAchievement++, pSrcAchievement, sizeof(*pAchievement));
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

                    if (pSrcAchievement != nullptr)
                    {
                        // transfer the trigger ownership to the new SubsetWrapper
                        if (pAchievement->trigger && DetachMemory(m_pSubsetWrapper->vAllocatedMemory, pAchievement->trigger))
                            pSubsetWrapper->vAllocatedMemory.push_back(pAchievement->trigger);

                        memcpy(pAchievement++, pSrcAchievement, sizeof(*pAchievement));
                    }
                    else
                    {
                        SyncAchievement(pAchievement++, pSubsetWrapper, vmAchievement);
                    }
                }
            }

            pSubset->public_.num_achievements = gsl::narrow_cast<uint32_t>(pAchievement - pSubset->achievements);
        }

        if (!vLeaderboards.empty())
        {
            pSubset->leaderboards = static_cast<rc_client_leaderboard_info_t*>(rc_buf_alloc(
                &pSubsetWrapper->pBuffer, vLeaderboards.size() * sizeof(*pSubset->leaderboards)));
            memset(pSubset->leaderboards, 0, vLeaderboards.size() * sizeof(*pSubset->leaderboards));

            rc_client_leaderboard_info_t* pLeaderboard = pSubset->leaderboards;
            rc_client_leaderboard_info_t* pSrcLeaderboard = nullptr;
            for (const auto* vmLeaderboard : vLeaderboards)
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
                            memcpy(pLeaderboard++, pSrcLeaderboard, sizeof(*pLeaderboard));
                            break;
                        }
                    }
                }
                else
                {
                    pSrcLeaderboard = nullptr;
                    if (m_pSubsetWrapper != nullptr)
                    {
                        if (vmLeaderboard->GetCategory() == ra::data::models::AssetCategory::Local)
                            pSrcLeaderboard = FindLeaderboard(&m_pSubsetWrapper->pLocalSubset, nLeaderboardId);
                        else
                            pSrcLeaderboard = FindLeaderboard(&m_pSubsetWrapper->pCoreSubset, nLeaderboardId);
                    }

                    if (pSrcLeaderboard != nullptr)
                    {
                        // transfer the lboard ownership to the new SubsetWrapper
                        if (pLeaderboard->lboard && DetachMemory(m_pSubsetWrapper->vAllocatedMemory, pLeaderboard->lboard))
                            pSubsetWrapper->vAllocatedMemory.push_back(pLeaderboard->lboard);

                        memcpy(pLeaderboard++, pSrcLeaderboard, sizeof(*pLeaderboard));
                    }
                    else
                    {
                        SyncLeaderboard(pLeaderboard++, pSubsetWrapper, vmLeaderboard);
                    }
                }
            }

            pSubset->public_.num_leaderboards = gsl::narrow_cast<uint32_t>(pLeaderboard - pSubset->leaderboards);
        }
    }

public:
    void SyncAssets(rc_client_t* pClient)
    {
        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
        const auto& vAssets = pGameContext.Assets();

        m_pClient = pClient;
        m_pNextMemref = &pClient->game->runtime.memrefs;
        m_pNextVariable = &pClient->game->runtime.variables;
        AllocatedMemrefs(); // advance pointers

        if (m_pPublishedSubset == nullptr)
            m_pPublishedSubset = pClient->game->subsets;

        std::vector<const ra::data::models::AchievementModel*> vCoreAchievements;
        std::vector<const ra::data::models::AchievementModel*> vLocalAchievements;
        std::vector<const ra::data::models::LeaderboardModel*> vCoreLeaderboards;
        std::vector<const ra::data::models::LeaderboardModel*> vLocalLeaderboards;

        for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(vAssets.Count()); ++nIndex)
        {
            const auto* pAsset = vAssets.GetItemAt(nIndex);
            Expects(pAsset != nullptr);
            if (pAsset->GetChanges() == ra::data::models::AssetChanges::Deleted)
                continue;

            switch (pAsset->GetType())
            {
                case ra::data::models::AssetType::Achievement:
                    if (pAsset->GetCategory() == ra::data::models::AssetCategory::Local)
                        vLocalAchievements.push_back(dynamic_cast<const ra::data::models::AchievementModel*>(pAsset));
                    else
                        vCoreAchievements.push_back(dynamic_cast<const ra::data::models::AchievementModel*>(pAsset));
                    break;

                case ra::data::models::AssetType::Leaderboard:
                    if (pAsset->GetCategory() == ra::data::models::AssetCategory::Local)
                        vLocalLeaderboards.push_back(dynamic_cast<const ra::data::models::LeaderboardModel*>(pAsset));
                    else
                        vCoreLeaderboards.push_back(dynamic_cast<const ra::data::models::LeaderboardModel*>(pAsset));
                    break;

                default:
                    break;
            }
        }

        std::unique_ptr<SubsetWrapper> pSubsetWrapper;
        pSubsetWrapper.reset(new SubsetWrapper);
        memset(&pSubsetWrapper->pCoreSubset, 0, sizeof(pSubsetWrapper->pCoreSubset));
        memset(&pSubsetWrapper->pLocalSubset, 0, sizeof(pSubsetWrapper->pLocalSubset));
        rc_buf_init(&pSubsetWrapper->pBuffer);

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

    void SyncAchievement(ra::data::models::AchievementModel& vmAchievement, bool bStateOnly)
    {
        const auto nId = vmAchievement.GetID();
        auto* pAchievement = FindAchievement(&m_pSubsetWrapper->pLocalSubset, nId);
        if (pAchievement != nullptr)
        {
            if (bStateOnly)
                SyncAchievementState(pAchievement, &vmAchievement);
            else
                SyncAchievement(pAchievement, m_pSubsetWrapper.get(), &vmAchievement);
            return;
        }

        pAchievement = FindAchievement(&m_pSubsetWrapper->pCoreSubset, nId);
        if (pAchievement != nullptr)
        {
            if (bStateOnly)
                SyncAchievementState(pAchievement, &vmAchievement);
            else
                SyncAchievement(pAchievement, m_pSubsetWrapper.get(), &vmAchievement);
            return;
        }

        SyncAssets(m_pClient);
    }

    void SyncLeaderboard(ra::data::models::LeaderboardModel& vmLeaderboard, bool bStateOnly)
    {
        const auto nId = vmLeaderboard.GetID();
        auto* pLeaderboard = FindLeaderboard(&m_pSubsetWrapper->pLocalSubset, nId);
        if (pLeaderboard != nullptr)
        {
            if (bStateOnly)
                SyncLeaderboardState(pLeaderboard, &vmLeaderboard);
            else
                SyncLeaderboard(pLeaderboard, m_pSubsetWrapper.get(), &vmLeaderboard);
            return;
        }

        pLeaderboard = FindLeaderboard(&m_pSubsetWrapper->pCoreSubset, nId);
        if (pLeaderboard != nullptr)
        {
            if (bStateOnly)
                SyncLeaderboardState(pLeaderboard, &vmLeaderboard);
            else
                SyncLeaderboard(pLeaderboard, m_pSubsetWrapper.get(), &vmLeaderboard);
            return;
        }

        SyncAssets(m_pClient);
    }
};

void RcheevosClient::SyncAssets()
{
    if (m_pClientSynchronizer == nullptr)
        m_pClientSynchronizer.reset(new ClientSynchronizer());
    m_pClientSynchronizer->SyncAssets(GetClient());
}

void RcheevosClient::SyncAchievement(ra::data::models::AchievementModel& vmAchievement, bool bStateOnly)
{
    if (m_pClientSynchronizer != nullptr)
        m_pClientSynchronizer->SyncAchievement(vmAchievement, bStateOnly);
}

void RcheevosClient::SyncLeaderboard(ra::data::models::LeaderboardModel& vmLeaderboard, bool bStateOnly)
{
    if (m_pClientSynchronizer != nullptr)
        m_pClientSynchronizer->SyncLeaderboard(vmLeaderboard, bStateOnly);
}

static rc_client_achievement_info_t* GetAchievementInfo(rc_client_t* pClient, ra::AchievementID nId)
{
    rc_client_subset_info_t* subset = pClient->game->subsets;
    for (; subset; subset = subset->next)
    {
        rc_client_achievement_info_t* achievement = subset->achievements;
        rc_client_achievement_info_t* stop = achievement + subset->public_.num_achievements;

        for (; achievement < stop; ++achievement)
        {
            if (achievement->public_.id == nId)
                return achievement;
        }
    }

    return nullptr;
}

rc_trigger_t* RcheevosClient::GetAchievementTrigger(ra::AchievementID nId) const
{
    rc_client_achievement_info_t* achievement = GetAchievementInfo(GetClient(), nId);
    return (achievement != nullptr) ? achievement->trigger : nullptr;
}

static rc_client_leaderboard_info_t* GetLeaderboardInfo(rc_client_t* pClient, ra::LeaderboardID nId)
{
    rc_client_subset_info_t* subset = pClient->game->subsets;
    for (; subset; subset = subset->next)
    {
        rc_client_leaderboard_info_t* leaderboard = subset->leaderboards;
        rc_client_leaderboard_info_t* stop = leaderboard + subset->public_.num_leaderboards;

        for (; leaderboard < stop; ++leaderboard)
        {
            if (leaderboard->public_.id == nId)
                return leaderboard;
        }
    }

    return nullptr;
}

rc_lboard_t* RcheevosClient::GetLeaderboardDefinition(ra::LeaderboardID nId) const
{
    rc_client_leaderboard_info_t* leaderboard = GetLeaderboardInfo(GetClient(), nId);
    return (leaderboard != nullptr) ? leaderboard->lboard : nullptr;
}

bool RcheevosClient::HasRichPresence() const
{
    return rc_client_has_rich_presence(GetClient());
}

std::wstring RcheevosClient::GetRichPresenceDisplayString() const
{
    char buffer[256];
    if (rc_client_get_rich_presence_message(GetClient(), buffer, sizeof(buffer)) > 0)
        return ra::Widen(buffer);

    return L"";
}

/* ---- Login ----- */

void RcheevosClient::BeginLoginWithPassword(const std::string& sUsername, const std::string& sPassword,
                                           rc_client_callback_t fCallback, void* pCallbackData)
{
    GSL_SUPPRESS_R3
    auto* pCallbackWrapper = new CallbackWrapper(m_pClient.get(), fCallback, pCallbackData);
    BeginLoginWithPassword(sUsername.c_str(), sPassword.c_str(), pCallbackWrapper);
}

rc_client_async_handle_t* RcheevosClient::BeginLoginWithPassword(const char* sUsername, const char* sPassword,
                                                                 CallbackWrapper* pCallbackWrapper) noexcept
{
    return rc_client_begin_login_with_password(GetClient(), sUsername, sPassword,
                                               RcheevosClient::LoginCallback, pCallbackWrapper);
}

void RcheevosClient::BeginLoginWithToken(const std::string& sUsername, const std::string& sApiToken,
                                         rc_client_callback_t fCallback, void* pCallbackData)
{
    GSL_SUPPRESS_R3
    auto* pCallbackWrapper = new CallbackWrapper(m_pClient.get(), fCallback, pCallbackData);
    BeginLoginWithToken(sUsername.c_str(), sApiToken.c_str(), pCallbackWrapper);
}

rc_client_async_handle_t* RcheevosClient::BeginLoginWithToken(const char* sUsername, const char* sApiToken,
                                                              CallbackWrapper* pCallbackWrapper) noexcept
{
    return rc_client_begin_login_with_token(GetClient(), sUsername, sApiToken,
                                            RcheevosClient::LoginCallback, pCallbackWrapper);
}

GSL_SUPPRESS_CON3
void RcheevosClient::LoginCallback(int nResult, const char* sErrorMessage,
                                   rc_client_t* pClient, void* pUserdata)
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

void RcheevosClient::BeginLoadGame(const std::string& sHash, unsigned id,
                                   rc_client_callback_t fCallback, void* pCallbackData)
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
    rc_json_field_t fields[] = {
        RC_JSON_NEW_FIELD("Success"),
        RC_JSON_NEW_FIELD("Error"),
        RC_JSON_NEW_FIELD("PatchData")
    };


    if (rc_json_parse_server_response(&api_response, server_response, fields, sizeof(fields) / sizeof(fields[0])) == RC_OK &&
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
void RcheevosClient::PostProcessGameDataResponse(const rc_api_server_response_t* server_response,
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

rc_client_async_handle_t* RcheevosClient::BeginLoadGame(const char* sHash, unsigned id,
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

    return rc_client_begin_load_game(client, sHash, RcheevosClient::LoadGameCallback, pCallbackWrapper);
}

GSL_SUPPRESS_CON3
void RcheevosClient::LoadGameCallback(int nResult, const char* sErrorMessage, rc_client_t*, void* pUserdata)
{
    auto* wrapper = static_cast<LoadGameCallbackWrapper*>(pUserdata);
    Expects(wrapper != nullptr);

    if (nResult == RC_OK || nResult == RC_NO_GAME_LOADED)
    {
        // initialize the game context
        auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
        pGameContext.InitializeFromRcheevosClient(wrapper->m_mAchievementDefinitions,
                                                  wrapper->m_mLeaderboardDefinitions);
    }
    else
    {

    }

    wrapper->DoCallback(nResult, sErrorMessage);

    delete wrapper;
}

/* ---- DoFrame ----- */

void RcheevosClient::DoFrame()
{
    rc_client_do_frame(GetClient());
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
    vmPopup->SetImage(ra::ui::ImageType::Badge, pAchievement.badge_name);
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

    if (pGameContext.GetMode() == ra::data::context::GameContext::Mode::CompatibilityTest)
    {
        auto sHeader = vmPopup->GetTitle();
        sHeader.insert(0, L"Test ");
        vmPopup->SetTitle(sHeader);

        RA_LOG_INFO("Achievement %u not unlocked - %s", pAchievement.id, "test compatibility mode");
        bSubmit = false;
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
            std::wstring sPath = ra::StringPrintf(L"%s%u.png", pConfiguration.GetScreenshotDirectory(), pAchievement.id);
            pOverlayManager.CaptureScreenshot(nPopupId, sPath);
        }

        s_mAchievementPopups[pAchievement.id] = nPopupId;
    }

    if (vmAchievement->IsPauseOnTrigger())
    {
        auto& pFrameEventQueue = ra::services::ServiceLocator::GetMutable<ra::services::FrameEventQueue>();
        pFrameEventQueue.QueuePauseOnTrigger(vmAchievement->GetName());
    }
}

static void HandleChallengeIndicatorShowEvent(const rc_client_achievement_t& pAchievement)
{
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    auto* vmAchievement = pGameContext.Assets().FindAchievement(pAchievement.id);
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
    auto* vmAchievement = pGameContext.Assets().FindAchievement(pAchievement.id);
    if (!vmAchievement)
    {
        RA_LOG_ERR("Received achievement triggered event for unknown achievement %u", pAchievement.id);
        return;
    }

    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (pConfiguration.GetPopupLocation(ra::ui::viewmodels::Popup::Progress) !=
        ra::ui::viewmodels::PopupLocation::None)
    {
        auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();

        // use locked badge if available
        auto sBadgeName = ra::Narrow(vmAchievement->GetBadge());
        if (!ra::StringStartsWith(sBadgeName, "local\\"))
            sBadgeName += "_lock";

        pOverlayManager.UpdateProgressTracker(ra::ui::ImageType::Badge, sBadgeName, ra::Widen(pAchievement.measured_progress));
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

static void HandleLeaderboardStartedEvent(const rc_client_leaderboard_t& pLeaderboard)
{
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    auto* vmLeaderboard = pGameContext.Assets().FindLeaderboard(pLeaderboard.id);
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
    auto* vmLeaderboard = pGameContext.Assets().FindLeaderboard(pLeaderboard.id);
    if (!vmLeaderboard)
    {
        RA_LOG_ERR("Received leaderboard started event for unknown leaderboard %u", pLeaderboard.id);
        return;
    }

    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (pConfiguration.GetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardStarted) !=
        ra::ui::viewmodels::PopupLocation::None)
    {
        ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\lbcancel.wav");
        auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
        pOverlayManager.QueueMessage(ra::ui::viewmodels::Popup::LeaderboardStarted, L"Leaderboard attempt failed",
                                     ra::Widen(pLeaderboard.title), ra::Widen(pLeaderboard.description));
    }
}

static void HandleLeaderboardSubmittedEvent(const rc_client_leaderboard_t& pLeaderboard)
{
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    auto* vmLeaderboard = pGameContext.Assets().FindLeaderboard(pLeaderboard.id);
    if (!vmLeaderboard)
    {
        RA_LOG_ERR("Received leaderboard started event for unknown leaderboard %u", pLeaderboard.id);
        return;
    }

    std::unique_ptr<ra::ui::viewmodels::PopupMessageViewModel> vmPopup(new ra::ui::viewmodels::PopupMessageViewModel);
    vmPopup->SetDescription(ra::Widen(pLeaderboard.title));
    std::wstring sTitle = L"Leaderboard NOT Submitted";
    bool bSubmit = true;

    switch (vmLeaderboard->GetCategory())
    {
        case ra::data::models::AssetCategory::Local:
            sTitle.insert(0, L"Local ");
            bSubmit = false;
            break;

        case ra::data::models::AssetCategory::Unofficial:
            sTitle.insert(0, L"Unofficial ");
            bSubmit = false;
            break;

        default:
            break;
    }

    if (pGameContext.GetMode() == ra::data::context::GameContext::Mode::CompatibilityTest)
    {
        vmPopup->SetDetail(L"Leaderboards are not submitted in test mode.");
        bSubmit = false;
    }

    if (vmLeaderboard->IsModified() ||                                                    // actual modifications
        (bSubmit && vmLeaderboard->GetChanges() != ra::data::models::AssetChanges::None)) // unpublished changes
    {
        sTitle.insert(0, L"Modified ");
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

static void HandleLeaderboardScoreboardEvent(const rc_client_leaderboard_scoreboard_t& pScoreboard, const rc_client_leaderboard_t& pLeaderboard)
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
                pEntryViewModel->SetScore(ra::StringPrintf(L"(%s) %s", pScoreboard.submitted_score, pScoreboard.best_score));
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
        // TODO: match pServerError.id to a popup - by matching description? - then SetErrorDetail
        const auto nAchievementId = 0;//pServerError.target_id;
        const auto nPopupId = s_mAchievementPopups[nAchievementId];

        const auto sErrorMessage = pServerError.error_message ?
            ra::Widen(pServerError.error_message) : L"Error submitting unlock";
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
    {

    }

    ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(
        ra::StringPrintf(L"%s %s", pServerError.api, pServerError.error_message));
}

void RcheevosClient::EventHandler(const rc_client_event_t* pEvent, rc_client_t*)
{
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

        case RC_CLIENT_EVENT_SERVER_ERROR:
            HandleServerError(*pEvent->server_error);
            break;

        default:
            RA_LOG_WARN("Unhandled rc_client_event %u", pEvent->type);
            break;
    }
}

/* ---- Exports ----- */

#ifdef RC_CLIENT_SUPPORTS_EXTERNAL

class RcheevosClientExports : private RcheevosClient
{
public:
    static rc_client_async_handle_t* begin_login_with_password(rc_client_t* client,
        const char* username, const char* password,
        rc_client_callback_t callback, void* callback_userdata)
    {
        auto* pCallbackData = new CallbackWrapper(client, callback, callback_userdata);

        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::RcheevosClient>();
        return pClient.BeginLoginWithPassword(username, password, pCallbackData);
    }

    static rc_client_async_handle_t* begin_login_with_token(rc_client_t* client,
        const char* username, const char* token,
        rc_client_callback_t callback, void* callback_userdata)
    {
        auto* pCallbackData = new CallbackWrapper(client, callback, callback_userdata);

        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::RcheevosClient>();
        return pClient.BeginLoginWithToken(username, token, pCallbackData);
    }

    static const rc_client_user_t* get_user_info()
    {
        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::RcheevosClient>();
        return rc_client_get_user_info(pClient.GetClient());
    }

    static void logout()
    {
        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::RcheevosClient>();
        return rc_client_logout(pClient.GetClient());
    }
};

#endif RC_CLIENT_SUPPORTS_EXTERNAL

} // namespace services
} // namespace ra

#ifdef RC_CLIENT_SUPPORTS_EXTERNAL

#include "Exports.hh"
#include "rcheevos/src/rcheevos/rc_client_external.h"

#ifdef __cplusplus
extern "C" {
#endif

API int CCONV _Rcheevos_GetExternalClient(rc_client_external_t* pClient, int nVersion)
{
    switch (nVersion)
    {
        default:
            RA_LOG_WARN("Unknown rc_client_external interface version: %s", nVersion);
            __fallthrough;

        case 1:
            pClient->begin_login_with_password = ra::services::RcheevosClientExports::begin_login_with_password;
            pClient->begin_login_with_token = ra::services::RcheevosClientExports::begin_login_with_token;
            pClient->logout = ra::services::RcheevosClientExports::logout;
            pClient->get_user_info = ra::services::RcheevosClientExports::get_user_info;
            break;
    }

    return 1;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* RC_CLIENT_SUPPORTS_EXTERNAL */
