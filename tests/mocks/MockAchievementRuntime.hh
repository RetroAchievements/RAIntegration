#ifndef RA_SERVICES_MOCK_ACHIEVEMENT_RUNTIME_HH
#define RA_SERVICES_MOCK_ACHIEVEMENT_RUNTIME_HH
#pragma once

#include "services\AchievementRuntime.hh"
#include "services\ServiceLocator.hh"

#include <rcheevos\src\rcheevos\rc_client_internal.h>

namespace ra {
namespace services {
namespace mocks {

class MockAchievementRuntime : public AchievementRuntime
{
public:
    MockAchievementRuntime() noexcept : m_Override(this)
    {
        /* intercept server requests */
        GetClient()->callbacks.server_call = MockServerCall;

        /* disable logging */
        rc_client_enable_logging(GetClient(), RC_CLIENT_LOG_LEVEL_NONE, nullptr);
    }

    bool IsAchievementSupported(ra::AchievementID nAchievement) noexcept
    {
        const auto* pAchievement = GetAchievementTrigger(nAchievement);
        return (pAchievement != nullptr && pAchievement->state != RC_TRIGGER_STATE_DISABLED);
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

    rc_client_achievement_info_t* ActivateAchievement(uint32_t nId, const std::string& sTrigger);
    void UnlockAchievement(rc_client_achievement_info_t* pAchievement, int nMode);

    rc_client_leaderboard_info_t* MockLeaderboard(uint32_t nId, const char* sTitle = nullptr);
    rc_client_leaderboard_info_t* MockLeaderboardWithLboard(uint32_t nId, const char* sTitle = nullptr);
    rc_client_leaderboard_info_t* MockLocalLeaderboard(uint32_t nId, const char* sTitle = nullptr);

    rc_client_leaderboard_info_t* ActivateLeaderboard(uint32_t nId, const std::string& sDefinition);

    void AssertCalled(const std::string& sRequestParams) const;

    void AssertNoPendingRequests() const;

    std::map<uint32_t, std::string>& GetAchievementDefinitions() { return m_mAchievementDefinitions; }
    std::map<uint32_t, std::string>& GetLeaderboardDefinitions() { return m_mLeaderboardDefinitions; }

private:
    static void MockServerCall(const rc_api_request_t* pRequest, rc_client_server_callback_t fCallback,
                               void* pCallbackData, rc_client_t*);

    ra::services::ServiceLocator::ServiceOverride<ra::services::AchievementRuntime> m_Override;

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

    std::map<uint32_t, std::string> m_mAchievementDefinitions;
    std::map<uint32_t, std::string> m_mLeaderboardDefinitions;

    std::string m_sUsername;
    std::string m_sApiToken;
};

} // namespace mocks
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_MOCK_ACHIEVEMENT_RUNTIME_HH
