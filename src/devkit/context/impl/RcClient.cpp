#include "RcClient.hh"

#include "context/UserContext.hh"

#include "services/IHttpRequester.hh"
#include "services/ILocalStorage.hh"
#include "services/ServiceLocator.hh"

#include "util/Log.hh"
#include "util/Strings.hh"

#include <rcheevos/src/rc_client_internal.h>

namespace ra {
namespace context {

IRcClient::~IRcClient() noexcept
{
    // destructor must be defined in a scope where rc_client_t is not a forward declaration
}

rc_api_host_t* IRcClient::GetHost() const noexcept
{
    return &m_pClient->state.host;
}

const std::wstring IRcClient::GetErrorMessage(int nResult, const rc_api_response_t& pResponse)
{
    if (pResponse.error_message)
        return ra::util::String::Widen(pResponse.error_message);

    return ra::util::String::Widen(rc_error_str(nResult));
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

void RcClient::AddAuthentication(const char** pUsername, const char** pApiToken) const
{
    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::context::UserContext>();
    if (pUsername)
        *pUsername = pUserContext.GetUsername().c_str();
    if (pApiToken)
        *pApiToken = pUserContext.GetApiToken().c_str();
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

static std::string_view FindParameter(const std::string& sInput, const std::string& sParameter)
{
    auto nIndex = sInput.find(sParameter);
    while (nIndex != std::string::npos)
    {
        if (nIndex == 0 || sInput.at(nIndex - 1) == '&')
        {
            nIndex += sParameter.length();
            auto nIndex2 = sInput.find('&', nIndex);
            if (nIndex2 == std::string::npos)
                nIndex2 = sInput.length();
            return std::string_view(&sInput.at(nIndex), nIndex2 - nIndex);
        }

        nIndex = sInput.find(sParameter, nIndex + sParameter.length());
    }

    return {};
}

void RcClient::DispatchRequest(const rc_api_request_t& pRequest,
    std::function<void(const rc_api_server_response_t&, void*)> fCallback,
    void* pCallbackData) const
{
    ra::services::Http::Request httpRequest(pRequest.url);
    httpRequest.SetPostData(pRequest.post_data);
    httpRequest.SetContentType(pRequest.content_type);

    std::string sParams = httpRequest.GetPostData();

    std::string sApi;
    const auto svApi = FindParameter(sParams, "r=");
    if (!svApi.empty())
    {
        sApi = svApi;

        if (ra::services::ServiceLocator::Exists<ra::services::ILogger>())
        {
            const auto svToken = FindParameter(sParams, "t=");
            if (!svToken.empty())
                sParams.replace(svToken.data() - sParams.data(), svToken.length(), "[redacted]");

            if (ra::util::String::StartsWith(sApi, "login"))
            {
                const auto svPassword = FindParameter(sParams, "p=");
                if (!svPassword.empty())
                    sParams.replace(svPassword.data() - sParams.data(), svPassword.length(), "[redacted]");
            }

            RA_LOG_INFO(">> %s request: %s", sApi.c_str(), sParams.c_str());
        }
    }

    CallApi(sApi, httpRequest, fCallback, pCallbackData);
}

static void CacheCodeNotesResponse(const rc_api_server_response_t& pResponse, const std::wstring& sGameId)
{
    // store a copy in the cache for offline mode
    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
    auto pData = pLocalStorage.WriteText(ra::services::StorageItemType::MemoryNotes, sGameId);
    if (pData != nullptr)
    {
        std::string sContent(pResponse.body, pResponse.body_length);
        auto nIndex = sContent.find('[');
        sContent.erase(0, nIndex);
        nIndex = sContent.find_last_of(']');
        sContent.erase(nIndex + 1);
        pData->Write(sContent);
    }
}

void RcClient::CallApi(const std::string& sApi, const ra::services::Http::Request& pRequest,
    std::function<void(const rc_api_server_response_t&, void*)> fCallback,
    void* pCallbackData) const
{
    std::wstring sParameter;
    if (sApi == "codenotes2") {
        const auto svGameId = FindParameter(pRequest.GetPostData(), "g=");
        if (!svGameId.empty())
            sParameter = ra::util::String::Widen(svGameId);
    }

    pRequest.CallAsync([fCallback, pCallbackData, sApi=sApi, sParameter](const ra::services::Http::Response& httpResponse)
        {
            rc_api_server_response_t pResponse;
            std::string sErrorBuffer;
            ConvertHttpResponseToApiServerResponse(pResponse, httpResponse, sErrorBuffer);

            if (ra::services::ServiceLocator::Exists<ra::services::ILogger>())
            {
                if (httpResponse.StatusCode() == ra::services::Http::StatusCode::OK &&
                    ra::util::String::StartsWith(sApi, "login"))
                {
                    auto sResponse = httpResponse.Content();
                    auto nIndex = sResponse.find("\"Token\":\"");
                    if (nIndex != std::string::npos)
                    {
                        nIndex += 9;
                        const auto nIndex2 = sResponse.find('"', nIndex);
                        if (nIndex2 != std::string::npos)
                            sResponse.replace(nIndex, nIndex2 - nIndex, "[redacted]");
                    }
                    RA_LOG_INFO("<< %s response (%d): %s", sApi.c_str(), ra::etoi(httpResponse.StatusCode()), sResponse.c_str());
                }
                else
                {
                    RA_LOG_INFO("<< %s response (%d): %s", sApi.c_str(), ra::etoi(httpResponse.StatusCode()), httpResponse.Content().c_str());
                }
            }

            if (sApi == "codenotes2") {
                CacheCodeNotesResponse(pResponse, sParameter);
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
