#ifndef RA_SERVICES_MOCK_RCCLIENT_HH
#define RA_SERVICES_MOCK_RCCLIENT_HH
#pragma once

#include "context\impl\RcClient.hh"

#include "services\ServiceLocator.hh"

namespace ra {
namespace context {
namespace mocks {

class MockRcClient : public impl::RcClient
{
public:
    MockRcClient() noexcept;

    void AddAuthentication(const char** username, const char** api_token) const override;

    void DispatchRequest(const rc_api_request_t& pRequest, std::function<void(const rc_api_server_response_t&, void*)> fCallback, void* fCallbackData) const override;

    void MockResponse(const std::string& sRequestParams, const std::string& sResponseBody);

    void OnBeforeResponse(const std::string& sRequestParams, const std::function<void()>&& fHandler);

    void AssertNoUnhandled() const;
    void AssertCalled(const std::string& sRequestParams) const;
    void AssertNoPendingRequests() const;

private:
    typedef struct MockApiResponse
    {
        std::string sRequestParams;
        std::string sResponseBody;
        int nHttpStatus = 0;
        bool bSeen = 0;
        std::function<void(const rc_api_server_response_t&, void*)> fAsyncCallback = nullptr;
        void* pAsyncCallbackData = nullptr;
        std::function<void()> fBeforeResponse = nullptr;
    } MockApiResponse;

    mutable std::vector<MockApiResponse> m_vResponses;

    ra::services::ServiceLocator::ServiceOverride<IRcClient> m_Override;
};

} // namespace mocks
} // namespace context
} // namespace ra

#endif // !RA_SERVICES_MOCK_RCCLIENT_HH
