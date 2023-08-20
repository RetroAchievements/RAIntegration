#include "RcheevosClient.hh"

#include "data/context/EmulatorContext.hh"
#include "data/context/SessionTracker.hh"
#include "data/context/UserContext.hh"

#include "services/Http.hh"
#include "services/IAudioSystem.hh"
#include "services/ServiceLocator.hh"

#include "ui/viewmodels/MessageBoxViewModel.hh"
#include "ui/viewmodels/OverlayManager.hh"
#include "ui/viewmodels/PopupMessageViewModel.hh"
#include "ui/viewmodels/WindowManager.hh"

#include "RA_Log.h"
#include "RA_StringUtils.h"

#include <rcheevos\src\rcheevos\rc_internal.h>
#include <rcheevos\src\rcheevos\rc_client_internal.h>

#undef RC_CLIENT_SUPPORTS_EXTERNAL

namespace ra {
namespace services {

RcheevosClient::RcheevosClient()
{
    m_pClient.reset(rc_client_create(RcheevosClient::ReadMemory, RcheevosClient::ServerCallAsync));

    rc_client_enable_logging(m_pClient.get(), RC_CLIENT_LOG_LEVEL_VERBOSE, RcheevosClient::LogMessage);
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
