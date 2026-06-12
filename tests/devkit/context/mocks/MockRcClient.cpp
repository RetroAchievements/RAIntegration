#include "MockRcClient.hh"

#include "context/UserContext.hh"

#include "services/ServiceLocator.hh"

#include "tests/devkit/testutil/CppUnitTest.hh"

#include "util/Strings.hh"

#include <rcheevos/src/rc_client_internal.h>

namespace ra {
namespace context {
namespace mocks {

MockRcClient::MockRcClient() noexcept : m_Override(this)
{
    /* disable logging */
    rc_client_enable_logging(GetClient(), RC_CLIENT_LOG_LEVEL_NONE, nullptr);
}

void MockRcClient::AddAuthentication(const char** pUsername, const char** pApiToken) const
{
    if (ra::services::ServiceLocator::Exists<ra::context::UserContext>())
    {
        RcClient::AddAuthentication(pUsername, pApiToken);
    }
    else
    {
        if (pUsername)
            *pUsername = "USERNAME";
        if (pApiToken)
            *pApiToken = "APITOKEN";
    }
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

bool MockRcClient::HasMockResponse(const std::string& sRequestParams) const noexcept
{
    for (auto& pResponse : m_vResponses)
    {
        if (pResponse.sRequestParams == sRequestParams)
            return true;
    }

    return false;
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

void MockRcClient::AssertNoUnhandled() const
{
    for (auto& pResponse : m_vResponses)
    {
        if (pResponse.fAsyncCallback != nullptr)
        {
            // pending callback means the request was not handled
            Assert::Fail(ra::util::String::Printf(L"Unhandled request: %s", pResponse.sRequestParams).c_str());
        }
    }
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

void MockRcClient::SetHardcoreEnabled(bool bValue) noexcept
{
    // just set the state; don't worry about all the management stuff rc_client_set_hardcore_enabled does
    auto* pClient = GetClient();
    pClient->state.hardcore = bValue ? 1 : 0;
}

void MockRcClient::MockGame(uint32_t nGameId, const char* title, uint32_t nConsoleId)
{
    rc_client_game_info_t* pGame = static_cast<rc_client_game_info_t*>(calloc(1, sizeof(*pGame)));
    Assert::IsNotNull(pGame);
    Ensures(pGame != nullptr);

    rc_buffer_init(&pGame->buffer);
    rc_runtime_init(&pGame->runtime);

    pGame->public_.id = nGameId;
    pGame->public_.console_id = nConsoleId;
    pGame->public_.title = title;
    pGame->public_.badge_name = "55443";

    auto* pClient = GetClient();
    rc_client_unload_game(pClient);

    pClient->game = pGame;
    pClient->state.frames_processed = pClient->state.frames_at_last_ping = 0;
}

static rc_client_subset_info_t* GetSubset(rc_client_game_info_t* game, uint32_t subset_id, const char* name) noexcept
{
    rc_client_subset_info_t* subset = game->subsets, ** next = &game->subsets;
    for (; subset; subset = subset->next)
    {
        if (subset->public_.id == subset_id)
            return subset;

        next = &subset->next;
    }

    subset = static_cast<rc_client_subset_info_t*>(rc_buffer_alloc(&game->buffer, sizeof(rc_client_subset_info_t)));
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
    Assert::IsNotNull(game, L"MockGame must be called first");
    return GetSubset(game, game->public_.id, game->public_.title);
}

static rc_client_achievement_info_t* AddAchievement(rc_client_game_info_t* game,
    rc_client_subset_info_t* subset, uint32_t nId, const char* sTitle)
{
    if (subset->public_.num_achievements % 8 == 0)
    {
        const uint32_t new_count = subset->public_.num_achievements + 8;
        rc_client_achievement_info_t* new_achievements = static_cast<rc_client_achievement_info_t*>(rc_buffer_alloc(
            &game->buffer, sizeof(rc_client_achievement_info_t) * new_count));

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
        const std::string sGeneratedTitle = ra::util::String::Printf("Achievement %u", nId);
        achievement->public_.title = rc_buffer_strcpy(&game->buffer, sGeneratedTitle.c_str());
    }

    const std::string sGeneratedDescripton = ra::util::String::Printf("Description %u", nId);
    achievement->public_.description = rc_buffer_strcpy(&game->buffer, sGeneratedDescripton.c_str());

    achievement->public_.category = RC_CLIENT_ACHIEVEMENT_CATEGORY_CORE;
    achievement->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
    achievement->public_.points = 5;
    achievement->author = "Author";

    return achievement;
}

rc_client_achievement_info_t* MockRcClient::MockAchievement(uint32_t nId, const char* sTitle)
{
    rc_client_game_info_t* game = GetClient()->game;
    rc_client_achievement_info_t* achievement = AddAchievement(game, GetCoreSubset(game), nId, sTitle);

    achievement->trigger = static_cast<rc_trigger_t*>(rc_buffer_alloc(&game->buffer, sizeof(rc_trigger_t)));
    memset(achievement->trigger, 0, sizeof(*achievement->trigger));
    achievement->trigger->state = RC_TRIGGER_STATE_ACTIVE;

    return achievement;
}

static rc_client_leaderboard_info_t* AddLeaderboard(const rc_client_t* client, rc_client_game_info_t* game,
    rc_client_subset_info_t* subset, uint32_t nId, const char* sTitle)
{
    if (subset->public_.num_leaderboards % 8 == 0)
    {
        const uint32_t new_count = subset->public_.num_leaderboards + 8;
        rc_client_leaderboard_info_t* new_leaderboards = static_cast<rc_client_leaderboard_info_t*>(rc_buffer_alloc(
            &game->buffer, sizeof(rc_client_leaderboard_info_t) * new_count));

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
        const std::string sGeneratedTitle = ra::util::String::Printf("Leaderboard %u", nId);
        leaderboard->public_.title = rc_buffer_strcpy(&game->buffer, sGeneratedTitle.c_str());
    }

    const std::string sGeneratedDescripton = ra::util::String::Printf("Description %u", nId);
    leaderboard->public_.description = rc_buffer_strcpy(&game->buffer, sGeneratedDescripton.c_str());

    leaderboard->public_.state = static_cast<uint8_t>(rc_client_get_hardcore_enabled(client) ?
        RC_CLIENT_LEADERBOARD_STATE_ACTIVE : RC_CLIENT_LEADERBOARD_STATE_INACTIVE);

    return leaderboard;
}

rc_client_leaderboard_info_t* MockRcClient::MockLeaderboard(uint32_t nId, const char* sTitle)
{
    rc_client_game_info_t* game = GetClient()->game;
    rc_client_leaderboard_info_t* leaderboard = AddLeaderboard(GetClient(), game, GetCoreSubset(game), nId, sTitle);

    leaderboard->lboard = static_cast<rc_lboard_t*>(rc_buffer_alloc(&game->buffer, sizeof(rc_lboard_t)));
    memset(leaderboard->lboard, 0, sizeof(*leaderboard->lboard));
    leaderboard->lboard->state = static_cast<uint8_t>(
        rc_client_get_hardcore_enabled(GetClient()) ? RC_LBOARD_STATE_ACTIVE : RC_LBOARD_STATE_INACTIVE);

    return leaderboard;
}

} // namespace mocks
} // namespace context
} // namespace ra
