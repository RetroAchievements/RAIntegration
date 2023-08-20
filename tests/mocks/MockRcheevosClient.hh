#ifndef RA_SERVICES_MOCK_RCHEEVOS_CLIENT_HH
#define RA_SERVICES_MOCK_RCHEEVOS_CLIENT_HH
#pragma once

#include "services\RcheevosClient.hh"
#include "services\ServiceLocator.hh"

#include "CppUnitTest.h"
#include "RA_StringUtils.h"

#include <rcheevos\src\rcheevos\rc_client_internal.h>

namespace ra {
namespace services {
namespace mocks {

class MockRcheevosClient : public RcheevosClient
{
public:
    MockRcheevosClient() noexcept : m_Override(this)
    {
        /* intercept server requests */
        GetClient()->callbacks.server_call = MockRcheevosClient::MockServerCall;

        /* disable logging */
        rc_client_enable_logging(GetClient(), RC_CLIENT_LOG_LEVEL_NONE, nullptr);
    }

    void MockResponse(const std::string& sRequestParams, const std::string& sResponseBody)
    {
        auto& pResponse = m_vResponses.emplace_back();
        pResponse.sRequestParams = sRequestParams;
        pResponse.sResponseBody = sResponseBody;
        pResponse.nHttpStatus = 200;
    }

    void AssertCalled(const std::string& sRequestParams) const
    {
        for (auto& pResponse : m_vResponses)
        {
            if (pResponse.sRequestParams == sRequestParams)
            {
                Microsoft::VisualStudio::CppUnitTestFramework::Assert::IsTrue(pResponse.bSeen);
                return;
            }
        }

        Microsoft::VisualStudio::CppUnitTestFramework::Assert::Fail(
            ra::StringPrintf(L"Could not find mock response for %s", sRequestParams).c_str());
    }

    void AssertNoPendingRequests() const
    {
        for (auto& pResponse : m_vResponses)
        {
            if (pResponse.fAsyncCallback != nullptr && !pResponse.bSeen)
            {
                Microsoft::VisualStudio::CppUnitTestFramework::Assert::Fail(
                    ra::StringPrintf(L"Unexpected request pending for %s", pResponse.sRequestParams).c_str());
                return;
            }
        }
    }

private:
    static void MockServerCall(const rc_api_request_t* pRequest, rc_client_server_callback_t fCallback,
                               void* pCallbackData, rc_client_t*)
    {
        auto* pClient = dynamic_cast<MockRcheevosClient*>(&ra::services::ServiceLocator::GetMutable<ra::services::RcheevosClient>());
        std::string sRequestParams = pRequest->post_data;

        for (auto& pResponse : pClient->m_vResponses)
        {
            if (pResponse.sRequestParams == sRequestParams)
            {
                pResponse.bSeen = true;

                rc_api_server_response_t pServerResponse;
                memset(&pServerResponse, 0, sizeof(pServerResponse));
                pServerResponse.http_status_code = pResponse.nHttpStatus;
                pServerResponse.body = pResponse.sResponseBody.c_str();
                pServerResponse.body_length = pResponse.sResponseBody.length();

                fCallback(&pServerResponse, pCallbackData);
                return;
            }
        }

        auto& pResponse = pClient->m_vResponses.emplace_back();
        pResponse.sRequestParams = sRequestParams;
        pResponse.fAsyncCallback = fCallback;
        pResponse.pAsyncCallbackData = pCallbackData;
    }

    ra::services::ServiceLocator::ServiceOverride<ra::services::RcheevosClient> m_Override;

    typedef struct MockApiResponse
    {
        std::string sRequestParams;
        std::string sResponseBody;
        int nHttpStatus = 0;
        bool bSeen = 0;
        rc_client_server_callback_t fAsyncCallback = nullptr;
        void* pAsyncCallbackData = nullptr;
    } MockApiResponse;

    std::vector<MockApiResponse> m_vResponses;
};

} // namespace mocks
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_MOCK_RCHEEVOS_CLIENT_HH
