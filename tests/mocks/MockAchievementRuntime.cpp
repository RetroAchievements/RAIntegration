#include "MockAchievementRuntime.hh"

#include "MockEmulatorContext.hh"
#include "MockGameContext.hh"
#include "services\ServiceLocator.hh"

#include "CppUnitTest.h"
#include "RA_StringUtils.h"

#include "data\context\GameAssets.hh"

#include <rcheevos\src\rapi\rc_api_common.h>
#include <rcheevos\src\rc_client_internal.h>

namespace ra {
namespace services {
namespace mocks {

void MockAchievementRuntime::OnBeforeResponse(const std::string& sRequestParams, std::function<void()>&& fHandler)
{
    for (auto& pResponse : m_vResponses)
    {
        if (pResponse.sRequestParams == sRequestParams)
        {
            pResponse.fBeforeResponse = fHandler;
            return;
        }
    }

    Microsoft::VisualStudio::CppUnitTestFramework::Assert::Fail(
        ra::StringPrintf(L"No response registered for: %s", sRequestParams).c_str());
}

void MockAchievementRuntime::MockUser(const std::string& sUsername, const std::string& sApiToken)
{
    m_sUsername = sUsername;
    m_sApiToken = sApiToken;
    rc_client_t* pClient = GetClient();
    pClient->user.username = m_sUsername.c_str();
    pClient->user.display_name = m_sUsername.c_str();
    pClient->user.token = m_sApiToken.c_str();
    pClient->state.user = RC_CLIENT_USER_STATE_LOGGED_IN;
}

void MockAchievementRuntime::MockGame()
{
    rc_client_game_info_t* game = (rc_client_game_info_t*)calloc(1, sizeof(rc_client_game_info_t));
    Expects(game != nullptr);
    rc_buffer_init(&game->buffer);
    rc_runtime_init(&game->runtime);

    game->public_.id = 1;
    game->public_.console_id = RC_CONSOLE_NINTENDO;
    game->public_.badge_name = "012345";
    game->public_.title = "Game Title";

    GetClient()->game = game;
}

static rc_client_subset_info_t* GetSubset(rc_client_game_info_t* game, uint32_t subset_id, const char* name)
{
    rc_client_subset_info_t* subset = game->subsets, **next = &game->subsets;
    for (; subset; subset = subset->next)
    {
        if (subset->public_.id == subset_id)
            return subset;

        next = &subset->next;
    }

    subset = (rc_client_subset_info_t*)rc_buffer_alloc(&game->buffer, sizeof(rc_client_subset_info_t));
    memset(subset, 0, sizeof(*subset));
    subset->public_.id = subset_id;
    strcpy_s(subset->public_.badge_name, sizeof(subset->public_.badge_name), game->public_.badge_name);
    subset->public_.title = name;
    subset->active = 1;

    *next = subset;

    return subset;
}

static rc_client_subset_info_t* GetCoreSubset(rc_client_game_info_t* game)
{
    Microsoft::VisualStudio::CppUnitTestFramework::Assert::IsNotNull(game, L"MockGame must be called first");
    return GetSubset(game, game->public_.id, game->public_.title);
}

static rc_client_subset_info_t* GetLocalSubset(rc_client_game_info_t* game)
{
    Microsoft::VisualStudio::CppUnitTestFramework::Assert::IsNotNull(game, L"MockGame must be called first");
    return GetSubset(game, ra::data::context::GameAssets::LocalSubsetId, "Local");
}

static rc_client_achievement_info_t* AddAchievement(rc_client_game_info_t* game,
        rc_client_subset_info_t* subset, uint32_t nId, const char* sTitle)
{
    if (subset->public_.num_achievements % 8 == 0)
    {
        const uint32_t new_count = subset->public_.num_achievements + 8;
        rc_client_achievement_info_t* new_achievements = (rc_client_achievement_info_t*)rc_buffer_alloc(
            &game->buffer, sizeof(rc_client_achievement_info_t) * new_count);

        if (subset->public_.num_achievements > 0)
        {
            memcpy(new_achievements, subset->achievements,
                   sizeof(rc_client_achievement_info_t) * subset->public_.num_achievements);
        }

        subset->achievements = new_achievements;
    }

    rc_client_achievement_info_t* achievement = &subset->achievements[subset->public_.num_achievements++];
    memset(achievement, 0, sizeof(*achievement));
    achievement->public_.id = nId;

    if (sTitle)
    {
        achievement->public_.title = rc_buffer_strcpy(&game->buffer, sTitle);
    }
    else
    {
        const std::string sGeneratedTitle = ra::StringPrintf("Achievement %u", nId);
        achievement->public_.title = rc_buffer_strcpy(&game->buffer, sGeneratedTitle.c_str());
    }

    const std::string sGeneratedDescripton = ra::StringPrintf("Description %u", nId);
    achievement->public_.description = rc_buffer_strcpy(&game->buffer, sGeneratedDescripton.c_str());

    achievement->public_.category = RC_CLIENT_ACHIEVEMENT_CATEGORY_CORE;
    achievement->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
    achievement->public_.points = 5;
    achievement->author = "Author";

    return achievement;
}

void MockAchievementRuntime::MockSubset(uint32_t nSubsetId, const std::string& sName)
{
    rc_client_game_info_t* game = GetClient()->game;
    auto* pSubset = GetSubset(game, nSubsetId, sName.c_str());
    // create a copy of the name in case it goes out of scope
    pSubset->public_.title = rc_buffer_strcpy(&game->buffer, sName.c_str());
}

rc_client_achievement_info_t* MockAchievementRuntime::MockAchievement(uint32_t nId, const char* sTitle)
{
    rc_client_game_info_t* game = GetClient()->game;
    return AddAchievement(game, GetCoreSubset(game), nId, sTitle);
}

rc_client_achievement_info_t* MockAchievementRuntime::MockSubsetAchievement(uint32_t nSubsetId, uint32_t nId, const char* sTitle)
{
    rc_client_game_info_t* game = GetClient()->game;
    return AddAchievement(game, GetSubset(game, nSubsetId, sTitle), nId, sTitle);
}

rc_client_achievement_info_t* MockAchievementRuntime::MockAchievementWithTrigger(uint32_t nId, const char* sTitle)
{
    rc_client_game_info_t* game = GetClient()->game;
    rc_client_achievement_info_t* achievement = AddAchievement(game, GetCoreSubset(game), nId, sTitle);

    achievement->trigger = (rc_trigger_t*)rc_buffer_alloc(&game->buffer, sizeof(rc_trigger_t));
    memset(achievement->trigger, 0, sizeof(*achievement->trigger));
    achievement->trigger->state = RC_TRIGGER_STATE_ACTIVE;

    return achievement;
}

rc_client_achievement_info_t* MockAchievementRuntime::ActivateAchievement(uint32_t nId, const std::string& sTrigger)
{
    rc_client_game_info_t* game = GetClient()->game;
    rc_client_achievement_info_t* achievement = AddAchievement(game, GetCoreSubset(game), nId, "Achievement");

    m_mAchievementDefinitions[nId] = sTrigger;

    auto nSize = rc_trigger_size(sTrigger.c_str());
    if (nSize > 0)
    {
        void* trigger_buffer = rc_buffer_alloc(&game->buffer, nSize);
        achievement->trigger = rc_parse_trigger(trigger_buffer, sTrigger.c_str(), nullptr, 0);
        achievement->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
    }

    return achievement;
}

void MockAchievementRuntime::UnlockAchievement(rc_client_achievement_info_t* pAchievement, int nMode)
{
    pAchievement->public_.unlocked |= nMode;

    if (pAchievement->unlock_time_softcore == 0)
        pAchievement->unlock_time_softcore = time(nullptr);

    if (nMode == RC_CLIENT_ACHIEVEMENT_UNLOCKED_BOTH)
    {
        if (pAchievement->unlock_time_hardcore == 0)
            pAchievement->unlock_time_hardcore = time(nullptr);

        pAchievement->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED;

        if (pAchievement->trigger)
            pAchievement->trigger->state = RC_TRIGGER_STATE_TRIGGERED;
    }
    else if (nMode == RC_CLIENT_ACHIEVEMENT_UNLOCKED_SOFTCORE &&
             !rc_client_get_hardcore_enabled(GetClient()))
    {
        pAchievement->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED;

        if (pAchievement->trigger)
            pAchievement->trigger->state = RC_TRIGGER_STATE_TRIGGERED;
    }
}

rc_client_achievement_info_t* MockAchievementRuntime::MockUnofficialAchievement(uint32_t nId, const char* sTitle)
{
    rc_client_game_info_t* game = GetClient()->game;
    rc_client_achievement_info_t* achievement = AddAchievement(game, GetCoreSubset(game), nId, sTitle);
    achievement->public_.category = RC_CLIENT_ACHIEVEMENT_CATEGORY_UNOFFICIAL;
    return achievement;
}

rc_client_achievement_info_t* MockAchievementRuntime::MockLocalAchievement(uint32_t nId, const char* sTitle)
{
    rc_client_game_info_t* game = GetClient()->game;
    return AddAchievement(game, GetLocalSubset(game), nId, sTitle);
}

static rc_client_leaderboard_info_t* AddLeaderboard(rc_client_t* client, rc_client_game_info_t* game,
    rc_client_subset_info_t* subset, uint32_t nId, const char* sTitle)
{
    if (subset->public_.num_leaderboards % 8 == 0)
    {
        const uint32_t new_count = subset->public_.num_leaderboards + 8;
        rc_client_leaderboard_info_t* new_leaderboards = (rc_client_leaderboard_info_t*)rc_buffer_alloc(
            &game->buffer, sizeof(rc_client_leaderboard_info_t) * new_count);

        if (subset->public_.num_leaderboards > 0)
        {
            memcpy(new_leaderboards, subset->leaderboards,
                   sizeof(rc_client_leaderboard_info_t) * subset->public_.num_leaderboards);
        }

        subset->leaderboards = new_leaderboards;
    }

    rc_client_leaderboard_info_t* leaderboard = &subset->leaderboards[subset->public_.num_leaderboards++];
    memset(leaderboard, 0, sizeof(*leaderboard));
    leaderboard->public_.id = nId;

    if (sTitle)
    {
        leaderboard->public_.title = rc_buffer_strcpy(&game->buffer, sTitle);
    }
    else
    {
        const std::string sGeneratedTitle = ra::StringPrintf("Leaderboard %u", nId);
        leaderboard->public_.title = rc_buffer_strcpy(&game->buffer, sGeneratedTitle.c_str());
    }

    const std::string sGeneratedDescripton = ra::StringPrintf("Description %u", nId);
    leaderboard->public_.description = rc_buffer_strcpy(&game->buffer, sGeneratedDescripton.c_str());

    leaderboard->public_.state = static_cast<uint8_t>(rc_client_get_hardcore_enabled(client) ?
        RC_CLIENT_LEADERBOARD_STATE_ACTIVE : RC_CLIENT_LEADERBOARD_STATE_INACTIVE);

    return leaderboard;
}

rc_client_leaderboard_info_t* MockAchievementRuntime::MockLeaderboard(uint32_t nId, const char* sTitle)
{
    rc_client_game_info_t* game = GetClient()->game;
    return AddLeaderboard(GetClient(), game, GetCoreSubset(game), nId, sTitle);
}

rc_client_leaderboard_info_t* MockAchievementRuntime::MockLeaderboardWithLboard(uint32_t nId, const char* sTitle)
{
    rc_client_game_info_t* game = GetClient()->game;
    rc_client_leaderboard_info_t* leaderboard = AddLeaderboard(GetClient(), game, GetCoreSubset(game), nId, sTitle);

    leaderboard->lboard = (rc_lboard_t*)rc_buffer_alloc(&game->buffer, sizeof(rc_lboard_t));
    memset(leaderboard->lboard, 0, sizeof(*leaderboard->lboard));
    leaderboard->lboard->state = static_cast<uint8_t>(
        rc_client_get_hardcore_enabled(GetClient()) ? RC_LBOARD_STATE_ACTIVE : RC_LBOARD_STATE_INACTIVE);

    return leaderboard;
}

rc_client_leaderboard_info_t* MockAchievementRuntime::MockLocalLeaderboard(uint32_t nId, const char* sTitle)
{
    rc_client_game_info_t* game = GetClient()->game;
    return AddLeaderboard(GetClient(), game, GetLocalSubset(game), nId, sTitle);
}

rc_client_leaderboard_info_t* MockAchievementRuntime::ActivateLeaderboard(uint32_t nId, const std::string& sDefinition)
{
    rc_client_game_info_t* game = GetClient()->game;
    rc_client_leaderboard_info_t* leaderboard = AddLeaderboard(GetClient(), game, GetCoreSubset(game), nId, "Leaderboard");

    m_mLeaderboardDefinitions[nId] = sDefinition;

    auto nSize = rc_lboard_size(sDefinition.c_str());
    void* trigger_buffer = rc_buffer_alloc(&game->buffer, nSize);
    leaderboard->lboard = rc_parse_lboard(trigger_buffer, sDefinition.c_str(), nullptr, 0);
    leaderboard->public_.state = RC_CLIENT_LEADERBOARD_STATE_ACTIVE;

    return leaderboard;
}

void MockAchievementRuntime::AssertCalled(const std::string& sRequestParams) const
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

void MockAchievementRuntime::AssertNoPendingRequests() const
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

void MockAchievementRuntime::MockServerCall(const rc_api_request_t* pRequest,
     rc_client_server_callback_t fCallback, void* pCallbackData, rc_client_t*)
{
    auto* pClient = dynamic_cast<MockAchievementRuntime*>(&ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>());
    std::string sRequestParams = pRequest->post_data;

    for (auto& pResponse : pClient->m_vResponses)
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

            fCallback(&pServerResponse, pCallbackData);
            return;
        }
    }

    auto& pResponse = pClient->m_vResponses.emplace_back();
    pResponse.sRequestParams = sRequestParams;
    pResponse.fAsyncCallback = fCallback;
    pResponse.pAsyncCallbackData = pCallbackData;
}

} // namespace mocks
} // namespace services

namespace data {
namespace context {
namespace mocks {

void MockEmulatorContext::SetRuntimeMemorySize(size_t nBytes)
{
    if (ra::services::ServiceLocator::Exists<ra::services::AchievementRuntime>())
    {
        auto* pGame = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>().GetClient()->game;
        if (pGame && pGame->max_valid_address == 0U)
            pGame->max_valid_address = gsl::narrow_cast<uint32_t>(nBytes);
    }
}

void MockGameContext::InitializeFromAchievementRuntime()
{
    auto* pMockRuntime = dynamic_cast<ra::services::mocks::MockAchievementRuntime*>(
        &ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>());

    if (pMockRuntime != nullptr)
    {
        GameContext::InitializeFromAchievementRuntime(pMockRuntime->GetAchievementDefinitions(),
                                                      pMockRuntime->GetLeaderboardDefinitions());
    }
    else
    {
        std::map<uint32_t, std::string> mAchievementDefinitions;
        std::map<uint32_t, std::string> mLeaderboardDefinitions;
        GameContext::InitializeFromAchievementRuntime(mAchievementDefinitions, mLeaderboardDefinitions);
    }
}

} // namespace mocks
} // namespace context
} // namespace data
} // namespace ra

