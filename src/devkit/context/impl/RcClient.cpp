#include "RcClient.hh"

#include "services/IHttpRequester.hh"
#include "services/ServiceLocator.hh"

#include "util/Log.hh"

#include <rcheevos/src/rc_client_internal.h>

namespace ra {
namespace context {

IRcClient::~IRcClient() noexcept
{
    // destructor must be defined in a scope where rc_client_t is not a forward declaration
}

namespace impl {

RcClient::RcClient() noexcept
{
    m_pClient.reset(rc_client_create(nullptr, RcClient::ServerCallAsync));

    if (ra::services::ServiceLocator::Exists<ra::services::ILogger>())
       rc_client_enable_logging(m_pClient.get(), RC_CLIENT_LOG_LEVEL_VERBOSE, RcClient::LogMessage);

    m_pClient->state.allow_leaderboards_in_softcore = true;

    rc_client_set_unofficial_enabled(m_pClient.get(), 1);
}

static void DummyEventHandler(const rc_client_event_t*, rc_client_t*) noexcept
{
}

void RcClient::Shutdown() noexcept
{
    if (m_pClient != nullptr)
    {
        // don't need to handle events while shutting down
        rc_client_set_event_handler(m_pClient.get(), DummyEventHandler);

        rc_client_destroy(m_pClient.release());
    }
}

void RcClient::LogMessage(const char* sMessage, const rc_client_t*)
{
    const auto& pLogger = ra::services::ServiceLocator::Get<ra::services::ILogger>();
    if (pLogger.IsEnabled(ra::services::LogLevel::Info))
        pLogger.LogMessage(ra::services::LogLevel::Info, sMessage);
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

void RcClient::DispatchRequest(const rc_api_request_t& pRequest,
    std::function<void(const rc_api_server_response_t&, void*)> fCallback,
    void* pCallbackData) const
{
    ra::services::Http::Request httpRequest(pRequest.url);
    httpRequest.SetPostData(pRequest.post_data);
    httpRequest.SetContentType(pRequest.content_type);

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

        if (ra::services::ServiceLocator::Exists<ra::services::ILogger>())
        {
            RA_LOG_INFO(">> %s request: %s", sApi.c_str(), sParams.c_str());
        }
    }

    CallApi(sApi, httpRequest, fCallback, pCallbackData);
}

void RcClient::CallApi(const std::string& sApi, const ra::services::Http::Request& pRequest,
    std::function<void(const rc_api_server_response_t&, void*)> fCallback,
    void* pCallbackData) const
{
    pRequest.CallAsync([fCallback, pCallbackData, sApi](const ra::services::Http::Response& httpResponse)
        {
            rc_api_server_response_t pResponse;
            std::string sErrorBuffer;
            ConvertHttpResponseToApiServerResponse(pResponse, httpResponse, sErrorBuffer);

            if (ra::services::ServiceLocator::Exists<ra::services::ILogger>())
            {
                RA_LOG_INFO("<< %s response (%d): %s", sApi.c_str(), ra::etoi(httpResponse.StatusCode()), httpResponse.Content().c_str());
            }

            fCallback(pResponse, pCallbackData);
        });
}

void RcClient::ServerCallAsync(const rc_api_request_t* pRequest, rc_client_server_callback_t fCallback,
    void* pCallbackData, rc_client_t*)
{
    Expects(pRequest != nullptr);

    auto& pClient = ra::services::ServiceLocator::GetMutable<IRcClient>();
    pClient.DispatchRequest(*pRequest, [fCallback](const rc_api_server_response_t& pResponse, void* pCallbackData)
        {
            fCallback(&pResponse, pCallbackData);
        }, pCallbackData);
}

} // namespace impl
} // namespace context
} // namespace ra
