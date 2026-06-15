#ifndef RA_SERVICES_MOCK_RCCLIENT_HH
#define RA_SERVICES_MOCK_RCCLIENT_HH
#pragma once

#include "context\impl\RcClient.hh"

#include "services\ServiceLocator.hh"

typedef struct rc_client_achievement_info_t rc_client_achievement_info_t;
typedef struct rc_client_leaderboard_info_t rc_client_leaderboard_info_t;

namespace ra {
namespace context {
namespace mocks {

class MockRcClient : public impl::RcClient
{
public:
    MockRcClient() noexcept;

    void AddAuthentication(const char** username, const char** api_token) const override;

    void DispatchRequest(const rc_api_request_t& pRequest, std::function<void(const rc_api_server_response_t&, void*)> fCallback, void* fCallbackData) const override;
    void SendRequest(const rc_api_request_t& pRequest, rc_api_server_response_t& pResponse, std::string& sResponseBuffer) const override;

    void MockResponse(const std::string& sRequestParams, const std::string& sResponseBody, int nHttpStatusCode = 200);
    bool HasMockResponse(const std::string& sRequestParams) const noexcept;

    void OnBeforeResponse(const std::string& sRequestParams, const std::function<void()>&& fHandler);

    void AssertNoUnhandled() const;
    void AssertCalled(const std::string& sRequestParams) const;
    void AssertNoPendingRequests() const;

    void SetHardcoreEnabled(bool bValue) noexcept;

    void MockGame(uint32_t nGameId, const char* title, uint32_t nConsoleId = 1);
    rc_client_achievement_info_t* MockAchievement(uint32_t nId, const char* sTitle = nullptr);
    rc_client_leaderboard_info_t* MockLeaderboard(uint32_t nId, const char* sTitle = nullptr);

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
