#include "MockRcheevosClient.hh"
#include "services\ServiceLocator.hh"

#include "CppUnitTest.h"
#include "RA_StringUtils.h"

#include "data\context\GameAssets.hh"

#include <rcheevos\src\rapi\rc_api_common.h>
#include <rcheevos\src\rcheevos\rc_client_internal.h>

namespace ra {
namespace services {
namespace mocks {

void MockRcheevosClient::OnBeforeResponse(const std::string& sRequestParams, std::function<void()>&& fHandler)
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

void MockRcheevosClient::MockUser(const std::string& sUsername, const std::string& sApiToken)
{
    m_sUsername = sUsername;
    m_sApiToken = sApiToken;
    GetClient()->user.username = m_sUsername.c_str();
    GetClient()->user.display_name = m_sUsername.c_str();
    GetClient()->user.token = m_sApiToken.c_str();
    GetClient()->state.user = RC_CLIENT_USER_STATE_LOGGED_IN;
}

void MockRcheevosClient::MockGame()
{
    rc_client_game_info_t* game = (rc_client_game_info_t*)calloc(1, sizeof(rc_client_game_info_t));
    Expects(game != nullptr);
    rc_buf_init(&game->buffer);
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

    subset = (rc_client_subset_info_t*)rc_buf_alloc(&game->buffer, sizeof(rc_client_subset_info_t));
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
        rc_client_achievement_info_t* new_achievements = (rc_client_achievement_info_t*)rc_buf_alloc(
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
        achievement->public_.title = rc_buf_strcpy(&game->buffer, sTitle);
    }
    else
    {
        const std::string sGeneratedTitle = ra::StringPrintf("Achievement %u", nId);
        achievement->public_.title = rc_buf_strcpy(&game->buffer, sGeneratedTitle.c_str());
    }

    const std::string sGeneratedDescripton = ra::StringPrintf("Description %u", nId);
    achievement->public_.description = rc_buf_strcpy(&game->buffer, sGeneratedDescripton.c_str());

    achievement->public_.category = RC_CLIENT_ACHIEVEMENT_CATEGORY_CORE;
    achievement->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
    achievement->public_.points = 5;

    return achievement;
}

rc_client_achievement_info_t* MockRcheevosClient::MockAchievement(uint32_t nId, const char* sTitle)
{
    rc_client_game_info_t* game = GetClient()->game;
    return AddAchievement(game, GetCoreSubset(game), nId, sTitle);
}

rc_client_achievement_info_t* MockRcheevosClient::MockAchievementWithTrigger(uint32_t nId, const char* sTitle)
{
    rc_client_game_info_t* game = GetClient()->game;
    rc_client_achievement_info_t* achievement = AddAchievement(game, GetCoreSubset(game), nId, sTitle);

    achievement->trigger = (rc_trigger_t*)rc_buf_alloc(&game->buffer, sizeof(rc_trigger_t));
    memset(achievement->trigger, 0, sizeof(*achievement->trigger));

    return achievement;
}

rc_client_achievement_info_t* MockRcheevosClient::MockUnofficialAchievement(uint32_t nId, const char* sTitle)
{
    rc_client_game_info_t* game = GetClient()->game;
    rc_client_achievement_info_t* achievement = AddAchievement(game, GetCoreSubset(game), nId, sTitle);
    achievement->public_.category = RC_CLIENT_ACHIEVEMENT_CATEGORY_UNOFFICIAL;
    return achievement;
}

rc_client_achievement_info_t* MockRcheevosClient::MockLocalAchievement(uint32_t nId, const char* sTitle)
{
    rc_client_game_info_t* game = GetClient()->game;
    return AddAchievement(game, GetLocalSubset(game), nId, sTitle);
}

static rc_client_leaderboard_info_t* AddLeaderboard(rc_client_game_info_t* game, rc_client_subset_info_t* subset,
                                                    uint32_t nId, const char* sTitle)
{
    if (subset->public_.num_leaderboards % 8 == 0)
    {
        const uint32_t new_count = subset->public_.num_leaderboards + 8;
        rc_client_leaderboard_info_t* new_leaderboards = (rc_client_leaderboard_info_t*)rc_buf_alloc(
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
        leaderboard->public_.title = rc_buf_strcpy(&game->buffer, sTitle);
    }
    else
    {
        const std::string sGeneratedTitle = ra::StringPrintf("Leaderboard %u", nId);
        leaderboard->public_.title = rc_buf_strcpy(&game->buffer, sGeneratedTitle.c_str());
    }

    const std::string sGeneratedDescripton = ra::StringPrintf("Description %u", nId);
    leaderboard->public_.description = rc_buf_strcpy(&game->buffer, sGeneratedDescripton.c_str());

    leaderboard->public_.state = RC_CLIENT_LEADERBOARD_STATE_ACTIVE;

    return leaderboard;
}

rc_client_leaderboard_info_t* MockRcheevosClient::MockLeaderboard(uint32_t nId, const char* sTitle)
{
    rc_client_game_info_t* game = GetClient()->game;
    return AddLeaderboard(game, GetCoreSubset(game), nId, sTitle);
}

rc_client_leaderboard_info_t* MockRcheevosClient::MockLocalLeaderboard(uint32_t nId, const char* sTitle)
{
    rc_client_game_info_t* game = GetClient()->game;
    return AddLeaderboard(game, GetLocalSubset(game), nId, sTitle);
}

void MockRcheevosClient::AssertCalled(const std::string& sRequestParams) const
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

void MockRcheevosClient::AssertNoPendingRequests() const
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

void MockRcheevosClient::MockServerCall(const rc_api_request_t* pRequest,
     rc_client_server_callback_t fCallback, void* pCallbackData, rc_client_t*)
{
    auto* pClient = dynamic_cast<MockRcheevosClient*>(&ra::services::ServiceLocator::GetMutable<ra::services::RcheevosClient>());
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
} // namespace ra
