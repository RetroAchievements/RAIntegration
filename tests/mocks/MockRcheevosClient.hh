#ifndef RA_SERVICES_MOCK_RCHEEVOS_CLIENT_HH
#define RA_SERVICES_MOCK_RCHEEVOS_CLIENT_HH
#pragma once

#include "services\RcheevosClient.hh"
#include "services\ServiceLocator.hh"

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

    void OnBeforeResponse(const std::string& sRequestParams, std::function<void()>&& fHandler);

    void MockUser(const std::string& sUsername, const std::string& sApiToken);

    void MockGame();
    rc_client_achievement_info_t* MockAchievement(uint32_t nId, const char* sTitle = nullptr);
    rc_client_achievement_info_t* MockAchievementWithTrigger(uint32_t nId, const char* sTitle = nullptr);
    rc_client_achievement_info_t* MockUnofficialAchievement(uint32_t nId, const char* sTitle = nullptr);
    rc_client_achievement_info_t* MockLocalAchievement(uint32_t nId, const char* sTitle = nullptr);

    rc_client_leaderboard_info_t* MockLeaderboard(uint32_t nId, const char* sTitle = nullptr);
    rc_client_leaderboard_info_t* MockLocalLeaderboard(uint32_t nId, const char* sTitle = nullptr);

    void AssertCalled(const std::string& sRequestParams) const;

    void AssertNoPendingRequests() const;

private:
    static void MockServerCall(const rc_api_request_t* pRequest, rc_client_server_callback_t fCallback,
                               void* pCallbackData, rc_client_t*);

    ra::services::ServiceLocator::ServiceOverride<ra::services::RcheevosClient> m_Override;

    typedef struct MockApiResponse
    {
        std::string sRequestParams;
        std::string sResponseBody;
        int nHttpStatus = 0;
        bool bSeen = 0;
        rc_client_server_callback_t fAsyncCallback = nullptr;
        void* pAsyncCallbackData = nullptr;
        std::function<void()> fBeforeResponse = nullptr;
    } MockApiResponse;

    std::vector<MockApiResponse> m_vResponses;

    std::string m_sUsername;
    std::string m_sApiToken;
};

} // namespace mocks
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_MOCK_RCHEEVOS_CLIENT_HH
