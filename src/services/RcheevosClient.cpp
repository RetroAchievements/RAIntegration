#include "RcheevosClient.hh"

#include "data/context/EmulatorContext.hh"
#include "data/context/SessionTracker.hh"
#include "data/context/UserContext.hh"

#include "services/Http.hh"
#include "services/IAudioSystem.hh"
#include "services/ILocalStorage.hh"
#include "services/ServiceLocator.hh"

#include "ui/viewmodels/MessageBoxViewModel.hh"
#include "ui/viewmodels/OverlayManager.hh"
#include "ui/viewmodels/PopupMessageViewModel.hh"
#include "ui/viewmodels/WindowManager.hh"

#include "RA_Log.h"
#include "RA_StringUtils.h"

#include <rcheevos\include\rc_api_runtime.h>
#include <rcheevos\src\rapi\rc_api_common.h> // for parsing cached patchdata response
#include <rcheevos\src\rcheevos\rc_internal.h>
#include <rcheevos\src\rcheevos\rc_client_internal.h>

#undef RC_CLIENT_SUPPORTS_EXTERNAL

namespace ra {
namespace services {

static int CanSubmit()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    if (pGameContext.GetMode() == ra::data::context::GameContext::Mode::CompatibilityTest)
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

static int CanSubmitLeaderboardEntry(uint32_t, rc_client_t*)
{
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

    if (id != 0)
    {
        auto* client_hash = rc_client_find_game_hash(client, sHash);
        if (ra::to_signed(client_hash->game_id) < 0)
            client_hash->game_id = id;
    }

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
