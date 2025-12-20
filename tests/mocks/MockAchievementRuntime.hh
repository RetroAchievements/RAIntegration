#ifndef RA_SERVICES_MOCK_ACHIEVEMENT_RUNTIME_HH
#define RA_SERVICES_MOCK_ACHIEVEMENT_RUNTIME_HH
#pragma once

#include "context\IRcClient.hh"

#include "services\AchievementRuntime.hh"
#include "services\ServiceLocator.hh"

#include <rcheevos\src\rc_client_internal.h>

#include "tests\devkit\testutil\CppUnitTest.hh"

namespace ra {
namespace services {
namespace mocks {

class MockAchievementRuntime : public AchievementRuntime
{
public:
    GSL_SUPPRESS_F6 MockAchievementRuntime()
        : AchievementRuntime(false), m_Override(this)
    {
        Assert::IsTrue(ra::services::ServiceLocator::Exists<context::IRcClient>(), L"MockAchievementRuntime requires MockRcClient");

        InitializeRcClient();
    }

    rc_client_t* GetClient() const
    {
        return ra::services::ServiceLocator::Get<ra::context::IRcClient>().GetClient();
    }

    bool IsAchievementSupported(ra::AchievementID nAchievement)
    {
        const auto* pAchievement = GetAchievementTrigger(nAchievement);
        return (pAchievement != nullptr && pAchievement->state != RC_TRIGGER_STATE_DISABLED);
    }

    void MockUser(const std::string& sUsername, const std::string& sApiToken);

    void MockGame();

    void MockSubset(uint32_t nSubsetId, const std::string& sName);

    rc_client_achievement_info_t* MockAchievement(uint32_t nId, const char* sTitle = nullptr);
    rc_client_achievement_info_t* MockAchievementWithTrigger(uint32_t nId, const char* sTitle = nullptr);
    rc_client_achievement_info_t* MockUnofficialAchievement(uint32_t nId, const char* sTitle = nullptr);
    rc_client_achievement_info_t* MockLocalAchievement(uint32_t nId, const char* sTitle = nullptr);
    rc_client_achievement_info_t* MockSubsetAchievement(uint32_t nSubsetId, uint32_t nId, const char* sTitle = nullptr);

    rc_client_achievement_info_t* ActivateAchievement(uint32_t nId, const std::string& sTrigger);
    void UnlockAchievement(rc_client_achievement_info_t* pAchievement, int nMode);

    rc_client_leaderboard_info_t* MockLeaderboard(uint32_t nId, const char* sTitle = nullptr);
    rc_client_leaderboard_info_t* MockLeaderboardWithLboard(uint32_t nId, const char* sTitle = nullptr);
    rc_client_leaderboard_info_t* MockLocalLeaderboard(uint32_t nId, const char* sTitle = nullptr);

    rc_client_leaderboard_info_t* ActivateLeaderboard(uint32_t nId, const std::string& sDefinition);

    std::map<uint32_t, std::string>& GetAchievementDefinitions() noexcept { return m_mAchievementDefinitions; }
    std::map<uint32_t, std::string>& GetLeaderboardDefinitions() noexcept { return m_mLeaderboardDefinitions; }

private:
    ra::services::ServiceLocator::ServiceOverride<ra::services::AchievementRuntime> m_Override;

    std::map<uint32_t, std::string> m_mAchievementDefinitions;
    std::map<uint32_t, std::string> m_mLeaderboardDefinitions;

    std::string m_sUsername;
    std::string m_sApiToken;
};

} // namespace mocks
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_MOCK_ACHIEVEMENT_RUNTIME_HH
