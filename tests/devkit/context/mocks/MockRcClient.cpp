#include "MockRcClient.hh"

#include "tests\devkit\testutil\CppUnitTest.hh"

#include "util\Strings.hh"

#include <rcheevos\src\rc_client_internal.h>

namespace ra {
namespace context {
namespace mocks {

MockRcClient::MockRcClient() noexcept : m_Override(this)
{
    /* disable logging */
    rc_client_enable_logging(GetClient(), RC_CLIENT_LOG_LEVEL_NONE, nullptr);
}

void MockRcClient::DispatchRequest(const rc_api_request_t & pRequest,
    std::function<void(const rc_api_server_response_t&, void*)> fCallback,
    void* pCallbackData) const
{
    std::string sRequestParams = pRequest.post_data;

    for (auto& pResponse : m_vResponses)
    {
        if (pResponse.sRequestParams == sRequestParams)
        {
            pResponse.bSeen = true;

            if (pResponse.fBeforeResponse)
                pResponse.fBeforeResponse();

            rc_api_server_response_t pServerResponse;
            memset(&pServerResponse, 0, sizeof(pServerResponse));
            pServerResponse.http_status_code = pResponse.nHttpStatus;
            pServerResponse.body = pResponse.sResponseBody.c_str();
            pServerResponse.body_length = pResponse.sResponseBody.length();

            fCallback(pServerResponse, pCallbackData);
            return;
        }
    }

    auto& pResponse = m_vResponses.emplace_back();
    pResponse.sRequestParams = sRequestParams;
    pResponse.fAsyncCallback = fCallback;
    pResponse.pAsyncCallbackData = pCallbackData;
}

void MockRcClient::MockResponse(const std::string& sRequestParams, const std::string& sResponseBody)
{
    for (auto& pResponse : m_vResponses)
    {
        // if there's already a response queued for these params, replace it.
        if (pResponse.sRequestParams == sRequestParams)
        {
            pResponse.sResponseBody = sResponseBody;
            pResponse.nHttpStatus = 200;

            // if a callback was captured for these params, the request has already been
            // made. call the callback now (simulates async responses).
            if (pResponse.fAsyncCallback)
            {
                rc_api_server_response_t pServerResponse;
                memset(&pServerResponse, 0, sizeof(pServerResponse));
                pServerResponse.http_status_code = pResponse.nHttpStatus;
                pServerResponse.body = pResponse.sResponseBody.c_str();
                pServerResponse.body_length = pResponse.sResponseBody.length();

                pResponse.fAsyncCallback(pServerResponse, pResponse.pAsyncCallbackData);

                pResponse.fAsyncCallback = nullptr;
                pResponse.pAsyncCallbackData = nullptr;
            }

            return;
        }
    }

    // no handler yet for these params, add one.
    auto& pNewResponse = m_vResponses.emplace_back();
    pNewResponse.sRequestParams = sRequestParams;
    pNewResponse.sResponseBody = sResponseBody;
    pNewResponse.nHttpStatus = 200;
}

void MockRcClient::OnBeforeResponse(const std::string& sRequestParams, const std::function<void()>&& fHandler)
{
    for (auto& pResponse : m_vResponses)
    {
        if (pResponse.sRequestParams == sRequestParams)
        {
            pResponse.fBeforeResponse = fHandler;
            return;
        }
    }

    Assert::Fail(ra::util::String::Printf(L"No response registered for: %s", sRequestParams).c_str());
}

void MockRcClient::AssertCalled(const std::string& sRequestParams) const
{
    for (auto& pResponse : m_vResponses)
    {
        if (pResponse.sRequestParams == sRequestParams)
        {
            Assert::IsTrue(pResponse.bSeen);
            return;
        }
    }

    Assert::Fail(ra::util::String::Printf(L"Could not find mock response for %s", sRequestParams).c_str());
}

void MockRcClient::AssertNoPendingRequests() const
{
    for (auto& pResponse : m_vResponses)
    {
        if (pResponse.fAsyncCallback != nullptr && !pResponse.bSeen)
        {
            Assert::Fail(ra::util::String::Printf(L"Unexpected request pending for %s", pResponse.sRequestParams).c_str());
            return;
        }
    }
}

} // namespace mocks
} // namespace context
} // namespace ra
