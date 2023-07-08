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

namespace ra {
namespace services {

typedef struct rc_client_callback_wrapper_t
{
    rc_client_callback_t callback;
    void* userdata;
} rc_client_callback_wrapper_t;

RcheevosClient::RcheevosClient()
{
    m_pClient.reset(rc_client_create(RcheevosClient::ReadMemory, RcheevosClient::ServerCallAsync));

    rc_client_enable_logging(m_pClient.get(), RC_CLIENT_LOG_LEVEL_VERBOSE, RcheevosClient::LogMessage);
}

RcheevosClient::~RcheevosClient()
{
    Shutdown();
}

void RcheevosClient::Shutdown()
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

void RcheevosClient::SetSynchronous(bool bSynchronous)
{
    if (bSynchronous)
        m_pClient->callbacks.server_call = RcheevosClient::ServerCallAsync;
    else
        m_pClient->callbacks.server_call = RcheevosClient::ServerCall;
}

uint32_t RcheevosClient::ReadMemory(uint32_t nAddress, uint8_t* pBuffer, uint32_t nBytes, rc_client_t*)
{
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    return pEmulatorContext.ReadMemory(nAddress, pBuffer, nBytes);
}

static void ConvertHttpResponseToApiServerResponse(rc_api_server_response_t& pResponse,
                                                   const ra::services::Http::Response& httpResponse)
{
    memset(&pResponse, 0, sizeof(pResponse));
    pResponse.http_status_code = ra::etoi(httpResponse.StatusCode());
    pResponse.body = httpResponse.Content().c_str();
    pResponse.body_length = httpResponse.Content().length();
}

void RcheevosClient::ServerCall(const rc_api_request_t* pRequest,
                                rc_client_server_callback_t fCallback, void* pCallbackData, rc_client_t*)
{
    ra::services::Http::Request httpRequest(pRequest->url);
    httpRequest.SetPostData(pRequest->post_data);

    ra::services::Http::Response httpResponse = httpRequest.Call();
    rc_api_server_response_t pResponse;
    ConvertHttpResponseToApiServerResponse(pResponse, httpResponse);

    fCallback(&pResponse, pCallbackData);
}

void RcheevosClient::ServerCallAsync(const rc_api_request_t* pRequest,
                                     rc_client_server_callback_t fCallback, void* pCallbackData, rc_client_t*)
{
    ra::services::Http::Request httpRequest(pRequest->url);
    httpRequest.SetPostData(pRequest->post_data);

    httpRequest.CallAsync([fCallback, pCallbackData](const ra::services::Http::Response& httpResponse) {
        rc_api_server_response_t pResponse;
        ConvertHttpResponseToApiServerResponse(pResponse, httpResponse);

        fCallback(&pResponse, pCallbackData);
    });
}

void RcheevosClient::BeginLoginWithPassword(const std::string& sUsername, const std::string& sPassword,
                                           rc_client_callback_t fCallback, void* pCallbackData)
{
    rc_client_callback_wrapper_t* wrapper = new rc_client_callback_wrapper_t();
    wrapper->callback = fCallback;
    wrapper->userdata = pCallbackData;

    rc_client_begin_login_with_password(GetClient(), sUsername.c_str(), sPassword.c_str(),
        RcheevosClient::LoginCallback, wrapper);
}

void RcheevosClient::BeginLoginWithToken(const std::string& sUsername, const std::string& sApiToken,
                                         rc_client_callback_t fCallback, void* pCallbackData)
{
    rc_client_callback_wrapper_t* wrapper = new rc_client_callback_wrapper_t();
    wrapper->callback = fCallback;
    wrapper->userdata = pCallbackData;

    rc_client_begin_login_with_token(GetClient(), sUsername.c_str(), sApiToken.c_str(),
        RcheevosClient::LoginCallback, wrapper);
}

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

    auto* wrapper = reinterpret_cast<rc_client_callback_wrapper_t*>(pUserdata);
    wrapper->callback(nResult, sErrorMessage, pClient, wrapper->userdata);

    delete wrapper;
}

} // namespace services
} // namespace ra
