#include "services\AchievementRuntime.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\data\DataAsserts.hh"
#include "tests\mocks\MockAudioSystem.hh"
#include "tests\mocks\MockClock.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockConsoleContext.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockFileSystem.hh"
#include "tests\mocks\MockFrameEventQueue.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockImageRepository.hh"
#include "tests\mocks\MockOverlayManager.hh"
#include "tests\mocks\MockOverlayTheme.hh"
#include "tests\mocks\MockSessionTracker.hh"
#include "tests\mocks\MockSurface.hh"
#include "tests\mocks\MockThreadPool.hh"
#include "tests\mocks\MockUserContext.hh"
#include "tests\mocks\MockWindowManager.hh"

#include "tests\ui\UIAsserts.hh"

#include <rcheevos\src\rapi\rc_api_common.h>
#include <rcheevos\src\rc_client_internal.h>

#ifdef RC_CLIENT_EXPORTS_EXTERNAL
#include <rcheevos\src\rc_client_external.h>
#include <rcheevos\include\rc_client_raintegration.h>
#include "Exports.hh"
#include "RA_Resource.h"
extern "C" API int CCONV _Rcheevos_GetExternalClient(rc_client_external_t* pClientExternal, int nVersion);
extern "C" API const rc_client_raintegration_menu_t* CCONV _Rcheevos_RAIntegrationGetMenu();
#endif

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace services {
namespace tests {

class AchievementRuntimeHarness : public AchievementRuntime
{
public:
    GSL_SUPPRESS_F6 AchievementRuntimeHarness() : m_Override(this)
    {
        mockUserContext.Initialize("User", "ApiToken");
        GetClient()->user.display_name = "UserDisplay";
        GetClient()->state.user = RC_CLIENT_USER_STATE_LOGGED_IN;
        MockGame();

        m_fRealEventHandler = GetClient()->callbacks.event_handler;
        rc_client_set_event_handler(GetClient(), CaptureEventHandler);

        rc_client_set_userdata(GetClient(), this);
    }

    ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
    ra::data::context::mocks::MockGameContext mockGameContext;
    ra::data::context::mocks::MockSessionTracker mockSessionTracker;
    ra::data::context::mocks::MockUserContext mockUserContext;
    ra::services::mocks::MockAudioSystem mockAudioSystem;
    ra::services::mocks::MockClock mockClock;
    ra::services::mocks::MockConfiguration mockConfiguration;
    ra::services::mocks::MockFileSystem mockFileSystem;
    ra::services::mocks::MockFrameEventQueue mockFrameEventQueue;
    ra::services::mocks::MockThreadPool mockThreadPool;
    ra::ui::drawing::mocks::MockSurfaceFactory mockSurfaceFactory;
    ra::ui::mocks::MockDesktop mockDesktop;
    ra::ui::mocks::MockImageRepository mockImageRepository;
    ra::ui::mocks::MockOverlayTheme mockOverlayTheme;
    ra::ui::viewmodels::mocks::MockOverlayManager mockOverlayManager;
    ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager;

    void MockGame()
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

    void ResetEvents() { m_vEvents.clear(); }

    size_t GetEventCount() const { return m_vEvents.size(); }

    void AssertEvent(uint32_t nType, uint32_t nRecordId = 0)
    {
        for (const auto& pEvent : m_vEvents)
        {
            if (pEvent.type != nType)
                continue;

            switch (nType)
            {
                case RC_CLIENT_EVENT_ACHIEVEMENT_CHALLENGE_INDICATOR_SHOW:
                case RC_CLIENT_EVENT_ACHIEVEMENT_CHALLENGE_INDICATOR_HIDE:
                case RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_SHOW:
                case RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_UPDATE:
                case RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED:
                    if (pEvent.achievement->id == nRecordId)
                        return;
                    break;

                case RC_CLIENT_EVENT_LEADERBOARD_STARTED:
                case RC_CLIENT_EVENT_LEADERBOARD_FAILED:
                case RC_CLIENT_EVENT_LEADERBOARD_SUBMITTED:
                case RC_CLIENT_EVENT_LEADERBOARD_SCOREBOARD:
                    if (pEvent.leaderboard->id == nRecordId)
                        return;
                    break;

                case RC_CLIENT_EVENT_LEADERBOARD_TRACKER_UPDATE:
                case RC_CLIENT_EVENT_LEADERBOARD_TRACKER_SHOW:
                case RC_CLIENT_EVENT_LEADERBOARD_TRACKER_HIDE:
                    if (pEvent.leaderboard_tracker->id == nRecordId)
                        return;
                    break;

                case RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_HIDE:
                case RC_CLIENT_EVENT_GAME_COMPLETED:
                    return;
            }
        }

        if (nRecordId != 0)
            Assert::Fail(ra::StringPrintf(L"Event %u not found for record %u.", nType, nRecordId).c_str());
        else
            Assert::Fail(ra::StringPrintf(L"Event %u not found.", nType).c_str());
    }

    void RaiseEvent(const rc_client_event_t& event)
    {
        m_fRealEventHandler(&event, GetClient());
    }

    rc_client_achievement_info_t* MockAchievement(uint32_t nId, const std::string& sTrigger)
    {
        auto* game = GetClient()->game;
        auto* ach = AddAchievement(game, GetCoreSubset(game), nId, ra::StringPrintf("Ach%u", nId).c_str());

        auto nSize = rc_trigger_size(sTrigger.c_str());
        void* trigger_buffer = rc_buffer_alloc(&game->buffer, nSize);
        ach->trigger = rc_parse_trigger(trigger_buffer, sTrigger.c_str(), nullptr, 0);
        ActivateAchievement(ach);

        return ach;
    }

    rc_client_achievement_info_t* MockLocalAchievement(uint32_t nId, const std::string& sTrigger)
    {
        auto* game = GetClient()->game;
        auto* ach = AddAchievement(game, GetLocalSubset(game), nId, ra::StringPrintf("Ach%u", nId).c_str());

        auto nSize = rc_trigger_size(sTrigger.c_str());
        void* trigger_buffer = rc_buffer_alloc(&game->buffer, nSize);
        ach->trigger = rc_parse_trigger(trigger_buffer, sTrigger.c_str(), nullptr, 0);
        ActivateAchievement(ach);

        return ach;
    }

    void ActivateAchievement(rc_client_achievement_info_t* pAchievement)
    {
        pAchievement->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
        pAchievement->trigger->state = RC_TRIGGER_STATE_WAITING;
    }

    void DeactivateAchievement(rc_client_achievement_info_t* pAchievement)
    {
        pAchievement->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_INACTIVE;
        pAchievement->trigger->state = RC_TRIGGER_STATE_INACTIVE;
    }

    ra::data::models::AchievementModel* WrapAchievement(rc_client_achievement_info_t* pAchievement)
    {
        auto nCategory = ra::data::models::AssetCategory::Core;
        if (pAchievement->public_.category == RC_CLIENT_ACHIEVEMENT_CATEGORY_UNOFFICIAL)
        {
            nCategory = ra::data::models::AssetCategory::Unofficial;
        }
        else
        {
            const auto* pSubset = GetClient()->game->subsets;
            for (; pSubset; pSubset = pSubset->next)
            {
                for (uint32_t i = 0; i < pSubset->public_.num_achievements; ++i)
                {
                    if (&pSubset->achievements[i] == pAchievement)
                    {
                        if (pSubset->public_.id == ra::data::context::GameAssets::LocalSubsetId)
                            nCategory = ra::data::models::AssetCategory::Local;
                        break;
                    }
                }
            }
        }

        auto vmAchievement = std::make_unique<ra::data::models::AchievementModel>();
        vmAchievement->Attach(*pAchievement, nCategory, "");

        mockGameContext.Assets().Append(std::move(vmAchievement));
        return mockGameContext.Assets().FindAchievement(pAchievement->public_.id);
    }

    rc_client_leaderboard_info_t* MockLeaderboard(uint32_t nId, const std::string& sDefinition)
    {
        rc_client_game_info_t* game = GetClient()->game;
        auto* lboard = AddLeaderboard(game, GetCoreSubset(game), nId, ra::StringPrintf("Leaderboard%u", nId).c_str());

        auto nSize = rc_lboard_size(sDefinition.c_str());
        void* lboard_buffer = rc_buffer_alloc(&game->buffer, nSize);
        lboard->lboard = rc_parse_lboard(lboard_buffer, sDefinition.c_str(), nullptr, 0);
        ActivateLeaderboard(lboard);

        return lboard;
    }

    rc_client_leaderboard_info_t* MockLocalLeaderboard(uint32_t nId, const std::string& sDefinition)
    {
        rc_client_game_info_t* game = GetClient()->game;
        auto* lboard = AddLeaderboard(game, GetLocalSubset(game), nId, ra::StringPrintf("Leaderboard%u", nId).c_str());

        auto nSize = rc_lboard_size(sDefinition.c_str());
        void* lboard_buffer = rc_buffer_alloc(&game->buffer, nSize);
        lboard->lboard = rc_parse_lboard(lboard_buffer, sDefinition.c_str(), nullptr, 0);
        ActivateLeaderboard(lboard);

        return lboard;
    }

    void ActivateLeaderboard(rc_client_leaderboard_info_t* pLeaderboard)
    {
        pLeaderboard->public_.state = RC_CLIENT_LEADERBOARD_STATE_ACTIVE;
        pLeaderboard->lboard->state = RC_LBOARD_STATE_ACTIVE;
    }

    void DeactivateLeaderboard(rc_client_leaderboard_info_t* pLeaderboard)
    {
        pLeaderboard->public_.state = RC_CLIENT_LEADERBOARD_STATE_INACTIVE;
        pLeaderboard->lboard->state = RC_LBOARD_STATE_INACTIVE;
    }

    ra::data::models::LeaderboardModel* WrapLeaderboard(rc_client_leaderboard_info_t* pLeaderboard)
    {
        auto nCategory = ra::data::models::AssetCategory::Core;

        const auto* pSubset = GetClient()->game->subsets;
        for (; pSubset; pSubset = pSubset->next)
        {
            for (uint32_t i = 0; i < pSubset->public_.num_leaderboards; ++i)
            {
                if (&pSubset->leaderboards[i] == pLeaderboard)
                {
                    if (pSubset->public_.id == ra::data::context::GameAssets::LocalSubsetId)
                        nCategory = ra::data::models::AssetCategory::Local;
                    break;
                }
            }
        }

        auto vmLeaderboard = std::make_unique<ra::data::models::LeaderboardModel>();
        vmLeaderboard->Attach(*pLeaderboard, nCategory, "");
        pLeaderboard->lboard->state = RC_LBOARD_STATE_ACTIVE; // syncing State sets this back to waiting

        mockGameContext.Assets().Append(std::move(vmLeaderboard));
        return mockGameContext.Assets().FindLeaderboard(pLeaderboard->public_.id);
    }

    void SyncToRuntime()
    {
        // toggling hardcore will reset the runtime with appropriately active achievements
        bool bHardcoreEnabled = rc_client_get_hardcore_enabled(GetClient());
        rc_client_set_hardcore_enabled(GetClient(), !bHardcoreEnabled);
        rc_client_set_hardcore_enabled(GetClient(), bHardcoreEnabled);

        // one of the toggles will be to hardcore mode, which expects a reset - clear the flag
        GetClient()->game->waiting_for_reset = false;

        // MockAchievement generates triggers with their own memrefs - promote to common memref pool
        auto& runtime = GetClient()->game->runtime;
        Assert::IsNull(runtime.memrefs);
        auto** next = &runtime.memrefs;
        for (unsigned i = 0; i < runtime.trigger_count; ++i)
        {
            *next = runtime.triggers[i].trigger->memrefs;
            while (*next)
                next = &(*next)->next;

            runtime.triggers[i].trigger->memrefs = nullptr;
        }
    }

    void SaveProgressToString(std::string& sBuffer)
    {
        const size_t nSize = SaveProgressToBuffer(reinterpret_cast<uint8_t*>(sBuffer.data()), gsl::narrow_cast<int>(sBuffer.size()));
        if (nSize > sBuffer.size())
        {
            sBuffer.resize(nSize);
            SaveProgressToBuffer(reinterpret_cast<uint8_t*>(sBuffer.data()), gsl::narrow_cast<int>(nSize));
        }
    }

    bool LoadProgressFromString(const std::string& sBuffer)
    {
        return LoadProgressFromBuffer(reinterpret_cast<const uint8_t*>(sBuffer.data()));
    }

    std::string GetRichPresenceOverride()
    {
        char buffer[256];
        if (!GetClient()->callbacks.rich_presence_override(GetClient(), buffer, sizeof(buffer)))
            buffer[0] = '\0';

        return std::string(buffer);
    }

    void MockInspectingMemory(bool bInspectingMemory)
    {
        mockWindowManager.MemoryBookmarks.SetIsVisible(bInspectingMemory);
    }

private:
    static void CaptureEventHandler(const rc_client_event_t* pEvent, rc_client_t* pClient)
    {
        auto* harness = static_cast<AchievementRuntimeHarness*>(rc_client_get_userdata(pClient));
        Expects(harness != nullptr);
        harness->m_vEvents.push_back(*pEvent);
    }

    std::vector<rc_client_event_t> m_vEvents;

    static rc_client_subset_info_t* GetSubset(rc_client_game_info_t* game, uint32_t subset_id, const char* name)
    {
        rc_client_subset_info_t *subset = game->subsets, **next = &game->subsets;
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
        return GetSubset(game, game->public_.id, game->public_.title);
    }

    static rc_client_subset_info_t* GetLocalSubset(rc_client_game_info_t* game)
    {
        return GetSubset(game, ra::data::context::GameAssets::LocalSubsetId, "Local");
    }

    static rc_client_achievement_info_t* AddAchievement(rc_client_game_info_t* game, rc_client_subset_info_t* subset,
                                                        uint32_t nId, const char* sTitle)
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

        return achievement;
    }

    static rc_client_leaderboard_info_t* AddLeaderboard(rc_client_game_info_t* game, rc_client_subset_info_t* subset,
                                                        uint32_t nId, const char* sTitle)
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

        leaderboard->public_.state = RC_CLIENT_LEADERBOARD_STATE_ACTIVE;

        return leaderboard;
    }

    ra::services::ServiceLocator::ServiceOverride<ra::services::AchievementRuntime> m_Override;
    rc_client_event_handler_t m_fRealEventHandler;
};

TEST_CLASS(AchievementRuntime_Tests)
{
public:
    TEST_METHOD(TestSyncAssetsAchievement)
    {
        AchievementRuntimeHarness runtime;
        runtime.MockGame();

        auto vmAchievement = std::make_unique<ra::data::models::AchievementModel>();
        vmAchievement->CreateServerCheckpoint();
        vmAchievement->SetCategory(ra::data::models::AssetCategory::Local);
        vmAchievement->SetName(L"Achievement Name");
        vmAchievement->SetDescription(L"Do something cool");
        vmAchievement->SetPoints(25);
        vmAchievement->SetTrigger("0xH0000=1");
        vmAchievement->SetID(ra::data::context::GameAssets::FirstLocalId);
        vmAchievement->CreateLocalCheckpoint();
        vmAchievement->SetState(ra::data::models::AssetState::Active);
        runtime.mockGameContext.Assets().Append(std::move(vmAchievement));

        // SyncAssets should generate an empty core subset and a local subset containing the achievement
        runtime.SyncAssets();

        auto* pSubset = runtime.GetClient()->game->subsets;
        Expects(pSubset != nullptr);
        Assert::AreEqual("Game Title", pSubset->public_.title);
        Assert::AreEqual(1U, pSubset->public_.id);
        Assert::AreEqual("012345", pSubset->public_.badge_name);
        Assert::AreEqual(0U, pSubset->public_.num_achievements);
        Assert::IsTrue(pSubset->active);

        pSubset = pSubset->next;
        Expects(pSubset != nullptr);
        Assert::AreEqual("Local", pSubset->public_.title);
        Assert::AreEqual(ra::data::context::GameAssets::LocalSubsetId, pSubset->public_.id);
        Assert::AreEqual("012345", pSubset->public_.badge_name);
        Assert::AreEqual(1U, pSubset->public_.num_achievements);
        Assert::IsTrue(pSubset->active);
        Assert::IsNull(pSubset->next);

        const auto* pAchievement = pSubset->achievements;
        Assert::AreEqual("Achievement Name", pAchievement->public_.title);
        Assert::AreEqual("Do something cool", pAchievement->public_.description);
        Assert::AreEqual(25U, pAchievement->public_.points);
        Assert::AreEqual(ra::data::context::GameAssets::FirstLocalId, pAchievement->public_.id);
        Assert::IsNotNull(pAchievement->trigger);
        const auto* pOldTrigger = pAchievement->trigger;

        // directly modifying the achievement trigger should rebuild the underlying trigger
        runtime.mockGameContext.Assets().FindAchievement(ra::data::context::GameAssets::FirstLocalId)
            ->SetTrigger("0xH0000=2");
        Assert::IsNotNull(pAchievement->trigger);
        const auto* pNewTrigger = pAchievement->trigger;
        Assert::AreNotEqual(static_cast<const void*>(pOldTrigger), static_cast<const void*>(pNewTrigger));

        // refreshing a local asset will reconstruct the model. since the hash is the same, the sync
        // should reuse the trigger.
        runtime.mockGameContext.Assets().Clear();

        auto vmAchievement2 = std::make_unique<ra::data::models::AchievementModel>();
        vmAchievement2->CreateServerCheckpoint();
        vmAchievement2->SetCategory(ra::data::models::AssetCategory::Local);
        vmAchievement2->SetName(L"Achievement Name");
        vmAchievement2->SetDescription(L"Do something cool");
        vmAchievement2->SetPoints(25);
        vmAchievement2->SetTrigger("0xH0000=2");
        vmAchievement2->SetID(ra::data::context::GameAssets::FirstLocalId);
        vmAchievement2->CreateLocalCheckpoint();
        vmAchievement2->SetState(ra::data::models::AssetState::Active);
        runtime.mockGameContext.Assets().Append(std::move(vmAchievement2));

        runtime.SyncAssets();

        pAchievement = runtime.GetClient()->game->subsets->next->achievements;
        Expects(pAchievement != nullptr);
        const auto* pNewerTrigger = pAchievement->trigger;
        Assert::AreEqual(static_cast<const void*>(pNewTrigger), static_cast<const void*>(pNewerTrigger));

        // refreshing a local asset will reconstruct the model. if the hash changes, a new trigger
        // will be constructed.
        runtime.mockGameContext.Assets().Clear();

        auto vmAchievement3 = std::make_unique<ra::data::models::AchievementModel>();
        vmAchievement3->CreateServerCheckpoint();
        vmAchievement3->SetCategory(ra::data::models::AssetCategory::Local);
        vmAchievement3->SetName(L"Achievement Name");
        vmAchievement3->SetDescription(L"Do something cool");
        vmAchievement3->SetPoints(25);
        vmAchievement3->SetTrigger("0xH0000=1");
        vmAchievement3->SetID(ra::data::context::GameAssets::FirstLocalId);
        vmAchievement3->CreateLocalCheckpoint();
        vmAchievement3->SetState(ra::data::models::AssetState::Active);
        runtime.mockGameContext.Assets().Append(std::move(vmAchievement3));

        runtime.SyncAssets();

        pAchievement = runtime.GetClient()->game->subsets->next->achievements;
        Expects(pAchievement != nullptr);
        const auto* pNewestTrigger = pAchievement->trigger;
        Assert::AreNotEqual(static_cast<const void*>(pNewerTrigger), static_cast<const void*>(pNewestTrigger));
    }

    TEST_METHOD(TestDoFrameTriggerAchievement)
    {
        std::array<unsigned char, 1> memory{ 0x00 };

        AchievementRuntimeHarness runtime;
        runtime.mockEmulatorContext.MockMemory(memory);
        runtime.MockAchievement(6U, "0xH0000=1");

        // expect no events for untriggered achievement
        runtime.DoFrame();
        Assert::AreEqual({ 0U }, runtime.GetEventCount());

        // now trigger it
        memory.at(0) = 1;
        runtime.DoFrame();
        Assert::AreEqual({ 1U }, runtime.GetEventCount());
        runtime.AssertEvent(RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED, 6U);
        runtime.ResetEvents();

        // triggered achievement should not continue to trigger
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
    }

    TEST_METHOD(TestDoFrameTriggerAchievementWhilePaused)
    {
        std::array<unsigned char, 1> memory{ 0x00 };

        AchievementRuntimeHarness runtime;
        runtime.mockEmulatorContext.MockMemory(memory);
        runtime.MockAchievement(6U, "0xH0000=1");

        runtime.SetPaused(true);
        Assert::IsTrue(runtime.IsPaused());

        // runtime is paused and achievement will be ignored as long as its true
        for (int i = 0; i < 10; i++)
        {
            runtime.DoFrame();
            Assert::AreEqual({0U}, runtime.GetEventCount());
        }

        runtime.SetPaused(false);
        Assert::IsFalse(runtime.IsPaused());

        // expect no events for untriggered achievement
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());

        // pause runtime and trigger achievement
        runtime.SetPaused(true);
        Assert::IsTrue(runtime.IsPaused());
        memory.at(0) = 1;

        // runtime is paused and achievement will be ignored as long as its paused
        for (int i = 0; i < 10; i++)
        {
            runtime.DoFrame();
            Assert::AreEqual({0U}, runtime.GetEventCount());
        }

        // unpausing the runtime does not explicitly process a frame
        runtime.SetPaused(false);
        Assert::IsFalse(runtime.IsPaused());
        Assert::AreEqual({0U}, runtime.GetEventCount());

        // now achievement should trigger
        runtime.DoFrame();
        Assert::AreEqual({1U}, runtime.GetEventCount());
        runtime.AssertEvent(RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED, 6U);
        runtime.ResetEvents();
    }

    TEST_METHOD(TestDoFrameReactivateAchievementWhilePaused)
    {
        std::array<unsigned char, 1> memory{0x00};

        AchievementRuntimeHarness runtime;
        runtime.mockEmulatorContext.MockMemory(memory);
        auto* pAchievement = runtime.MockAchievement(6U, "0xH0000=1");

        // expect no events for untriggered achievement
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());

        // trigger achievement
        memory.at(0) = 1;
        runtime.DoFrame();
        Assert::AreEqual({1U}, runtime.GetEventCount());
        runtime.AssertEvent(RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED, 6U);
        runtime.ResetEvents();

        // pause runtime and reactivate the achievement - expect no events
        runtime.SetPaused(true);
        runtime.ActivateAchievement(pAchievement);
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());

        // unpause runtime - expect no events - achievement is waiting
        runtime.SetPaused(false);
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
        Assert::AreEqual((int)RC_TRIGGER_STATE_WAITING, (int)pAchievement->trigger->state);

        // adjust memory so achievement will transition to active
        memory.at(0) = 0;
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
        Assert::AreEqual((int)RC_TRIGGER_STATE_ACTIVE, (int)pAchievement->trigger->state);

        // pause runtime and update memory so achievement would trigger - expect no events
        runtime.SetPaused(true);
        memory.at(0) = 1;
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());

        // unpause runtime - expect event
        runtime.SetPaused(false);
        runtime.DoFrame();
        Assert::AreEqual({1U}, runtime.GetEventCount());
        runtime.AssertEvent(RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED, 6U);
    }

    TEST_METHOD(TestDoFrameMeasuredAchievement)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};

        AchievementRuntimeHarness runtime;
        runtime.mockEmulatorContext.MockMemory(memory);
        runtime.MockAchievement(4U, "M:0xH0002=100");

        // value did not change, expect no event from initialization
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());

        // change value, expect notification
        memory.at(2) = 75;
        runtime.DoFrame();
        Assert::AreEqual({1U}, runtime.GetEventCount());
        runtime.AssertEvent(RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_SHOW, 4U);
        runtime.ResetEvents();

        // watch for notification, no change, no notification
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());

        // change value, expect notification
        memory.at(2) = 78;
        runtime.DoFrame();
        Assert::AreEqual({1U}, runtime.GetEventCount());
        runtime.AssertEvent(RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_UPDATE, 4U);
        runtime.ResetEvents();

        // trigger achievement, expect trigger notification, but not progress notification
        memory.at(2) = 100;
        runtime.DoFrame();
        Assert::AreEqual({1U}, runtime.GetEventCount());
        runtime.AssertEvent(RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED, 4U);
    }
    /*
    TEST_METHOD(TestUpdateAchievementId)
    {
        std::array<unsigned char, 1> memory{ 0x00 };

        AchievementRuntimeHarness runtime;
        runtime.mockEmulatorContext.MockMemory(memory);
        auto* sTrigger = "0xH0000=1";

        // achievement not active, should not raise events
        std::vector<AchievementRuntime::Change> vChanges;
        runtime.Process(vChanges);
        Assert::AreEqual({ 0U }, vChanges.size());

        // make sure the trigger is registered correctly
        runtime.ActivateAchievement(110000006U, sTrigger);
        auto* pTrigger = runtime.GetAchievementTrigger(110000006U);
        Assert::IsNotNull(pTrigger);

        // update the ID (this simulates uploading a local achievement)
        runtime.UpdateAchievementId(110000006U, 99001U);
        auto const* pTrigger2 = runtime.GetAchievementTrigger(99001U);
        Assert::IsTrue(pTrigger == pTrigger2);

        pTrigger = runtime.GetAchievementTrigger(110000006U);
        Assert::IsNull(pTrigger);

        // first Process call will switch the state from Waiting to Active
        runtime.Process(vChanges);
        Assert::AreEqual({ 1U }, vChanges.size());
        AssertChange(vChanges, AchievementRuntime::ChangeType::AchievementActivated, 99001U);
        vChanges.clear();

        // now that it's active, we can trigger it
        memory.at(0) = 1;
        runtime.Process(vChanges);
        Assert::AreEqual({ 1U }, vChanges.size());
        AssertChange(vChanges, AchievementRuntime::ChangeType::AchievementTriggered, 99001U);
    }
    */

    static rc_condition_t* GetCondition(AchievementRuntimeHarness& harness, ra::AchievementID nId, int nGroup, int nCond) noexcept
    {
        rc_trigger_t* pTrigger = harness.GetAchievementTrigger(nId);
        rc_condset_t* pCondset = pTrigger->requirement;
        if (nGroup > 1)
        {
            pCondset = pTrigger->alternative;
            while (pCondset && --nGroup > 0)
                pCondset = pCondset->next;
        }

        if (!pCondset)
            return nullptr;

        rc_condition_t* pCondition = pCondset->conditions;
        while (pCondition && nCond-- > 0)
            pCondition = pCondition->next;

        return pCondition;
    }

    static void SetConditionHitCount(AchievementRuntimeHarness& harness, ra::AchievementID nId, int nGroup, int nCond, int nHits)
    {
        rc_condition_t* pCond = GetCondition(harness, nId, nGroup, nCond);
        Assert::IsNotNull(pCond);
        pCond->current_hits = ra::to_unsigned(nHits);
    }

    static void AssertConditionHitCount(AchievementRuntimeHarness& harness, ra::AchievementID nId, int nGroup, int nCond, int nHits)
    {
        const rc_condition_t* pCond = GetCondition(harness, nId, nGroup, nCond);
        Assert::IsNotNull(pCond);
        Assert::AreEqual(pCond->current_hits, ra::to_unsigned(nHits));
    }

    TEST_METHOD(TestPersistProgressFile)
    {
        AchievementRuntimeHarness runtime;
        auto* pAch3 = runtime.MockAchievement(3U, "1=1.10.");
        auto* pAch5 = runtime.MockAchievement(5U, "1=1.2.");
        runtime.SyncToRuntime();

        SetConditionHitCount(runtime, 3U, 0, 0, 2);
        SetConditionHitCount(runtime, 5U, 0, 0, 2);

        runtime.SaveProgressToFile("test.sav");

        // inactive achievements aren't updated from the persisted data
        runtime.DeactivateAchievement(pAch3);
        runtime.DeactivateAchievement(pAch5);
        SetConditionHitCount(runtime, 3U, 0, 0, 1);
        SetConditionHitCount(runtime, 5U, 0, 0, 1);
        runtime.LoadProgressFromFile("test.sav");
        AssertConditionHitCount(runtime, 3U, 0, 0, 1);
        AssertConditionHitCount(runtime, 5U, 0, 0, 1);

        // activate one achievement, save, activate other, restore - only first should be restored,
        // second should be reset because it wasn't captured.
        runtime.ActivateAchievement(pAch3);
        SetConditionHitCount(runtime, 3U, 0, 0, 1);
        SetConditionHitCount(runtime, 5U, 0, 0, 1);
        runtime.SaveProgressToFile("test.sav");
        runtime.ActivateAchievement(pAch5);       
        SetConditionHitCount(runtime, 3U, 0, 0, 2);
        SetConditionHitCount(runtime, 5U, 0, 0, 2);
        runtime.LoadProgressFromFile("test.sav");
        AssertConditionHitCount(runtime, 3U, 0, 0, 1);
        AssertConditionHitCount(runtime, 5U, 0, 0, 0);

        // both active, save and restore should reset both
        SetConditionHitCount(runtime, 3U, 0, 0, 1);
        SetConditionHitCount(runtime, 5U, 0, 0, 1);
        runtime.SaveProgressToFile("test.sav");

        SetConditionHitCount(runtime, 3U, 0, 0, 2);
        SetConditionHitCount(runtime, 5U, 0, 0, 2);
        runtime.LoadProgressFromFile("test.sav");
        AssertConditionHitCount(runtime, 3U, 0, 0, 1);
        AssertConditionHitCount(runtime, 5U, 0, 0, 1);

        // second no longer active, file contains data that should be ignored
        SetConditionHitCount(runtime, 3U, 0, 0, 6);
        SetConditionHitCount(runtime, 5U, 0, 0, 6);
        runtime.DeactivateAchievement(pAch5);
        runtime.LoadProgressFromFile("test.sav");
        AssertConditionHitCount(runtime, 3U, 0, 0, 1);
        AssertConditionHitCount(runtime, 5U, 0, 0, 6);

        // if the definition changes (different md5), the hits should be reset instead of remembered
        runtime.GetClient()->game->runtime.triggers[0].md5[0]++;
        SetConditionHitCount(runtime, 3U, 0, 0, 6);
        runtime.ActivateAchievement(pAch3);
        runtime.LoadProgressFromFile("test.sav");
        AssertConditionHitCount(runtime, 3U, 0, 0, 0);
    }

    TEST_METHOD(TestPersistProgressBuffer)
    {
        AchievementRuntimeHarness runtime;
        auto* pAch3 = runtime.MockAchievement(3U, "1=1.10.");
        auto* pAch5 = runtime.MockAchievement(5U, "1=1.2.");
        runtime.SyncToRuntime();

        SetConditionHitCount(runtime, 3U, 0, 0, 2);
        SetConditionHitCount(runtime, 5U, 0, 0, 2);

        std::string sBuffer;
        runtime.SaveProgressToString(sBuffer);

        // inactive achievements aren't updated from the persisted data
        runtime.DeactivateAchievement(pAch3);
        runtime.DeactivateAchievement(pAch5);
        SetConditionHitCount(runtime, 3U, 0, 0, 1);
        SetConditionHitCount(runtime, 5U, 0, 0, 1);
        runtime.LoadProgressFromString(sBuffer);
        AssertConditionHitCount(runtime, 3U, 0, 0, 1);
        AssertConditionHitCount(runtime, 5U, 0, 0, 1);

        // activate one achievement, save, activate other, restore - only first should be restored,
        // second should be reset because it wasn't captured.
        runtime.ActivateAchievement(pAch3);
        SetConditionHitCount(runtime, 3U, 0, 0, 1);
        SetConditionHitCount(runtime, 5U, 0, 0, 1);
        runtime.SaveProgressToString(sBuffer);
        runtime.ActivateAchievement(pAch5);
        SetConditionHitCount(runtime, 3U, 0, 0, 2);
        SetConditionHitCount(runtime, 5U, 0, 0, 2);
        runtime.LoadProgressFromString(sBuffer);
        AssertConditionHitCount(runtime, 3U, 0, 0, 1);
        AssertConditionHitCount(runtime, 5U, 0, 0, 0);

        // both active, save and restore should reset both
        SetConditionHitCount(runtime, 3U, 0, 0, 1);
        SetConditionHitCount(runtime, 5U, 0, 0, 1);
        runtime.SaveProgressToString(sBuffer);

        SetConditionHitCount(runtime, 3U, 0, 0, 2);
        SetConditionHitCount(runtime, 5U, 0, 0, 2);
        runtime.LoadProgressFromString(sBuffer);
        AssertConditionHitCount(runtime, 3U, 0, 0, 1);
        AssertConditionHitCount(runtime, 5U, 0, 0, 1);

        // second no longer active, file contains data that should be ignored
        SetConditionHitCount(runtime, 3U, 0, 0, 6);
        SetConditionHitCount(runtime, 5U, 0, 0, 6);
        runtime.DeactivateAchievement(pAch5);
        runtime.LoadProgressFromString(sBuffer);
        AssertConditionHitCount(runtime, 3U, 0, 0, 1);
        AssertConditionHitCount(runtime, 5U, 0, 0, 6);

        // if the definition changes (different md5), the hits should be reset instead of remembered
        runtime.GetClient()->game->runtime.triggers[0].md5[0]++;
        SetConditionHitCount(runtime, 3U, 0, 0, 6);
        runtime.ActivateAchievement(pAch3);
        runtime.LoadProgressFromString(sBuffer);
        AssertConditionHitCount(runtime, 3U, 0, 0, 0);
    }

    TEST_METHOD(TestPersistProgressMemory)
    {
        AchievementRuntimeHarness runtime;
        runtime.MockAchievement(9U, "0xH1234=1_0xX1234>d0xX1234");
        runtime.SyncToRuntime();
        auto pMemRefs = runtime.GetClient()->game->runtime.memrefs;

        SetConditionHitCount(runtime, 9U, 0, 0, 2);
        SetConditionHitCount(runtime, 9U, 0, 1, 0);
        auto pMemRef = pMemRefs; // 0xH1234
        pMemRef->value.value = 0x02;
        pMemRef->value.changed = 0;
        pMemRef->value.prior = 0x03;
        pMemRef = pMemRef->next; // 0xX1234
        pMemRef->value.value = 0x020000;
        pMemRef->value.changed = 1;
        pMemRef->value.prior = 0x040000;

        runtime.SaveProgressToFile("test.sav");

        // modify data so we can see if the persisted data is restored
        pMemRef = pMemRefs;
        pMemRef->value.value = 0;
        pMemRef->value.changed = 0;
        pMemRef->value.prior = 0;
        pMemRef = pMemRef->next;
        pMemRef->value.value = 0;
        pMemRef->value.changed = 0;
        pMemRef->value.prior = 0;
        SetConditionHitCount(runtime, 9U, 0, 0, 1);
        SetConditionHitCount(runtime, 9U, 0, 1, 1);

        // restore persisted data
        runtime.LoadProgressFromFile("test.sav");

        AssertConditionHitCount(runtime, 9U, 0, 0, 2);
        AssertConditionHitCount(runtime, 9U, 0, 1, 0);

        pMemRef = pMemRefs;
        Assert::AreEqual(0x02U, pMemRef->value.value);
        Assert::AreEqual(0, (int)pMemRef->value.changed);
        Assert::AreEqual(0x03U, pMemRef->value.prior);
        pMemRef = pMemRef->next;
        Assert::AreEqual(0x020000U, pMemRef->value.value);
        Assert::AreEqual(1, (int)pMemRef->value.changed);
        Assert::AreEqual(0x040000U, pMemRef->value.prior);
    }

    TEST_METHOD(TestLoadProgressV1)
    {
        AchievementRuntimeHarness runtime;
        runtime.WrapAchievement(runtime.MockAchievement(3U, "1=1.10."))->SetTrigger("1=1.10.");
        runtime.WrapAchievement(runtime.MockAchievement(5U, "1=1.2."))->SetTrigger("1=1.2.");
        runtime.SyncToRuntime();

        SetConditionHitCount(runtime, 3U, 0, 0, 2);
        SetConditionHitCount(runtime, 5U, 0, 0, 2);

        runtime.mockFileSystem.MockFile(L"test.sav.rap", "3:1:1:0:0:0:0:6e2301982f40d1a3f311cdb063f57e2f:4f52856e145d7cb05822e8a9675b086b:5:1:1:0:0:0:0:0e9aec1797ad6ba861a4b1e0c7f6d2ab:dd9e5fc6020e728b8c9231d5a5c904d5:");

        runtime.LoadProgressFromFile("test.sav");

        AssertConditionHitCount(runtime, 3U, 0, 0, 1);
        AssertConditionHitCount(runtime, 5U, 0, 0, 1);
    }

    TEST_METHOD(TestDoFrameActivateLeaderboard)
    {
        std::array<unsigned char, 5> memory{ 0x00, 0x12, 0x34, 0xAB, 0x56 };

        AchievementRuntimeHarness runtime;
        runtime.mockEmulatorContext.MockMemory(memory);
        auto* pLeaderboard = runtime.MockLeaderboard(6U, "STA:0xH00=1::CAN:0xH00=2::SUB:0xH00=3::VAL:0xH02");

        // expect no events for inactive leaderboard
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());

        // leaderboard cancel condition should not be captured until it's been started
        memory.at(0) = 2;
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());

        // leaderboard start condition should notify the start with initial value
        memory.at(0) = 1;
        runtime.DoFrame();
        Assert::AreEqual({2U}, runtime.GetEventCount());
        runtime.AssertEvent(RC_CLIENT_EVENT_LEADERBOARD_STARTED, 6U);
        runtime.AssertEvent(RC_CLIENT_EVENT_LEADERBOARD_TRACKER_SHOW, 1U);
        runtime.ResetEvents();

        // still active leaderboard should not notify unless the value changes
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());

        memory.at(2) = 33;
        runtime.DoFrame();
        Assert::AreEqual({1U}, runtime.GetEventCount());
        runtime.AssertEvent(RC_CLIENT_EVENT_LEADERBOARD_TRACKER_UPDATE, 1U);
        runtime.ResetEvents();

        // leaderboard cancel condition should notify
        memory.at(0) = 2;
        runtime.DoFrame();
        Assert::AreEqual({2U}, runtime.GetEventCount());
        runtime.AssertEvent(RC_CLIENT_EVENT_LEADERBOARD_FAILED, 6U);
        runtime.AssertEvent(RC_CLIENT_EVENT_LEADERBOARD_TRACKER_HIDE, 1U);
        runtime.ResetEvents();

        // leaderboard submit condition should not trigger if leaderboard not started
        memory.at(0) = 3;
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());

        // restart the leaderboard
        memory.at(0) = 1;
        runtime.DoFrame();
        Assert::AreEqual({2U}, runtime.GetEventCount());
        runtime.AssertEvent(RC_CLIENT_EVENT_LEADERBOARD_STARTED, 6U);
        runtime.AssertEvent(RC_CLIENT_EVENT_LEADERBOARD_TRACKER_SHOW, 1U);
        runtime.ResetEvents();

        // leaderboard submit should trigger if leaderboard started
        memory.at(0) = 3;
        runtime.DoFrame();
        Assert::AreEqual({2U}, runtime.GetEventCount());
        runtime.AssertEvent(RC_CLIENT_EVENT_LEADERBOARD_SUBMITTED, 6U);
        runtime.AssertEvent(RC_CLIENT_EVENT_LEADERBOARD_TRACKER_HIDE, 1U);
        runtime.ResetEvents();

        // leaderboard cancel condition should not trigger after submission
        memory.at(0) = 2;
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());

        // restart the leaderboard
        memory.at(0) = 1;
        runtime.DoFrame();
        Assert::AreEqual({2U}, runtime.GetEventCount());
        runtime.AssertEvent(RC_CLIENT_EVENT_LEADERBOARD_STARTED, 6U);
        runtime.AssertEvent(RC_CLIENT_EVENT_LEADERBOARD_TRACKER_SHOW, 1U);
        runtime.ResetEvents();

        // deactivated leaderboard should not trigger
        runtime.DeactivateLeaderboard(pLeaderboard);
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());

        memory.at(0) = 2;
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
    }

    TEST_METHOD(TestActivateRichPresence)
    {
        std::array<unsigned char, 5> memory{ 0x00, 0x12, 0x34, 0xAB, 0x56 };

        AchievementRuntimeHarness runtime;
        runtime.mockEmulatorContext.MockMemory(memory);
        Assert::IsFalse(runtime.HasRichPresence());

        const char* pRichPresence = "Format:Num\nFormatType:Value\n\nDisplay:\n@Num(0xH01) @Num(d0xH01)\n";
        runtime.ActivateRichPresence(pRichPresence);
        Assert::IsTrue(runtime.HasRichPresence());

        // string is evaluated with current memrefs (which will be 0)
        Assert::AreEqual(std::wstring(L"0 0"), runtime.GetRichPresenceDisplayString());

        // do_frame should immediately process the rich presence
        runtime.DoFrame();
        Assert::AreEqual(std::wstring(L"18 0"), runtime.GetRichPresenceDisplayString());

        // first update - updates value and delta
        runtime.DoFrame();
        Assert::AreEqual(std::wstring(L"18 18"), runtime.GetRichPresenceDisplayString());

        // second update - updates delta
        runtime.DoFrame();
        Assert::AreEqual(std::wstring(L"18 18"), runtime.GetRichPresenceDisplayString());

        // third update - updates value after change
        memory.at(1) = 11;
        runtime.DoFrame();
        Assert::AreEqual(std::wstring(L"11 18"), runtime.GetRichPresenceDisplayString());

        // fourth update - updates delta and value after change
        memory.at(1) = 13;
        runtime.DoFrame();
        Assert::AreEqual(std::wstring(L"13 11"), runtime.GetRichPresenceDisplayString());
    }

    TEST_METHOD(TestActivateRichPresenceChange)
    {
        std::array<unsigned char, 5> memory{ 0x00, 0x12, 0x34, 0xAB, 0x56 };

        AchievementRuntimeHarness runtime;
        runtime.mockEmulatorContext.MockMemory(memory);
        Assert::IsFalse(runtime.HasRichPresence());

        runtime.ActivateRichPresence("Display:\nHello, World\n");
        Assert::AreEqual(std::wstring(L"Hello, World"), runtime.GetRichPresenceDisplayString());
        Assert::IsTrue(runtime.HasRichPresence());

        runtime.ActivateRichPresence("Display:\nNew String\n");
        Assert::AreEqual(std::wstring(L"New String"), runtime.GetRichPresenceDisplayString());
        Assert::IsTrue(runtime.HasRichPresence());

        runtime.ActivateRichPresence("");
        Assert::AreEqual(std::wstring(L"No Rich Presence defined."), runtime.GetRichPresenceDisplayString());
        Assert::IsFalse(runtime.HasRichPresence());
    }

    TEST_METHOD(TestActivateRichPresenceWithError)
    {
        std::array<unsigned char, 5> memory{ 0x00, 0x12, 0x34, 0xAB, 0x56 };

        AchievementRuntimeHarness runtime;
        runtime.mockEmulatorContext.MockMemory(memory);

        const char* pRichPresence = "Format:Num\nFormatType:Value\n\nDisplay:\n@Num(0H01) @Num(d0xH01)\n";
        runtime.ActivateRichPresence(pRichPresence);
        Assert::IsTrue(runtime.HasRichPresence());

        Assert::AreEqual(std::wstring(L"Parse error -6 (line 5): Invalid operator"), runtime.GetRichPresenceDisplayString());
    }

    TEST_METHOD(TestMonitorAchievementPauseOnTrigger)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};

        AchievementRuntimeHarness runtime;
        runtime.mockEmulatorContext.MockMemory(memory);
        auto* pAchievement = runtime.MockAchievement(6U, "0xH0000=1");
        auto* vmAchievement = runtime.WrapAchievement(pAchievement);

        // no trigger, no event
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumTriggeredTriggers());

        // trigger, expect no notification because not watching for one
        memory.at(0) = 1;
        runtime.DoFrame();
        Assert::AreEqual({1U}, runtime.GetEventCount()); // trigger event
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumTriggeredTriggers());
        runtime.ResetEvents();

        // reactivate, no event
        memory.at(0) = 0;
        vmAchievement->SetState(ra::data::models::AssetState::Active);
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumTriggeredTriggers());

        // trigger, watch for notification
        vmAchievement->SetPauseOnTrigger(true);
        memory.at(0) = 1;
        runtime.DoFrame();
        Assert::AreEqual({1U}, runtime.GetEventCount());
        Assert::AreEqual({1U}, runtime.mockFrameEventQueue.NumTriggeredTriggers());
        runtime.mockFrameEventQueue.Reset();
        runtime.ResetEvents();

        // already triggered, shouldn't get repeated notification
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumTriggeredTriggers());
    }

    TEST_METHOD(TestMonitorAchievementPauseOnReset)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};

        AchievementRuntimeHarness runtime;
        runtime.mockEmulatorContext.MockMemory(memory);
        auto* pAchievement = runtime.MockAchievement(4U, "1=1.3._R:0xH0000=1");
        auto* vmAchievement = runtime.WrapAchievement(pAchievement);

        // HitCount should increase, but no trigger
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumResetTriggers());
        Assert::AreEqual(true, pAchievement->trigger->has_hits != 0);

        // trigger reset, expect no notification because not watching for one
        memory.at(0) = 1;
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumResetTriggers());

        // no notification if already reset
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumResetTriggers());

        // disable reset, hitcount should increase, no notification
        memory.at(0) = 0;
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumResetTriggers());

        // trigger reset, watch for notification
        vmAchievement->SetPauseOnReset(true);
        memory.at(0) = 1;
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
        Assert::AreEqual({1U}, runtime.mockFrameEventQueue.NumResetTriggers());
        runtime.mockFrameEventQueue.Reset();

        // already reset, shouldn't get repeated notification
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumResetTriggers());
    }

    TEST_METHOD(TestMonitorLeaderboardStartPauseOnReset)
    {
        std::array<unsigned char, 2> memory{ 0x00, 0x00 };

        AchievementRuntimeHarness runtime;
        runtime.mockEmulatorContext.MockMemory(memory);
        auto* pLeaderboard = runtime.MockLeaderboard(6U,
            "STA:0xH0000=0_0xH0000=9_R:0xH0001=1::SUB:0xH0000=1::CAN:0xH0000=2::VAL:0xH0000");
        auto* vmLeaderboard = runtime.WrapLeaderboard(pLeaderboard);

        // impossible start condition - but it should accumulate hits so we can detect a reset.
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumResetTriggers());
        Assert::IsTrue(pLeaderboard->lboard->start.has_hits);

        // reset should clear hits, but we're not watching for event yet
        memory.at(1) = 1;
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumResetTriggers());
        Assert::IsFalse(pLeaderboard->lboard->start.has_hits);

        // start watching for event and allow hits to accumulate again
        vmLeaderboard->SetPauseOnReset(ra::data::models::LeaderboardModel::LeaderboardParts::Start);
        memory.at(1) = 0;
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumResetTriggers());
        Assert::IsTrue(pLeaderboard->lboard->start.has_hits);

        // reset should clear hits and raise event
        memory.at(1) = 1;
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
        Assert::AreEqual({1U}, runtime.mockFrameEventQueue.NumResetTriggers());
        Assert::IsFalse(pLeaderboard->lboard->start.has_hits);
        runtime.mockFrameEventQueue.Reset();

        // persistent reset should not raise event again
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumResetTriggers());
        Assert::IsFalse(pLeaderboard->lboard->start.has_hits);
    }

    TEST_METHOD(TestMonitorLeaderboardSubmitPauseOnReset)
    {
        std::array<unsigned char, 2> memory{0x00, 0x00};

        AchievementRuntimeHarness runtime;
        runtime.mockEmulatorContext.MockMemory(memory);
        auto* pLeaderboard = runtime.MockLeaderboard(6U,
            "STA:0xH0000=1::SUB:0xH0000=0_0xH0000=9_R:0xH0001=1::CAN:0xH0000=2::VAL:0xH0000");
        auto* vmLeaderboard = runtime.WrapLeaderboard(pLeaderboard);

        // accumulate hits on submit
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumResetTriggers());
        Assert::IsTrue(pLeaderboard->lboard->submit.has_hits);

        // reset should clear hits, but we're not watching for event yet
        memory.at(1) = 1;
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumResetTriggers());
        Assert::IsFalse(pLeaderboard->lboard->submit.has_hits);

        // start watching for event and allow hits to accumulate again
        vmLeaderboard->SetPauseOnReset(ra::data::models::LeaderboardModel::LeaderboardParts::Submit);
        memory.at(1) = 0;
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumResetTriggers());
        Assert::IsTrue(pLeaderboard->lboard->submit.has_hits);

        // reset should clear hits and raise event
        memory.at(1) = 1;
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
        Assert::AreEqual({1U}, runtime.mockFrameEventQueue.NumResetTriggers());
        Assert::IsFalse(pLeaderboard->lboard->submit.has_hits);
        runtime.mockFrameEventQueue.Reset();

        // persistent reset should not raise event again
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumResetTriggers());
        Assert::IsFalse(pLeaderboard->lboard->submit.has_hits);
    }
    
    TEST_METHOD(TestMonitorLeaderboardCancelPauseOnReset)
    {
        std::array<unsigned char, 2> memory{0x00, 0x00};

        AchievementRuntimeHarness runtime;
        runtime.mockEmulatorContext.MockMemory(memory);
        auto* pLeaderboard = runtime.MockLeaderboard(6U,
            "STA:0xH0000=1::SUB:0xH0000=2::CAN:0xH0000=0_0xH0000=9_R:0xH0001=1::VAL:0xH0000");
        auto* vmLeaderboard = runtime.WrapLeaderboard(pLeaderboard);

        // accumulate hits on cancel
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumResetTriggers());
        Assert::IsTrue(pLeaderboard->lboard->cancel.has_hits);

        // reset should clear hits, but we're not watching for event yet
        memory.at(1) = 1;
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumResetTriggers());
        Assert::IsFalse(pLeaderboard->lboard->cancel.has_hits);

        // start watching for event and allow hits to accumulate again
        vmLeaderboard->SetPauseOnReset(ra::data::models::LeaderboardModel::LeaderboardParts::Cancel);
        memory.at(1) = 0;
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumResetTriggers());
        Assert::IsTrue(pLeaderboard->lboard->cancel.has_hits);

        // reset should clear hits and raise event
        memory.at(1) = 1;
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
        Assert::AreEqual({1U}, runtime.mockFrameEventQueue.NumResetTriggers());
        Assert::IsFalse(pLeaderboard->lboard->cancel.has_hits);
        runtime.mockFrameEventQueue.Reset();

        // persistent reset should not raise event again
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumResetTriggers());
        Assert::IsFalse(pLeaderboard->lboard->cancel.has_hits);
    }

    TEST_METHOD(TestMonitorLeaderboardValuePauseOnReset)
    {
        std::array<unsigned char, 3> memory{0x00, 0x00, 0x00};

        AchievementRuntimeHarness runtime;
        runtime.mockEmulatorContext.MockMemory(memory);
        auto* pLeaderboard = runtime.MockLeaderboard(6U,
            "STA:0xH0000=0::SUB:0xH0000=2::CAN:0xH0000=3::VAL:R:0xH0001=1_M:0xH0002=0");
        auto* vmLeaderboard = runtime.WrapLeaderboard(pLeaderboard);

        // start the leaderboard so we can start counting hits on the value
        runtime.DoFrame();
        Assert::AreEqual({2U}, runtime.GetEventCount()); // started + tracker
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumResetTriggers());
        Assert::AreEqual(1U, pLeaderboard->lboard->value.value.value);
        runtime.ResetEvents();

        // reset should clear hits, but we're not watching for event yet
        memory.at(1) = 1;
        runtime.DoFrame();
        Assert::AreEqual({1U}, runtime.GetEventCount()); // tracker updated
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumResetTriggers());
        Assert::AreEqual(0U, pLeaderboard->lboard->value.value.value);
        runtime.ResetEvents();

        // start watching for event and allow hits to accumulate again
        vmLeaderboard->SetPauseOnReset(ra::data::models::LeaderboardModel::LeaderboardParts::Value);
        memory.at(1) = 0;
        runtime.DoFrame();
        Assert::AreEqual({1U}, runtime.GetEventCount()); // tracker updated
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumResetTriggers());
        Assert::AreEqual(1U, pLeaderboard->lboard->value.value.value);
        runtime.ResetEvents();

        // reset should clear hits and raise event
        memory.at(1) = 1;
        runtime.DoFrame();
        Assert::AreEqual({1U}, runtime.GetEventCount()); // tracker updated
        Assert::AreEqual({1U}, runtime.mockFrameEventQueue.NumResetTriggers());
        Assert::AreEqual(0U, pLeaderboard->lboard->value.value.value);
        runtime.mockFrameEventQueue.Reset();
        runtime.ResetEvents();

        // persistent reset should not raise event again
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumResetTriggers());
        Assert::AreEqual(0U, pLeaderboard->lboard->value.value.value);
    }

    TEST_METHOD(TestMonitorLeaderboardStartPauseOnTrigger)
    {
        std::array<unsigned char, 2> memory{0x00, 0x00};

        AchievementRuntimeHarness runtime;
        runtime.mockEmulatorContext.MockMemory(memory);
        auto* pLeaderboard = runtime.MockLeaderboard(6U,
            "STA:0xH0000=1::SUB:0xH0000=2::CAN:0xH0000=3::VAL:0xH0001");
        auto* vmLeaderboard = runtime.WrapLeaderboard(pLeaderboard);

        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());

        // start the leaderboard, but we're not watching for event yet
        memory.at(0) = 1;
        runtime.DoFrame();
        Assert::AreEqual({2U}, runtime.GetEventCount()); // started + tracker show
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumResetTriggers());
        runtime.ResetEvents();

        // cancel the leaderboard
        memory.at(0) = 3;
        runtime.DoFrame();
        Assert::AreEqual({2U}, runtime.GetEventCount()); // canceled + tracker hide
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumResetTriggers());
        runtime.ResetEvents();

        // let leaderboard transition from waiting to active
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumResetTriggers());

        // start watching for event and restart the leaderboard
        vmLeaderboard->SetPauseOnTrigger(ra::data::models::LeaderboardModel::LeaderboardParts::Start);
        memory.at(0) = 1;
        runtime.DoFrame();
        Assert::AreEqual({2U}, runtime.GetEventCount()); // started + tracker show
        Assert::AreEqual({1U}, runtime.mockFrameEventQueue.NumTriggeredTriggers());
        runtime.mockFrameEventQueue.Reset();
        runtime.ResetEvents();
    }

    TEST_METHOD(TestMonitorLeaderboardSubmitPauseOnTrigger)
    {
        std::array<unsigned char, 2> memory{0x00, 0x00};

        AchievementRuntimeHarness runtime;
        runtime.mockEmulatorContext.MockMemory(memory);
        auto* pLeaderboard = runtime.MockLeaderboard(6U,
            "STA:0xH0000=1::SUB:0xH0000=2::CAN:0xH0000=3::VAL:0xH0001");
        auto* vmLeaderboard = runtime.WrapLeaderboard(pLeaderboard);

        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());

        // start the leaderboard
        memory.at(0) = 1;
        runtime.DoFrame();
        Assert::AreEqual({2U}, runtime.GetEventCount()); // started + tracker show
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumResetTriggers());
        runtime.ResetEvents();

        // submit the leaderboard, but we're not watching for the event yet
        memory.at(0) = 2;
        runtime.DoFrame();
        Assert::AreEqual({2U}, runtime.GetEventCount()); // submitted + tracker hide
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumResetTriggers());
        runtime.ResetEvents();

        // let leaderboard transition from waiting to active
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumResetTriggers());

        // start watching for event and restart the leaderboard
        vmLeaderboard->SetPauseOnTrigger(ra::data::models::LeaderboardModel::LeaderboardParts::Submit);
        memory.at(0) = 1;
        runtime.DoFrame();
        Assert::AreEqual({2U}, runtime.GetEventCount()); // started + tracker show
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumTriggeredTriggers());
        runtime.ResetEvents();

        // submit the leaderboard, expect the event
        memory.at(0) = 2;
        runtime.DoFrame();
        Assert::AreEqual({2U}, runtime.GetEventCount()); // submitted + tracker hide
        Assert::AreEqual({1U}, runtime.mockFrameEventQueue.NumTriggeredTriggers());
    }

    TEST_METHOD(TestMonitorLeaderboardCancelPauseOnTrigger)
    {
        std::array<unsigned char, 2> memory{0x00, 0x00};

        AchievementRuntimeHarness runtime;
        runtime.mockEmulatorContext.MockMemory(memory);
        auto* pLeaderboard = runtime.MockLeaderboard(6U, "STA:0xH0000=1::SUB:0xH0000=2::CAN:0xH0000=3::VAL:0xH0001");
        auto* vmLeaderboard = runtime.WrapLeaderboard(pLeaderboard);

        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());

        // start the leaderboard
        memory.at(0) = 1;
        runtime.DoFrame();
        Assert::AreEqual({2U}, runtime.GetEventCount()); // started + tracker show
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumResetTriggers());
        runtime.ResetEvents();

        // cancel the leaderboard, but we're not watching for the event yet
        memory.at(0) = 3;
        runtime.DoFrame();
        Assert::AreEqual({2U}, runtime.GetEventCount()); // canceled + tracker hide
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumResetTriggers());
        runtime.ResetEvents();

        // let leaderboard transition from waiting to active
        runtime.DoFrame();
        Assert::AreEqual({0U}, runtime.GetEventCount());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumResetTriggers());

        // start watching for event and restart the leaderboard
        vmLeaderboard->SetPauseOnTrigger(ra::data::models::LeaderboardModel::LeaderboardParts::Cancel);
        memory.at(0) = 1;
        runtime.DoFrame();
        Assert::AreEqual({2U}, runtime.GetEventCount()); // started + tracker show
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumTriggeredTriggers());
        runtime.ResetEvents();

        // cancel the leaderboard, expect the event
        memory.at(0) = 3;
        runtime.DoFrame();
        Assert::AreEqual({2U}, runtime.GetEventCount()); // submitted + tracker hide
        Assert::AreEqual({1U}, runtime.mockFrameEventQueue.NumTriggeredTriggers());
    }

    TEST_METHOD(TestDoFrameInvalidAddressDisablesAchievement)
    {
        std::array<unsigned char, 2> memory{0x00, 0x00};

        AchievementRuntimeHarness runtime;
        runtime.mockEmulatorContext.MockMemory(memory);
        auto* pAch6 = runtime.MockAchievement(6U, "0xH0004=1");
        runtime.SyncToRuntime();
        auto* vmAch6 = runtime.WrapAchievement(pAch6);

        Assert::AreEqual(static_cast<uint8_t>(RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE), pAch6->public_.state);
        Assert::AreEqual(ra::data::models::AssetState::Waiting, vmAch6->GetState());

        runtime.DoFrame();

        Assert::AreEqual(static_cast<uint8_t>(RC_CLIENT_ACHIEVEMENT_STATE_DISABLED), pAch6->public_.state);

        // model state isn't synced until DoFrame is called.
        vmAch6->DoFrame();
        Assert::AreEqual(ra::data::models::AssetState::Disabled, vmAch6->GetState());
    }

    TEST_METHOD(TestHandleAchievementTriggeredEvent)
    {
        AchievementRuntimeHarness runtime;
        auto* pAch6 = runtime.MockAchievement(6U, "0xH0000=1");
        memcpy(pAch6->public_.badge_name, "012345", 7);
        auto* vmAch6 = runtime.WrapAchievement(pAch6);
        runtime.mockGameContext.SetRichPresenceDisplayString(L"Titles");
        runtime.mockGameContext.Assets().FindRichPresence()->Activate();
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered,
                                                   ra::ui::viewmodels::PopupLocation::BottomLeft);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED;
        event.achievement = &pAch6->public_;
        runtime.RaiseEvent(event);

        Assert::AreEqual(ra::data::models::AssetState::Triggered, vmAch6->GetState());
        Assert::AreEqual(std::wstring(L"Titles"), vmAch6->GetUnlockRichPresence());

        auto* pPopup = runtime.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::AreEqual(ra::ui::viewmodels::Popup::AchievementTriggered, pPopup->GetPopupType());
        Assert::AreEqual(std::wstring(L"Achievement Unlocked"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"Ach6 (5)"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Description 6"), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::Badge, pPopup->GetImage().Type());
        Assert::AreEqual(std::string("012345"), pPopup->GetImage().Name());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumTriggeredTriggers());
        Assert::IsTrue(runtime.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\unlock.wav"));
    }

    TEST_METHOD(TestHandleAchievementTriggeredEventNoRichPresence)
    {
        AchievementRuntimeHarness runtime;
        auto* pAch6 = runtime.MockAchievement(6U, "0xH0000=1");
        memcpy(pAch6->public_.badge_name, "012345", 7);
        auto* vmAch6 = runtime.WrapAchievement(pAch6);
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered,
                                                   ra::ui::viewmodels::PopupLocation::BottomLeft);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED;
        event.achievement = &pAch6->public_;
        runtime.RaiseEvent(event);

        Assert::AreEqual(ra::data::models::AssetState::Triggered, vmAch6->GetState());
        Assert::AreEqual(std::wstring(L""), vmAch6->GetUnlockRichPresence());

        auto* pPopup = runtime.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::AreEqual(ra::ui::viewmodels::Popup::AchievementTriggered, pPopup->GetPopupType());
        Assert::AreEqual(std::wstring(L"Achievement Unlocked"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"Ach6 (5)"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Description 6"), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::Badge, pPopup->GetImage().Type());
        Assert::AreEqual(std::string("012345"), pPopup->GetImage().Name());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumTriggeredTriggers());
        Assert::IsTrue(runtime.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\unlock.wav"));
    }

    TEST_METHOD(TestHandleAchievementTriggeredEventLocal)
    {
        AchievementRuntimeHarness runtime;
        auto* pAch6 = runtime.MockLocalAchievement(6U, "0xH0000=1");
        memcpy(pAch6->public_.badge_name, "012345", 7);
        auto* vmAch6 = runtime.WrapAchievement(pAch6);
        runtime.mockGameContext.SetRichPresenceDisplayString(L"Titles");
        runtime.mockGameContext.Assets().FindRichPresence()->Activate();
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered,
                                                   ra::ui::viewmodels::PopupLocation::BottomLeft);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED;
        event.achievement = &pAch6->public_;
        runtime.RaiseEvent(event);

        Assert::AreEqual(ra::data::models::AssetState::Triggered, vmAch6->GetState());
        Assert::AreEqual(std::wstring(L"Titles"), vmAch6->GetUnlockRichPresence());

        auto* pPopup = runtime.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::AreEqual(ra::ui::viewmodels::Popup::AchievementTriggered, pPopup->GetPopupType());
        Assert::AreEqual(std::wstring(L"Local Achievement Unlocked"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"Ach6 (5)"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Description 6"), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::Badge, pPopup->GetImage().Type());
        Assert::AreEqual(std::string("012345"), pPopup->GetImage().Name());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumTriggeredTriggers());
        Assert::IsTrue(runtime.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\unlock.wav"));
    }

    TEST_METHOD(TestHandleAchievementTriggeredEventUnofficial)
    {
        AchievementRuntimeHarness runtime;
        auto* pAch6 = runtime.MockAchievement(6U, "0xH0000=1");
        pAch6->public_.category = RC_CLIENT_ACHIEVEMENT_CATEGORY_UNOFFICIAL;
        memcpy(pAch6->public_.badge_name, "012345", 7);
        auto* vmAch6 = runtime.WrapAchievement(pAch6);
        runtime.mockGameContext.SetRichPresenceDisplayString(L"Titles");
        runtime.mockGameContext.Assets().FindRichPresence()->Activate();
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered,
                                                   ra::ui::viewmodels::PopupLocation::BottomLeft);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED;
        event.achievement = &pAch6->public_;
        runtime.RaiseEvent(event);

        Assert::AreEqual(ra::data::models::AssetState::Triggered, vmAch6->GetState());
        Assert::AreEqual(std::wstring(L"Titles"), vmAch6->GetUnlockRichPresence());

        auto* pPopup = runtime.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::AreEqual(ra::ui::viewmodels::Popup::AchievementTriggered, pPopup->GetPopupType());
        Assert::AreEqual(std::wstring(L"Unofficial Achievement Unlocked"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"Ach6 (5)"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Description 6"), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::Badge, pPopup->GetImage().Type());
        Assert::AreEqual(std::string("012345"), pPopup->GetImage().Name());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumTriggeredTriggers());
        Assert::IsTrue(runtime.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\unlock.wav"));
    }

    TEST_METHOD(TestHandleAchievementTriggeredEventCompatibilityMode)
    {
        AchievementRuntimeHarness runtime;
        auto* pAch6 = runtime.MockAchievement(6U, "0xH0000=1");
        memcpy(pAch6->public_.badge_name, "012345", 7);
        auto* vmAch6 = runtime.WrapAchievement(pAch6);
        runtime.mockGameContext.SetMode(ra::data::context::GameContext::Mode::CompatibilityTest);
        runtime.mockGameContext.SetRichPresenceDisplayString(L"Titles");
        runtime.mockGameContext.Assets().FindRichPresence()->Activate();
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered,
                                                   ra::ui::viewmodels::PopupLocation::BottomLeft);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED;
        event.achievement = &pAch6->public_;
        runtime.RaiseEvent(event);

        Assert::AreEqual(ra::data::models::AssetState::Triggered, vmAch6->GetState());
        Assert::AreEqual(std::wstring(L"Titles"), vmAch6->GetUnlockRichPresence());

        auto* pPopup = runtime.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::AreEqual(ra::ui::viewmodels::Popup::AchievementTriggered, pPopup->GetPopupType());
        Assert::AreEqual(std::wstring(L"Test Achievement Unlocked"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"Ach6 (5)"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Description 6"), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::Badge, pPopup->GetImage().Type());
        Assert::AreEqual(std::string("012345"), pPopup->GetImage().Name());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumTriggeredTriggers());
        Assert::IsTrue(runtime.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\unlock.wav"));
    }

    TEST_METHOD(TestHandleAchievementTriggeredEventModified)
    {
        AchievementRuntimeHarness runtime;
        auto* pAch6 = runtime.MockAchievement(6U, "0xH0000=1");
        memcpy(pAch6->public_.badge_name, "012345", 7);
        auto* vmAch6 = runtime.WrapAchievement(pAch6);
        vmAch6->SetTrigger("0xH0000=2");
        runtime.mockGameContext.SetRichPresenceDisplayString(L"Titles");
        runtime.mockGameContext.Assets().FindRichPresence()->Activate();
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered,
                                                   ra::ui::viewmodels::PopupLocation::BottomLeft);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED;
        event.achievement = &pAch6->public_;
        runtime.RaiseEvent(event);

        Assert::AreEqual(ra::data::models::AssetState::Triggered, vmAch6->GetState());
        Assert::AreEqual(std::wstring(L"Titles"), vmAch6->GetUnlockRichPresence());

        auto* pPopup = runtime.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::AreEqual(ra::ui::viewmodels::Popup::AchievementTriggered, pPopup->GetPopupType());
        Assert::AreEqual(std::wstring(L"Modified Achievement Unlocked LOCALLY"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"Ach6 (5)"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Description 6"), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::Badge, pPopup->GetImage().Type());
        Assert::AreEqual(std::string("012345"), pPopup->GetImage().Name());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumTriggeredTriggers());
        Assert::IsTrue(runtime.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\acherror.wav"));
    }

    TEST_METHOD(TestHandleAchievementTriggeredEventLocalModified)
    {
        AchievementRuntimeHarness runtime;
        auto* pAch6 = runtime.MockLocalAchievement(6U, "0xH0000=1");
        memcpy(pAch6->public_.badge_name, "012345", 7);
        auto* vmAch6 = runtime.WrapAchievement(pAch6);
        vmAch6->SetTrigger("0xH0000=2");
        runtime.mockGameContext.SetRichPresenceDisplayString(L"Titles");
        runtime.mockGameContext.Assets().FindRichPresence()->Activate();
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered,
                                                   ra::ui::viewmodels::PopupLocation::BottomLeft);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED;
        event.achievement = &pAch6->public_;
        runtime.RaiseEvent(event);

        Assert::AreEqual(ra::data::models::AssetState::Triggered, vmAch6->GetState());
        Assert::AreEqual(std::wstring(L"Titles"), vmAch6->GetUnlockRichPresence());

        auto* pPopup = runtime.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::AreEqual(ra::ui::viewmodels::Popup::AchievementTriggered, pPopup->GetPopupType());
        Assert::AreEqual(std::wstring(L"Modified Local Achievement Unlocked"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"Ach6 (5)"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Description 6"), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::Badge, pPopup->GetImage().Type());
        Assert::AreEqual(std::string("012345"), pPopup->GetImage().Name());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumTriggeredTriggers());
        Assert::IsTrue(runtime.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\acherror.wav"));
    }

    TEST_METHOD(TestHandleAchievementTriggeredEventMemoryModified)
    {
        AchievementRuntimeHarness runtime;
        auto* pAch6 = runtime.MockAchievement(6U, "0xH0000=1");
        memcpy(pAch6->public_.badge_name, "012345", 7);
        auto* vmAch6 = runtime.WrapAchievement(pAch6);
        runtime.mockGameContext.SetRichPresenceDisplayString(L"Titles");
        runtime.mockGameContext.Assets().FindRichPresence()->Activate();
        runtime.mockEmulatorContext.SetMemoryModified();
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered,
                                                   ra::ui::viewmodels::PopupLocation::BottomLeft);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED;
        event.achievement = &pAch6->public_;
        runtime.RaiseEvent(event);

        Assert::AreEqual(ra::data::models::AssetState::Triggered, vmAch6->GetState());
        Assert::AreEqual(std::wstring(L"Titles"), vmAch6->GetUnlockRichPresence());

        auto* pPopup = runtime.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::AreEqual(ra::ui::viewmodels::Popup::AchievementTriggered, pPopup->GetPopupType());
        Assert::AreEqual(std::wstring(L"Achievement Unlocked LOCALLY"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"Ach6 (5)"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Error: RAM tampered with"), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::Badge, pPopup->GetImage().Type());
        Assert::AreEqual(std::string("012345"), pPopup->GetImage().Name());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumTriggeredTriggers());
        Assert::IsTrue(runtime.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\acherror.wav"));
    }

    TEST_METHOD(TestHandleAchievementTriggeredEventMemoryInsecureHardcore)
    {
        AchievementRuntimeHarness runtime;
        auto* pAch6 = runtime.MockAchievement(6U, "0xH0000=1");
        memcpy(pAch6->public_.badge_name, "012345", 7);
        auto* vmAch6 = runtime.WrapAchievement(pAch6);
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        runtime.GetClient()->state.hardcore = 1;
        runtime.mockGameContext.SetRichPresenceDisplayString(L"Titles");
        runtime.mockGameContext.Assets().FindRichPresence()->Activate();
        runtime.mockEmulatorContext.MockMemoryInsecure(true);
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered,
                                                   ra::ui::viewmodels::PopupLocation::BottomLeft);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED;
        event.achievement = &pAch6->public_;
        runtime.RaiseEvent(event);

        Assert::AreEqual(ra::data::models::AssetState::Triggered, vmAch6->GetState());
        Assert::AreEqual(std::wstring(L"Titles"), vmAch6->GetUnlockRichPresence());

        auto* pPopup = runtime.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::AreEqual(ra::ui::viewmodels::Popup::AchievementTriggered, pPopup->GetPopupType());
        Assert::AreEqual(std::wstring(L"Achievement Unlocked LOCALLY"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"Ach6 (5)"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Error: RAM insecure"), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::Badge, pPopup->GetImage().Type());
        Assert::AreEqual(std::string("012345"), pPopup->GetImage().Name());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumTriggeredTriggers());
        Assert::IsTrue(runtime.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\acherror.wav"));
    }

    TEST_METHOD(TestHandleAchievementTriggeredEventMemoryInsecureSoftcore)
    {
        AchievementRuntimeHarness runtime;
        auto* pAch6 = runtime.MockAchievement(6U, "0xH0000=1");
        memcpy(pAch6->public_.badge_name, "012345", 7);
        auto* vmAch6 = runtime.WrapAchievement(pAch6);
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        runtime.GetClient()->state.hardcore = 0;
        runtime.mockGameContext.SetRichPresenceDisplayString(L"Titles");
        runtime.mockGameContext.Assets().FindRichPresence()->Activate();
        runtime.mockEmulatorContext.MockMemoryInsecure(true);
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered,
                                                   ra::ui::viewmodels::PopupLocation::BottomLeft);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED;
        event.achievement = &pAch6->public_;
        runtime.RaiseEvent(event);

        Assert::AreEqual(ra::data::models::AssetState::Triggered, vmAch6->GetState());
        Assert::AreEqual(std::wstring(L"Titles"), vmAch6->GetUnlockRichPresence());

        auto* pPopup = runtime.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::AreEqual(ra::ui::viewmodels::Popup::AchievementTriggered, pPopup->GetPopupType());
        Assert::AreEqual(std::wstring(L"Achievement Unlocked"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"Ach6 (5)"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Description 6"), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::Badge, pPopup->GetImage().Type());
        Assert::AreEqual(std::string("012345"), pPopup->GetImage().Name());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumTriggeredTriggers());
        Assert::IsTrue(runtime.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\unlock.wav"));
    }

    TEST_METHOD(TestHandleAchievementTriggeredEventOffline)
    {
        AchievementRuntimeHarness runtime;
        auto* pAch6 = runtime.MockAchievement(6U, "0xH0000=1");
        memcpy(pAch6->public_.badge_name, "012345", 7);
        auto* vmAch6 = runtime.WrapAchievement(pAch6);
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Offline, true);
        runtime.mockGameContext.SetRichPresenceDisplayString(L"Titles");
        runtime.mockGameContext.Assets().FindRichPresence()->Activate();
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered,
                                                   ra::ui::viewmodels::PopupLocation::BottomLeft);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED;
        event.achievement = &pAch6->public_;
        runtime.RaiseEvent(event);

        Assert::AreEqual(ra::data::models::AssetState::Triggered, vmAch6->GetState());
        Assert::AreEqual(std::wstring(L"Titles"), vmAch6->GetUnlockRichPresence());

        auto* pPopup = runtime.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::AreEqual(ra::ui::viewmodels::Popup::AchievementTriggered, pPopup->GetPopupType());
        Assert::AreEqual(std::wstring(L"Offline Achievement Unlocked"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"Ach6 (5)"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Description 6"), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::Badge, pPopup->GetImage().Type());
        Assert::AreEqual(std::string("012345"), pPopup->GetImage().Name());
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumTriggeredTriggers());
        Assert::IsTrue(runtime.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\unlock.wav"));
    }

    TEST_METHOD(TestHandleAchievementTriggeredEventNoPopup)
    {
        AchievementRuntimeHarness runtime;
        auto* pAch6 = runtime.MockAchievement(6U, "0xH0000=1");
        memcpy(pAch6->public_.badge_name, "012345", 7);
        auto* vmAch6 = runtime.WrapAchievement(pAch6);
        runtime.mockGameContext.SetRichPresenceDisplayString(L"Titles");
        runtime.mockGameContext.Assets().FindRichPresence()->Activate();
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered,
                                                   ra::ui::viewmodels::PopupLocation::None);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED;
        event.achievement = &pAch6->public_;
        runtime.RaiseEvent(event);

        Assert::AreEqual(ra::data::models::AssetState::Triggered, vmAch6->GetState());
        Assert::AreEqual(std::wstring(L"Titles"), vmAch6->GetUnlockRichPresence());

        auto* pPopup = runtime.mockOverlayManager.GetMessage(1);
        Assert::IsNull(pPopup);
        Assert::AreEqual({0U}, runtime.mockFrameEventQueue.NumTriggeredTriggers());
        Assert::IsTrue(runtime.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\unlock.wav"));
    }

    TEST_METHOD(TestHandleChallengeIndicatorShowEvent)
    {
        AchievementRuntimeHarness runtime;
        auto* pAch6 = runtime.MockAchievement(6U, "0xH0000=1");
        memcpy(pAch6->public_.badge_name, "012345", 7);
        runtime.WrapAchievement(pAch6);
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::Challenge,
                                                   ra::ui::viewmodels::PopupLocation::BottomRight);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_ACHIEVEMENT_CHALLENGE_INDICATOR_SHOW;
        event.achievement = &pAch6->public_;
        runtime.RaiseEvent(event);

        auto* pPopup = runtime.mockOverlayManager.GetChallengeIndicator(6U);
        Expects(pPopup != nullptr);
        Assert::AreEqual(ra::ui::viewmodels::Popup::Challenge, pPopup->GetPopupType());
        Assert::AreEqual(ra::ui::ImageType::Badge, pPopup->GetImage().Type());
        Assert::AreEqual(std::string("012345"), pPopup->GetImage().Name());
    }

    TEST_METHOD(TestHandleChallengeIndicatorShowEventNoPopup)
    {
        AchievementRuntimeHarness runtime;
        auto* pAch6 = runtime.MockAchievement(6U, "0xH0000=1");
        memcpy(pAch6->public_.badge_name, "012345", 7);
        runtime.WrapAchievement(pAch6);
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::Challenge,
                                                   ra::ui::viewmodels::PopupLocation::None);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_ACHIEVEMENT_CHALLENGE_INDICATOR_SHOW;
        event.achievement = &pAch6->public_;
        runtime.RaiseEvent(event);

        auto* pPopup = runtime.mockOverlayManager.GetChallengeIndicator(6U);
        Assert::IsNull(pPopup);
    }

    TEST_METHOD(TestHandleChallengeIndicatorHideEvent)
    {
        AchievementRuntimeHarness runtime;
        auto* pAch6 = runtime.MockAchievement(6U, "0xH0000=1");
        runtime.WrapAchievement(pAch6);
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::Challenge,
                                                   ra::ui::viewmodels::PopupLocation::BottomRight);
        runtime.mockOverlayManager.AddChallengeIndicator(6U, ra::ui::ImageType::Badge, "55223");
        auto* pPopup = runtime.mockOverlayManager.GetChallengeIndicator(6U);
        Assert::IsTrue(pPopup != nullptr && !pPopup->IsDestroyPending());

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_ACHIEVEMENT_CHALLENGE_INDICATOR_HIDE;
        event.achievement = &pAch6->public_;
        runtime.RaiseEvent(event);

        pPopup = runtime.mockOverlayManager.GetChallengeIndicator(6U);
        Assert::IsTrue(pPopup == nullptr || pPopup->IsDestroyPending());
    }

    TEST_METHOD(TestHandleChallengeIndicatorHideEventNoPopup)
    {
        AchievementRuntimeHarness runtime;
        auto* pAch6 = runtime.MockAchievement(6U, "0xH0000=1");
        runtime.WrapAchievement(pAch6);
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::Challenge,
                                                   ra::ui::viewmodels::PopupLocation::BottomRight);
        auto* pPopup = runtime.mockOverlayManager.GetChallengeIndicator(6U);
        Assert::IsNull(pPopup);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_ACHIEVEMENT_CHALLENGE_INDICATOR_HIDE;
        event.achievement = &pAch6->public_;
        runtime.RaiseEvent(event);

        pPopup = runtime.mockOverlayManager.GetChallengeIndicator(6U);
        Assert::IsNull(pPopup);
    }

    TEST_METHOD(TestHandleProgressIndicatorShowEvent)
    {
        AchievementRuntimeHarness runtime;
        auto* pAch6 = runtime.MockAchievement(6U, "0xH0000=1");
        snprintf(pAch6->public_.measured_progress, sizeof(pAch6->public_.measured_progress), "6/10");
        memcpy(pAch6->public_.badge_name, "012345", 7);
        runtime.WrapAchievement(pAch6);
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::Progress,
                                                   ra::ui::viewmodels::PopupLocation::BottomRight);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_SHOW;
        event.achievement = &pAch6->public_;
        runtime.RaiseEvent(event);

        auto* pPopup = runtime.mockOverlayManager.GetProgressTracker();
        Expects(pPopup != nullptr);
        Assert::AreEqual(ra::ui::viewmodels::Popup::Progress, pPopup->GetPopupType());
        Assert::AreEqual(ra::ui::ImageType::Badge, pPopup->GetImage().Type());
        Assert::AreEqual(std::string("012345_lock"), pPopup->GetImage().Name());
        Assert::AreEqual(std::wstring(L"6/10"), pPopup->GetText());
    }

    TEST_METHOD(TestHandleProgressIndicatorShowEventNoPopup)
    {
        AchievementRuntimeHarness runtime;
        auto* pAch6 = runtime.MockAchievement(6U, "0xH0000=1");
        snprintf(pAch6->public_.measured_progress, sizeof(pAch6->public_.measured_progress), "6/10");
        memcpy(pAch6->public_.badge_name, "012345", 7);
        runtime.WrapAchievement(pAch6);
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::Progress,
                                                   ra::ui::viewmodels::PopupLocation::None);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_SHOW;
        event.achievement = &pAch6->public_;
        runtime.RaiseEvent(event);

        auto* pPopup = runtime.mockOverlayManager.GetProgressTracker();
        Assert::IsNull(pPopup);
    }

    TEST_METHOD(TestHandleProgressIndicatorShowEventLocalBadge)
    {
        AchievementRuntimeHarness runtime;
        auto* pAch6 = runtime.MockAchievement(6U, "0xH0000=1");
        snprintf(pAch6->public_.measured_progress, sizeof(pAch6->public_.measured_progress), "6/10");
        memcpy(pAch6->public_.badge_name, "012345", 7);
        auto* vmAch6 = runtime.WrapAchievement(pAch6);
        vmAch6->SetBadge(L"local\\abcdefg.png");
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::Progress,
                                                   ra::ui::viewmodels::PopupLocation::BottomRight);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_SHOW;
        event.achievement = &pAch6->public_;
        runtime.RaiseEvent(event);

        auto* pPopup = runtime.mockOverlayManager.GetProgressTracker();
        Expects(pPopup != nullptr);
        Assert::AreEqual(ra::ui::viewmodels::Popup::Progress, pPopup->GetPopupType());
        Assert::AreEqual(ra::ui::ImageType::Badge, pPopup->GetImage().Type());
        Assert::AreEqual(std::string("local\\abcdefg.png"), pPopup->GetImage().Name());
        Assert::AreEqual(std::wstring(L"6/10"), pPopup->GetText());
    }

    TEST_METHOD(TestHandleProgressIndicatorUpdateEvent)
    {
        AchievementRuntimeHarness runtime;
        auto* pAch6 = runtime.MockAchievement(6U, "0xH0000=1");
        snprintf(pAch6->public_.measured_progress, sizeof(pAch6->public_.measured_progress), "6/10");
        memcpy(pAch6->public_.badge_name, "012345", 7);
        runtime.WrapAchievement(pAch6);
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::Progress,
                                                   ra::ui::viewmodels::PopupLocation::BottomRight);
        runtime.mockOverlayManager.UpdateProgressTracker(ra::ui::ImageType::Badge, "000001", L"4/10");

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_UPDATE;
        event.achievement = &pAch6->public_;
        runtime.RaiseEvent(event);

        auto* pPopup = runtime.mockOverlayManager.GetProgressTracker();
        Expects(pPopup != nullptr);
        Assert::AreEqual(ra::ui::viewmodels::Popup::Progress, pPopup->GetPopupType());
        Assert::AreEqual(ra::ui::ImageType::Badge, pPopup->GetImage().Type());
        Assert::AreEqual(std::string("012345_lock"), pPopup->GetImage().Name());
        Assert::AreEqual(std::wstring(L"6/10"), pPopup->GetText());
    }

    TEST_METHOD(TestHandleProgressIndicatorHideEvent)
    {
        AchievementRuntimeHarness runtime;
        auto* pAch6 = runtime.MockAchievement(6U, "0xH0000=1");
        snprintf(pAch6->public_.measured_progress, sizeof(pAch6->public_.measured_progress), "6/10");
        memcpy(pAch6->public_.badge_name, "012345", 7);
        runtime.WrapAchievement(pAch6);
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::Progress,
                                                   ra::ui::viewmodels::PopupLocation::BottomRight);
        runtime.mockOverlayManager.UpdateProgressTracker(ra::ui::ImageType::Badge, "000001", L"4/10");
        auto* pPopup = runtime.mockOverlayManager.GetProgressTracker();
        Assert::IsTrue(pPopup != nullptr && !pPopup->IsDestroyPending());

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_HIDE;
        runtime.RaiseEvent(event);

        pPopup = runtime.mockOverlayManager.GetProgressTracker();
        Assert::IsTrue(pPopup == nullptr || pPopup->IsDestroyPending());
    }
    
    TEST_METHOD(TestHandleGameCompletedEventHardcore)
    {
        AchievementRuntimeHarness runtime;
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        runtime.MockAchievement(6U, "0xH0000=1")->public_.points = 5;
        runtime.MockAchievement(7U, "0xH0000=1")->public_.points = 1;
        runtime.MockAchievement(8U, "0xH0000=1")->public_.points = 1;
        runtime.MockAchievement(9U, "0xH0000=1")->public_.points = 3;
        runtime.MockAchievement(10U, "0xH0000=1")->public_.points = 10;
        runtime.MockAchievement(11U, "0xH0000=1")->public_.points = 5;
        runtime.MockAchievement(12U, "0xH0000=1")->public_.points = 10;
        runtime.MockAchievement(13U, "0xH0000=1")->public_.points = 5;
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::Mastery,
                                                   ra::ui::viewmodels::PopupLocation::TopMiddle);
        runtime.mockSessionTracker.MockSession(1U, time(NULL), std::chrono::seconds(20000));

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_GAME_COMPLETED;
        runtime.RaiseEvent(event);

        auto* pPopup = runtime.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::AreEqual(ra::ui::viewmodels::Popup::Mastery, pPopup->GetPopupType());
        Assert::AreEqual(std::wstring(L"Mastered Game Title"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"8 achievements, 40 points"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"UserDisplay | Play time: 5h33m"), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::Icon, pPopup->GetImage().Type());
        Assert::AreEqual(std::string("012345"), pPopup->GetImage().Name());
        Assert::IsTrue(runtime.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\unlock.wav"));
    }

    TEST_METHOD(TestHandleGameCompletedEventSoftcore)
    {
        AchievementRuntimeHarness runtime;
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        runtime.MockAchievement(6U, "0xH0000=1")->public_.points = 5;
        runtime.MockAchievement(7U, "0xH0000=1")->public_.points = 1;
        runtime.MockAchievement(8U, "0xH0000=1")->public_.points = 1;
        runtime.MockAchievement(9U, "0xH0000=1")->public_.points = 3;
        runtime.MockAchievement(10U, "0xH0000=1")->public_.points = 10;
        runtime.MockAchievement(11U, "0xH0000=1")->public_.points = 5;
        runtime.MockAchievement(12U, "0xH0000=1")->public_.points = 10;
        runtime.MockAchievement(13U, "0xH0000=1")->public_.points = 5;
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::Mastery,
                                                   ra::ui::viewmodels::PopupLocation::TopMiddle);
        runtime.mockSessionTracker.MockSession(1U, time(NULL), std::chrono::seconds(20000));

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_GAME_COMPLETED;
        runtime.RaiseEvent(event);

        auto* pPopup = runtime.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::AreEqual(ra::ui::viewmodels::Popup::Mastery, pPopup->GetPopupType());
        Assert::AreEqual(std::wstring(L"Completed Game Title"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"8 achievements, 40 points"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"UserDisplay | Play time: 5h33m"), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::Icon, pPopup->GetImage().Type());
        Assert::AreEqual(std::string("012345"), pPopup->GetImage().Name());
        Assert::IsTrue(runtime.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\unlock.wav"));
    }

    TEST_METHOD(TestHandleLeaderboardStartedEvent)
    {
        AchievementRuntimeHarness runtime;
        auto *pLbd4 = runtime.MockLeaderboard(4U, "STA:0xH0000=1::CAN:0xH0000=1::SUB:0xH0000=2::VAL:0xH0001");
        runtime.WrapLeaderboard(pLbd4);
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardStarted,
                                                   ra::ui::viewmodels::PopupLocation::BottomLeft);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_LEADERBOARD_STARTED;
        event.leaderboard = &pLbd4->public_;
        runtime.RaiseEvent(event);

        auto* pPopup = runtime.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::AreEqual(ra::ui::viewmodels::Popup::LeaderboardStarted, pPopup->GetPopupType());
        Assert::AreEqual(std::wstring(L"Leaderboard attempt started"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"Leaderboard4"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Description 4"), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::None, pPopup->GetImage().Type());
        Assert::AreEqual(std::string(), pPopup->GetImage().Name());
        Assert::IsTrue(runtime.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\lb.wav"));
    }

    TEST_METHOD(TestHandleLeaderboardStartedEventNoPopup)
    {
        AchievementRuntimeHarness runtime;
        auto* pLbd4 = runtime.MockLeaderboard(4U, "STA:0xH0000=1::CAN:0xH0000=1::SUB:0xH0000=2::VAL:0xH0001");
        runtime.WrapLeaderboard(pLbd4);
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardStarted,
                                                   ra::ui::viewmodels::PopupLocation::None);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_LEADERBOARD_STARTED;
        event.leaderboard = &pLbd4->public_;
        runtime.RaiseEvent(event);

        auto* pPopup = runtime.mockOverlayManager.GetMessage(1);
        Assert::IsNull(pPopup);
    }

    TEST_METHOD(TestHandleLeaderboardFailedEvent)
    {
        AchievementRuntimeHarness runtime;
        auto* pLbd4 = runtime.MockLeaderboard(4U, "STA:0xH0000=1::CAN:0xH0000=1::SUB:0xH0000=2::VAL:0xH0001");
        runtime.WrapLeaderboard(pLbd4);
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardCanceled,
                                                   ra::ui::viewmodels::PopupLocation::BottomLeft);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_LEADERBOARD_FAILED;
        event.leaderboard = &pLbd4->public_;
        runtime.RaiseEvent(event);

        auto* pPopup = runtime.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::AreEqual(ra::ui::viewmodels::Popup::LeaderboardCanceled, pPopup->GetPopupType());
        Assert::AreEqual(std::wstring(L"Leaderboard attempt failed"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"Leaderboard4"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Description 4"), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::None, pPopup->GetImage().Type());
        Assert::AreEqual(std::string(), pPopup->GetImage().Name());
        Assert::IsTrue(runtime.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\lbcancel.wav"));
    }

    TEST_METHOD(TestHandleLeaderboardFailedEventNoPopup)
    {
        AchievementRuntimeHarness runtime;
        auto* pLbd4 = runtime.MockLeaderboard(4U, "STA:0xH0000=1::CAN:0xH0000=1::SUB:0xH0000=2::VAL:0xH0001");
        runtime.WrapLeaderboard(pLbd4);
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardCanceled,
                                                   ra::ui::viewmodels::PopupLocation::None);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_LEADERBOARD_FAILED;
        event.leaderboard = &pLbd4->public_;
        runtime.RaiseEvent(event);

        auto* pPopup = runtime.mockOverlayManager.GetMessage(1);
        Assert::IsNull(pPopup);
    }

    TEST_METHOD(TestHandleLeaderboardSubmittedEvent)
    {
        AchievementRuntimeHarness runtime;
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        auto* pLbd4 = runtime.MockLeaderboard(4U, "STA:0xH0000=1::CAN:0xH0000=1::SUB:0xH0000=2::VAL:0xH0001");
        pLbd4->public_.tracker_value = "1:23.45";
        runtime.WrapLeaderboard(pLbd4);
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard,
                                                   ra::ui::viewmodels::PopupLocation::BottomRight);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_LEADERBOARD_SUBMITTED;
        event.leaderboard = &pLbd4->public_;
        runtime.RaiseEvent(event);

        // Submitted event doesn't do anything unless a problem occurs - scoreboard event is used instead
        Assert::IsNull(runtime.mockOverlayManager.GetMessage(1));
        Assert::IsNull(runtime.mockOverlayManager.GetScoreboard(4U));
    }

private:
    void AssertSimpleScoreboard(AchievementRuntimeHarness& runtime, ra::LeaderboardID nId)
    {
        auto* pScoreboard = runtime.mockOverlayManager.GetScoreboard(nId);
        Assert::IsNotNull(pScoreboard);
        Ensures(pScoreboard != nullptr);

        Assert::AreEqual(ra::ui::viewmodels::Popup::LeaderboardScoreboard, pScoreboard->GetPopupType());
        Assert::AreEqual(std::wstring(L"Leaderboard4"), pScoreboard->GetHeaderText());
        Assert::AreEqual({1U}, pScoreboard->Entries().Count());

        const auto* pEntry = pScoreboard->Entries().GetItemAt(0);
        Expects(pEntry != nullptr);
        Assert::AreEqual(0, pEntry->GetRank());
        Assert::AreEqual(std::wstring(L"1:23.45"), pEntry->GetScore());
        Assert::AreEqual(std::wstring(L"User_"), pEntry->GetUserName());
        Assert::IsTrue(pEntry->IsHighlighted());
    }

public:
    TEST_METHOD(TestHandleLeaderboardSubmittedEventLocal)
    {
        AchievementRuntimeHarness runtime;
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        auto* pLbd4 = runtime.MockLocalLeaderboard(4U, "STA:0xH0000=1::CAN:0xH0000=1::SUB:0xH0000=2::VAL:0xH0001");
        pLbd4->public_.tracker_value = "1:23.45";
        runtime.WrapLeaderboard(pLbd4);
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard,
                                                   ra::ui::viewmodels::PopupLocation::BottomRight);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_LEADERBOARD_SUBMITTED;
        event.leaderboard = &pLbd4->public_;
        runtime.RaiseEvent(event);

        auto* pPopup = runtime.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::AreEqual(ra::ui::viewmodels::Popup::Message, pPopup->GetPopupType());
        Assert::AreEqual(std::wstring(L"Local Leaderboard Submitted"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"Leaderboard4"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Local leaderboards are not submitted."), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::None, pPopup->GetImage().Type());
        Assert::AreEqual(std::string(), pPopup->GetImage().Name());
        Assert::IsTrue(runtime.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\info.wav"));

        AssertSimpleScoreboard(runtime, 4U);
    }

    TEST_METHOD(TestHandleLeaderboardSubmittedEventCompatibilityMode)
    {
        AchievementRuntimeHarness runtime;
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        runtime.mockGameContext.SetMode(ra::data::context::GameContext::Mode::CompatibilityTest);
        auto* pLbd4 = runtime.MockLeaderboard(4U, "STA:0xH0000=1::CAN:0xH0000=1::SUB:0xH0000=2::VAL:0xH0001");
        pLbd4->public_.tracker_value = "1:23.45";
        runtime.WrapLeaderboard(pLbd4);
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard,
                                                   ra::ui::viewmodels::PopupLocation::BottomRight);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_LEADERBOARD_SUBMITTED;
        event.leaderboard = &pLbd4->public_;
        runtime.RaiseEvent(event);

        auto* pPopup = runtime.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::AreEqual(ra::ui::viewmodels::Popup::Message, pPopup->GetPopupType());
        Assert::AreEqual(std::wstring(L"Leaderboard Submitted"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"Leaderboard4"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Leaderboards are not submitted in compatibility test mode."), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::None, pPopup->GetImage().Type());
        Assert::AreEqual(std::string(), pPopup->GetImage().Name());
        Assert::IsTrue(runtime.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\info.wav"));

        AssertSimpleScoreboard(runtime, 4U);
    }

    TEST_METHOD(TestHandleLeaderboardSubmittedEventModified)
    {
        AchievementRuntimeHarness runtime;
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        auto* pLbd4 = runtime.MockLeaderboard(4U, "STA:0xH0000=1::CAN:0xH0000=1::SUB:0xH0000=2::VAL:0xH0001");
        pLbd4->public_.tracker_value = "1:23.45";
        auto* vmLbd4 = runtime.WrapLeaderboard(pLbd4);
        vmLbd4->SetDescription(L"Modified Description");
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard,
                                                   ra::ui::viewmodels::PopupLocation::BottomRight);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_LEADERBOARD_SUBMITTED;
        event.leaderboard = &pLbd4->public_;
        runtime.RaiseEvent(event);

        auto* pPopup = runtime.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::AreEqual(ra::ui::viewmodels::Popup::Message, pPopup->GetPopupType());
        Assert::AreEqual(std::wstring(L"Modified Leaderboard Submitted"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"Leaderboard4"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Modified leaderboards are not submitted."), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::None, pPopup->GetImage().Type());
        Assert::AreEqual(std::string(), pPopup->GetImage().Name());
        Assert::IsTrue(runtime.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\info.wav"));

        AssertSimpleScoreboard(runtime, 4U);
    }

    TEST_METHOD(TestHandleLeaderboardSubmittedEventUnpublished)
    {
        AchievementRuntimeHarness runtime;
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        auto* pLbd4 = runtime.MockLeaderboard(4U, "STA:0xH0000=1::CAN:0xH0000=1::SUB:0xH0000=2::VAL:0xH0001");
        pLbd4->public_.tracker_value = "1:23.45";
        auto* vmLbd4 = runtime.WrapLeaderboard(pLbd4);
        vmLbd4->SetDescription(L"Modified Description");
        vmLbd4->UpdateLocalCheckpoint();
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard,
                                                   ra::ui::viewmodels::PopupLocation::BottomRight);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_LEADERBOARD_SUBMITTED;
        event.leaderboard = &pLbd4->public_;
        runtime.RaiseEvent(event);

        auto* pPopup = runtime.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::AreEqual(ra::ui::viewmodels::Popup::Message, pPopup->GetPopupType());
        Assert::AreEqual(std::wstring(L"Modified Leaderboard Submitted"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"Leaderboard4"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Modified leaderboards are not submitted."), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::None, pPopup->GetImage().Type());
        Assert::AreEqual(std::string(), pPopup->GetImage().Name());
        Assert::IsTrue(runtime.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\info.wav"));

        AssertSimpleScoreboard(runtime, 4U);
    }

    TEST_METHOD(TestHandleLeaderboardSubmittedEventModifiedMemory)
    {
        AchievementRuntimeHarness runtime;
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        auto* pLbd4 = runtime.MockLeaderboard(4U, "STA:0xH0000=1::CAN:0xH0000=1::SUB:0xH0000=2::VAL:0xH0001");
        pLbd4->public_.tracker_value = "1:23.45";
        runtime.WrapLeaderboard(pLbd4);
        runtime.mockEmulatorContext.MockMemoryModified(true);
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard,
                                                   ra::ui::viewmodels::PopupLocation::BottomRight);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_LEADERBOARD_SUBMITTED;
        event.leaderboard = &pLbd4->public_;
        runtime.RaiseEvent(event);

        auto* pPopup = runtime.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::AreEqual(ra::ui::viewmodels::Popup::Message, pPopup->GetPopupType());
        Assert::AreEqual(std::wstring(L"Leaderboard Submitted"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"Leaderboard4"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Error: RAM tampered with"), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::None, pPopup->GetImage().Type());
        Assert::AreEqual(std::string(), pPopup->GetImage().Name());
        Assert::IsTrue(runtime.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\info.wav"));

        AssertSimpleScoreboard(runtime, 4U);
    }

    TEST_METHOD(TestHandleLeaderboardSubmittedEventSoftcore)
    {
        AchievementRuntimeHarness runtime;
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        auto* pLbd4 = runtime.MockLeaderboard(4U, "STA:0xH0000=1::CAN:0xH0000=1::SUB:0xH0000=2::VAL:0xH0001");
        pLbd4->public_.tracker_value = "1:23.45";
        runtime.WrapLeaderboard(pLbd4);
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard,
                                                   ra::ui::viewmodels::PopupLocation::BottomRight);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_LEADERBOARD_SUBMITTED;
        event.leaderboard = &pLbd4->public_;
        runtime.RaiseEvent(event);

        auto* pPopup = runtime.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::AreEqual(ra::ui::viewmodels::Popup::Message, pPopup->GetPopupType());
        Assert::AreEqual(std::wstring(L"Leaderboard Submitted"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"Leaderboard4"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Submission requires Hardcore mode"), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::None, pPopup->GetImage().Type());
        Assert::AreEqual(std::string(), pPopup->GetImage().Name());
        Assert::IsTrue(runtime.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\info.wav"));

        AssertSimpleScoreboard(runtime, 4U);
    }
   
    TEST_METHOD(TestHandleLeaderboardSubmittedEventMemoryInsecure)
    {
        AchievementRuntimeHarness runtime;
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        auto* pLbd4 = runtime.MockLeaderboard(4U, "STA:0xH0000=1::CAN:0xH0000=1::SUB:0xH0000=2::VAL:0xH0001");
        pLbd4->public_.tracker_value = "1:23.45";
        runtime.WrapLeaderboard(pLbd4);
        runtime.mockEmulatorContext.MockMemoryInsecure(true);
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard,
                                                   ra::ui::viewmodels::PopupLocation::BottomRight);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_LEADERBOARD_SUBMITTED;
        event.leaderboard = &pLbd4->public_;
        runtime.RaiseEvent(event);

        auto* pPopup = runtime.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::AreEqual(ra::ui::viewmodels::Popup::Message, pPopup->GetPopupType());
        Assert::AreEqual(std::wstring(L"Leaderboard Submitted"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"Leaderboard4"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Error: RAM insecure"), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::None, pPopup->GetImage().Type());
        Assert::AreEqual(std::string(), pPopup->GetImage().Name());
        Assert::IsTrue(runtime.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\info.wav"));

        AssertSimpleScoreboard(runtime, 4U);
    }

    TEST_METHOD(TestHandleLeaderboardSubmittedEventOffline)
    {
        AchievementRuntimeHarness runtime;
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Offline, true);
        auto* pLbd4 = runtime.MockLeaderboard(4U, "STA:0xH0000=1::CAN:0xH0000=1::SUB:0xH0000=2::VAL:0xH0001");
        pLbd4->public_.tracker_value = "1:23.45";
        runtime.WrapLeaderboard(pLbd4);
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard,
                                                   ra::ui::viewmodels::PopupLocation::BottomRight);

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_LEADERBOARD_SUBMITTED;
        event.leaderboard = &pLbd4->public_;
        runtime.RaiseEvent(event);

        auto* pPopup = runtime.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::AreEqual(ra::ui::viewmodels::Popup::Message, pPopup->GetPopupType());
        Assert::AreEqual(std::wstring(L"Leaderboard Submitted"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"Leaderboard4"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Leaderboards are not submitted in offline mode."), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::None, pPopup->GetImage().Type());
        Assert::AreEqual(std::string(), pPopup->GetImage().Name());
        Assert::IsTrue(runtime.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\info.wav"));

        AssertSimpleScoreboard(runtime, 4U);
    }

    TEST_METHOD(TestHandleLeaderboardTrackerShowEvent)
    {
        AchievementRuntimeHarness runtime;
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardTracker,
                                                   ra::ui::viewmodels::PopupLocation::BottomRight);

        rc_client_leaderboard_tracker_t tracker;
        memset(&tracker, 0, sizeof(tracker));
        snprintf(tracker.display, sizeof(tracker.display), "1:23.45");
        tracker.id = 1;

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_LEADERBOARD_TRACKER_SHOW;
        event.leaderboard_tracker = &tracker;
        runtime.RaiseEvent(event);

        auto* pPopup = runtime.mockOverlayManager.GetScoreTracker(1);
        Expects(pPopup != nullptr);
        Assert::AreEqual(ra::ui::viewmodels::Popup::LeaderboardTracker, pPopup->GetPopupType());
        Assert::AreEqual(std::wstring(L"1:23.45"), pPopup->GetDisplayText());
    }

    TEST_METHOD(TestHandleLeaderboardTrackerShowEventNoPopup)
    {
        AchievementRuntimeHarness runtime;
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardTracker,
                                                   ra::ui::viewmodels::PopupLocation::None);

        rc_client_leaderboard_tracker_t tracker;
        memset(&tracker, 0, sizeof(tracker));
        snprintf(tracker.display, sizeof(tracker.display), "1:23.45");
        tracker.id = 1;

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_LEADERBOARD_TRACKER_SHOW;
        event.leaderboard_tracker = &tracker;
        runtime.RaiseEvent(event);

        auto* pPopup = runtime.mockOverlayManager.GetScoreTracker(1);
        Assert::IsNull(pPopup);
    }

    TEST_METHOD(TestHandleLeaderboardTrackerUpdateEvent)
    {
        AchievementRuntimeHarness runtime;
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardTracker,
                                                   ra::ui::viewmodels::PopupLocation::BottomRight);
        runtime.mockOverlayManager.AddScoreTracker(1U).SetDisplayText(L"XXX");

        rc_client_leaderboard_tracker_t tracker;
        memset(&tracker, 0, sizeof(tracker));
        snprintf(tracker.display, sizeof(tracker.display), "1:23.45");
        tracker.id = 1;

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_LEADERBOARD_TRACKER_UPDATE;
        event.leaderboard_tracker = &tracker;
        runtime.RaiseEvent(event);

        auto* pPopup = runtime.mockOverlayManager.GetScoreTracker(1);
        Expects(pPopup != nullptr);
        Assert::AreEqual(ra::ui::viewmodels::Popup::LeaderboardTracker, pPopup->GetPopupType());
        Assert::AreEqual(std::wstring(L"1:23.45"), pPopup->GetDisplayText());
    }

    TEST_METHOD(TestHandleLeaderboardTrackerUpdateEventNoPopup)
    {
        AchievementRuntimeHarness runtime;
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardTracker,
                                                   ra::ui::viewmodels::PopupLocation::None);

        rc_client_leaderboard_tracker_t tracker;
        memset(&tracker, 0, sizeof(tracker));
        snprintf(tracker.display, sizeof(tracker.display), "1:23.45");
        tracker.id = 1;

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_LEADERBOARD_TRACKER_UPDATE;
        event.leaderboard_tracker = &tracker;
        runtime.RaiseEvent(event);

        auto* pPopup = runtime.mockOverlayManager.GetScoreTracker(1);
        Assert::IsNull(pPopup);
    }

    TEST_METHOD(TestHandleLeaderboardTrackerHideEvent)
    {
        AchievementRuntimeHarness runtime;
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardTracker,
                                                   ra::ui::viewmodels::PopupLocation::BottomRight);
        runtime.mockOverlayManager.AddScoreTracker(1U).SetDisplayText(L"XXX");

        rc_client_leaderboard_tracker_t tracker;
        memset(&tracker, 0, sizeof(tracker));
        snprintf(tracker.display, sizeof(tracker.display), "1:23.45");
        tracker.id = 1;

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_LEADERBOARD_TRACKER_HIDE;
        event.leaderboard_tracker = &tracker;
        runtime.RaiseEvent(event);

        auto* pPopup = runtime.mockOverlayManager.GetScoreTracker(1);
        Assert::IsTrue(pPopup == nullptr || pPopup->IsDestroyPending());
    }

    TEST_METHOD(TestHandleLeaderboardScoreboardEvent)
    {
        AchievementRuntimeHarness runtime;
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        auto* pLbd4 = runtime.MockLeaderboard(4U, "STA:0xH0000=1::CAN:0xH0000=1::SUB:0xH0000=2::VAL:0xH0001");
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard,
                                                   ra::ui::viewmodels::PopupLocation::BottomRight);

        rc_client_leaderboard_scoreboard_entry_t entries[3];
        memset(entries, 0, sizeof(entries));
        rc_client_leaderboard_scoreboard_t scoreboard;
        memset(&scoreboard, 0, sizeof(scoreboard));
        scoreboard.leaderboard_id = 4U;
        scoreboard.new_rank = 2;
        snprintf(scoreboard.best_score, sizeof(scoreboard.best_score), "1:20.00");
        snprintf(scoreboard.submitted_score, sizeof(scoreboard.submitted_score), "1:23.45");
        scoreboard.num_entries = 3;
        scoreboard.num_top_entries = 3;
        scoreboard.top_entries = entries;
        entries[0].rank = 1;
        snprintf(entries[0].score, sizeof(entries[0].score), "1:19.55");
        entries[0].username = "Bob";
        entries[1].rank = 2;
        snprintf(entries[1].score, sizeof(entries[0].score), "1:20.00");
        entries[1].username = "User_";
        entries[2].rank = 3;
        snprintf(entries[2].score, sizeof(entries[0].score), "1:20.12");
        entries[2].username = "Tara";

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_LEADERBOARD_SCOREBOARD;
        event.leaderboard = &pLbd4->public_;
        event.leaderboard_scoreboard = &scoreboard;
        runtime.RaiseEvent(event);

        auto* pScoreboard = runtime.mockOverlayManager.GetScoreboard(4U);
        Assert::IsNotNull(pScoreboard);
        Ensures(pScoreboard != nullptr);

        Assert::AreEqual(ra::ui::viewmodels::Popup::LeaderboardScoreboard, pScoreboard->GetPopupType());
        Assert::AreEqual(std::wstring(L"Leaderboard4"), pScoreboard->GetHeaderText());
        Assert::AreEqual({3U}, pScoreboard->Entries().Count());

        const auto* pEntry = pScoreboard->Entries().GetItemAt(0);
        Expects(pEntry != nullptr);
        Assert::AreEqual(1, pEntry->GetRank());
        Assert::AreEqual(std::wstring(L"1:19.55"), pEntry->GetScore());
        Assert::AreEqual(std::wstring(L"Bob"), pEntry->GetUserName());
        Assert::IsFalse(pEntry->IsHighlighted());

        pEntry = pScoreboard->Entries().GetItemAt(1);
        Expects(pEntry != nullptr);
        Assert::AreEqual(2, pEntry->GetRank());
        Assert::AreEqual(std::wstring(L"(1:23.45) 1:20.00"), pEntry->GetScore());
        Assert::AreEqual(std::wstring(L"User_"), pEntry->GetUserName());
        Assert::IsTrue(pEntry->IsHighlighted());

        pEntry = pScoreboard->Entries().GetItemAt(2);
        Expects(pEntry != nullptr);
        Assert::AreEqual(3, pEntry->GetRank());
        Assert::AreEqual(std::wstring(L"1:20.12"), pEntry->GetScore());
        Assert::AreEqual(std::wstring(L"Tara"), pEntry->GetUserName());
        Assert::IsFalse(pEntry->IsHighlighted());
    }

    TEST_METHOD(TestHandleLeaderboardScoreboardEventNoPopup)
    {
        AchievementRuntimeHarness runtime;
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        auto* pLbd4 = runtime.MockLeaderboard(4U, "STA:0xH0000=1::CAN:0xH0000=1::SUB:0xH0000=2::VAL:0xH0001");
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard,
                                                   ra::ui::viewmodels::PopupLocation::None);

        rc_client_leaderboard_scoreboard_entry_t entries[3];
        memset(entries, 0, sizeof(entries));
        rc_client_leaderboard_scoreboard_t scoreboard;
        memset(&scoreboard, 0, sizeof(scoreboard));
        scoreboard.leaderboard_id = 4U;
        scoreboard.new_rank = 2;
        snprintf(scoreboard.best_score, sizeof(scoreboard.best_score), "1:20.00");
        snprintf(scoreboard.submitted_score, sizeof(scoreboard.submitted_score), "1:23.45");
        scoreboard.num_entries = 3;
        scoreboard.num_top_entries = 3;
        scoreboard.top_entries = entries;
        entries[0].rank = 1;
        snprintf(entries[0].score, sizeof(entries[0].score), "1:19.55");
        entries[0].username = "Bob";
        entries[1].rank = 2;
        snprintf(entries[1].score, sizeof(entries[1].score), "1:20.00");
        entries[1].username = "User_";
        entries[2].rank = 3;
        snprintf(entries[2].score, sizeof(entries[2].score), "1:20.12");
        entries[2].username = "Tara";

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_LEADERBOARD_SCOREBOARD;
        event.leaderboard = &pLbd4->public_;
        event.leaderboard_scoreboard = &scoreboard;
        runtime.RaiseEvent(event);

        auto* pScoreboard = runtime.mockOverlayManager.GetScoreboard(4U);
        Assert::IsNull(pScoreboard);
    }

    TEST_METHOD(TestHandleLeaderboardScoreboardEventPlayerNotInTopEntries)
    {
        AchievementRuntimeHarness runtime;
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        auto* pLbd4 = runtime.MockLeaderboard(4U, "STA:0xH0000=1::CAN:0xH0000=1::SUB:0xH0000=2::VAL:0xH0001");
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard,
                                                   ra::ui::viewmodels::PopupLocation::BottomRight);

        rc_client_leaderboard_scoreboard_entry_t entries[7];
        memset(entries, 0, sizeof(entries));
        rc_client_leaderboard_scoreboard_t scoreboard;
        memset(&scoreboard, 0, sizeof(scoreboard));
        scoreboard.leaderboard_id = 4U;
        scoreboard.new_rank = 27;
        snprintf(scoreboard.best_score, sizeof(scoreboard.best_score), "1:20.00");
        snprintf(scoreboard.submitted_score, sizeof(scoreboard.submitted_score), "1:23.45");
        scoreboard.num_entries = 31;
        scoreboard.num_top_entries = 7;
        scoreboard.top_entries = entries;
        entries[0].rank = 1;
        snprintf(entries[0].score, sizeof(entries[0].score), "0:56.55");
        entries[0].username = "Bob";
        entries[1].rank = 2;
        snprintf(entries[1].score, sizeof(entries[1].score), "0:57.00");
        entries[1].username = "Calvin";
        entries[2].rank = 3;
        snprintf(entries[2].score, sizeof(entries[2].score), "0:57.12");
        entries[2].username = "Debbie";
        entries[3].rank = 4;
        snprintf(entries[3].score, sizeof(entries[3].score), "0:58.22");
        entries[3].username = "Erin";
        entries[4].rank = 4;
        snprintf(entries[4].score, sizeof(entries[4].score), "0:58.22");
        entries[4].username = "Frank";
        entries[5].rank = 6;
        snprintf(entries[5].score, sizeof(entries[5].score), "0:59.88");
        entries[5].username = "George";
        entries[6].rank = 7;
        snprintf(entries[6].score, sizeof(entries[6].score), "1:00.41");
        entries[6].username = "Harry";

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_LEADERBOARD_SCOREBOARD;
        event.leaderboard = &pLbd4->public_;
        event.leaderboard_scoreboard = &scoreboard;
        runtime.RaiseEvent(event);

        auto* pScoreboard = runtime.mockOverlayManager.GetScoreboard(4U);
        Assert::IsNotNull(pScoreboard);
        Ensures(pScoreboard != nullptr);

        Assert::AreEqual(ra::ui::viewmodels::Popup::LeaderboardScoreboard, pScoreboard->GetPopupType());
        Assert::AreEqual(std::wstring(L"Leaderboard4"), pScoreboard->GetHeaderText());
        Assert::AreEqual({7U}, pScoreboard->Entries().Count());

        const auto* pEntry = pScoreboard->Entries().GetItemAt(0);
        Expects(pEntry != nullptr);
        Assert::AreEqual(1, pEntry->GetRank());
        Assert::AreEqual(std::wstring(L"0:56.55"), pEntry->GetScore());
        Assert::AreEqual(std::wstring(L"Bob"), pEntry->GetUserName());
        Assert::IsFalse(pEntry->IsHighlighted());

        pEntry = pScoreboard->Entries().GetItemAt(1);
        Expects(pEntry != nullptr);
        Assert::AreEqual(2, pEntry->GetRank());
        Assert::AreEqual(std::wstring(L"0:57.00"), pEntry->GetScore());
        Assert::AreEqual(std::wstring(L"Calvin"), pEntry->GetUserName());
        Assert::IsFalse(pEntry->IsHighlighted());

        pEntry = pScoreboard->Entries().GetItemAt(2);
        Expects(pEntry != nullptr);
        Assert::AreEqual(3, pEntry->GetRank());
        Assert::AreEqual(std::wstring(L"0:57.12"), pEntry->GetScore());
        Assert::AreEqual(std::wstring(L"Debbie"), pEntry->GetUserName());
        Assert::IsFalse(pEntry->IsHighlighted());

        pEntry = pScoreboard->Entries().GetItemAt(3);
        Expects(pEntry != nullptr);
        Assert::AreEqual(4, pEntry->GetRank());
        Assert::AreEqual(std::wstring(L"0:58.22"), pEntry->GetScore());
        Assert::AreEqual(std::wstring(L"Erin"), pEntry->GetUserName());
        Assert::IsFalse(pEntry->IsHighlighted());

        pEntry = pScoreboard->Entries().GetItemAt(4);
        Expects(pEntry != nullptr);
        Assert::AreEqual(4, pEntry->GetRank());
        Assert::AreEqual(std::wstring(L"0:58.22"), pEntry->GetScore());
        Assert::AreEqual(std::wstring(L"Frank"), pEntry->GetUserName());
        Assert::IsFalse(pEntry->IsHighlighted());

        pEntry = pScoreboard->Entries().GetItemAt(5);
        Expects(pEntry != nullptr);
        Assert::AreEqual(6, pEntry->GetRank());
        Assert::AreEqual(std::wstring(L"0:59.88"), pEntry->GetScore());
        Assert::AreEqual(std::wstring(L"George"), pEntry->GetUserName());
        Assert::IsFalse(pEntry->IsHighlighted());

        pEntry = pScoreboard->Entries().GetItemAt(6);
        Expects(pEntry != nullptr);
        Assert::AreEqual(27, pEntry->GetRank());
        Assert::AreEqual(std::wstring(L"(1:23.45) 1:20.00"), pEntry->GetScore());
        Assert::AreEqual(std::wstring(L"User_"), pEntry->GetUserName());
        Assert::IsTrue(pEntry->IsHighlighted());
    }

    // TODO: TestHandleServerError

    TEST_METHOD(TestHandleResetEvent)
    {
        AchievementRuntimeHarness runtime;
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        runtime.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardTracker,
                                                   ra::ui::viewmodels::PopupLocation::BottomRight);

        bool bWasReset = false;
        runtime.mockEmulatorContext.SetResetFunction([&bWasReset]() { bWasReset = true; });

        rc_client_event_t event;
        memset(&event, 0, sizeof(event));
        event.type = RC_CLIENT_EVENT_RESET;
        runtime.RaiseEvent(event);

        Assert::IsTrue(bWasReset);
    }

    TEST_METHOD(TestRichPresenceOverrideNoRichPresence)
    {
        AchievementRuntimeHarness runtime;
        runtime.mockGameContext.SetGameTitle(L"GameTitle");
        Assert::AreEqual(std::string(""), runtime.GetRichPresenceOverride()); // no override
    }

    TEST_METHOD(TestRichPresenceOverrideRichPresence)
    {
        AchievementRuntimeHarness runtime;
        runtime.mockGameContext.SetRichPresenceDisplayString(L"Level 10");
        runtime.mockGameContext.Assets().FindRichPresence()->Activate();
        Assert::AreEqual(std::string(""), runtime.GetRichPresenceOverride()); // no override
    }

    TEST_METHOD(TestRichPresenceOverrideRichPresenceFromFile)
    {
        AchievementRuntimeHarness runtime;
        runtime.mockGameContext.SetGameTitle(L"GameTitle");
        runtime.mockGameContext.SetRichPresenceDisplayString(L"Level 10");
        runtime.mockGameContext.SetRichPresenceFromFile(true);
        runtime.mockGameContext.Assets().FindRichPresence()->Activate();
        Assert::AreEqual(std::string("Playing GameTitle"), runtime.GetRichPresenceOverride());
    }

    TEST_METHOD(TestRichPresenceOverrideRichPresenceCompatibilityMode)
    {
        AchievementRuntimeHarness runtime;
        runtime.mockGameContext.SetMode(ra::data::context::GameContext::Mode::CompatibilityTest);
        runtime.mockGameContext.SetRichPresenceDisplayString(L"Level 10");
        runtime.mockGameContext.Assets().FindRichPresence()->Activate();
        Assert::AreEqual(std::string(""), runtime.GetRichPresenceOverride()); // no override
    }

    TEST_METHOD(TestRichPresenceOverrideInspectingMemoryCompatibilityMode)
    {
        AchievementRuntimeHarness runtime;
        runtime.mockGameContext.Assets().NewAchievement().SetCategory(ra::data::models::AssetCategory::Core);
        runtime.mockGameContext.SetMode(ra::data::context::GameContext::Mode::CompatibilityTest);
        runtime.MockInspectingMemory(true);
        Assert::AreEqual(std::string("Testing Compatibility"), runtime.GetRichPresenceOverride());
    }

    TEST_METHOD(TestRichPresenceOverrideInspectingMemoryNoAchievements)
    {
        AchievementRuntimeHarness runtime;
        runtime.MockInspectingMemory(true);
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        Assert::AreEqual(std::string("Inspecting Memory"), runtime.GetRichPresenceOverride());
    }

    TEST_METHOD(TestRichPresenceOverrideInspectingMemoryNoAchievementsHardcore)
    {
        AchievementRuntimeHarness runtime;
        runtime.MockInspectingMemory(true);
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        Assert::AreEqual(std::string("Inspecting Memory in Hardcore mode"), runtime.GetRichPresenceOverride());
    }

    TEST_METHOD(TestRichPresenceOverrideInspectingMemoryCoreAchievementsHardcore)
    {
        AchievementRuntimeHarness runtime;
        runtime.MockInspectingMemory(true);
        runtime.mockGameContext.Assets().NewAchievement().SetCategory(ra::data::models::AssetCategory::Core);
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        Assert::AreEqual(std::string("Inspecting Memory in Hardcore mode"), runtime.GetRichPresenceOverride());
    }

    TEST_METHOD(TestRichPresenceOverrideInspectingMemoryModifiedCoreAchievementsHardcore)
    {
        AchievementRuntimeHarness runtime;
        runtime.MockInspectingMemory(true);
        auto& pAch = runtime.mockGameContext.Assets().NewAchievement();
        pAch.SetCategory(ra::data::models::AssetCategory::Core);
        pAch.SetName(L"Modified");
        Assert::AreEqual(pAch.GetChanges(), ra::data::models::AssetChanges::Modified);
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        Assert::AreEqual(std::string("Inspecting Memory in Hardcore mode"), runtime.GetRichPresenceOverride());
    }

    TEST_METHOD(TestRichPresenceOverrideInspectingMemoryCoreAchievements)
    {
        AchievementRuntimeHarness runtime;
        runtime.MockInspectingMemory(true);
        auto& pAch = runtime.mockGameContext.Assets().NewAchievement();
        pAch.SetCategory(ra::data::models::AssetCategory::Core);
        pAch.UpdateServerCheckpoint();
        Assert::AreEqual(pAch.GetChanges(), ra::data::models::AssetChanges::None);
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        Assert::AreEqual(std::string("Inspecting Memory"), runtime.GetRichPresenceOverride());
    }

    TEST_METHOD(TestRichPresenceOverrideInspectingMemoryModifiedCoreAchievements)
    {
        AchievementRuntimeHarness runtime;
        runtime.MockInspectingMemory(true);
        auto& pAch = runtime.mockGameContext.Assets().NewAchievement();
        pAch.SetCategory(ra::data::models::AssetCategory::Core);
        pAch.SetName(L"Modified");
        Assert::AreEqual(pAch.GetChanges(), ra::data::models::AssetChanges::Modified);
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        Assert::AreEqual(std::string("Fixing Achievements"), runtime.GetRichPresenceOverride());
    }

    TEST_METHOD(TestRichPresenceOverrideInspectingMemoryUnpublishedCoreAchievements)
    {
        AchievementRuntimeHarness runtime;
        runtime.MockInspectingMemory(true);
        auto& pAch = runtime.mockGameContext.Assets().NewAchievement();
        pAch.SetCategory(ra::data::models::AssetCategory::Core);
        pAch.SetName(L"Modified");
        pAch.UpdateLocalCheckpoint();
        Assert::AreEqual(pAch.GetChanges(), ra::data::models::AssetChanges::Unpublished);
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        Assert::AreEqual(std::string("Fixing Achievements"), runtime.GetRichPresenceOverride());
    }

    TEST_METHOD(TestRichPresenceOverrideInspectingMemoryUnofficialAchievements)
    {
        AchievementRuntimeHarness runtime;
        runtime.MockInspectingMemory(true);
        auto& pAch = runtime.mockGameContext.Assets().NewAchievement();
        pAch.SetCategory(ra::data::models::AssetCategory::Unofficial);
        pAch.UpdateServerCheckpoint();
        Assert::AreEqual(pAch.GetChanges(), ra::data::models::AssetChanges::None);
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        Assert::AreEqual(std::string("Inspecting Memory"), runtime.GetRichPresenceOverride());
    }

    TEST_METHOD(TestRichPresenceOverrideInspectingMemoryModifiedUnofficialAchievements)
    {
        AchievementRuntimeHarness runtime;
        runtime.MockInspectingMemory(true);
        auto& pAch = runtime.mockGameContext.Assets().NewAchievement();
        pAch.SetCategory(ra::data::models::AssetCategory::Unofficial);
        pAch.SetName(L"Modified");
        Assert::AreEqual(pAch.GetChanges(), ra::data::models::AssetChanges::Modified);
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        Assert::AreEqual(std::string("Fixing Achievements"), runtime.GetRichPresenceOverride());
    }

    TEST_METHOD(TestRichPresenceOverrideInspectingMemoryLocalAchievements)
    {
        AchievementRuntimeHarness runtime;
        runtime.MockInspectingMemory(true);
        runtime.mockGameContext.Assets().NewAchievement().SetCategory(ra::data::models::AssetCategory::Local);
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        Assert::AreEqual(std::string("Developing Achievements"), runtime.GetRichPresenceOverride());
    }

    TEST_METHOD(TestRichPresenceOverrideInspectingMemoryLocalAndCoreAchievements)
    {
        AchievementRuntimeHarness runtime;
        runtime.MockInspectingMemory(true);
        runtime.mockGameContext.Assets().NewAchievement().SetCategory(ra::data::models::AssetCategory::Local);
        auto& pAch = runtime.mockGameContext.Assets().NewAchievement();
        pAch.SetCategory(ra::data::models::AssetCategory::Core);
        pAch.UpdateServerCheckpoint();
        Assert::AreEqual(pAch.GetChanges(), ra::data::models::AssetChanges::None);
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        Assert::AreEqual(std::string("Developing Achievements"), runtime.GetRichPresenceOverride());
    }

    TEST_METHOD(TestRichPresenceOverrideInspectingMemoryLocalAndModifiedCoreAchievements)
    {
        AchievementRuntimeHarness runtime;
        runtime.MockInspectingMemory(true);
        runtime.mockGameContext.Assets().NewAchievement().SetCategory(ra::data::models::AssetCategory::Local);
        auto& pAch = runtime.mockGameContext.Assets().NewAchievement();
        pAch.SetCategory(ra::data::models::AssetCategory::Core);
        pAch.SetName(L"Modified");
        Assert::AreEqual(pAch.GetChanges(), ra::data::models::AssetChanges::Modified);
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        Assert::AreEqual(std::string("Developing Achievements"), runtime.GetRichPresenceOverride());
    }

#ifdef RC_CLIENT_EXPORTS_EXTERNAL
    static void AssertV1Exports(rc_client_external_t& pClient)
    {
        Assert::IsNotNull((void*)pClient.destroy, L"destory not set");

        Assert::IsNotNull((void*)pClient.enable_logging, L"enable_logging not set");
        Assert::IsNotNull((void*)pClient.set_event_handler, L"set_event_handler not set");
        Assert::IsNotNull((void*)pClient.set_read_memory, L"set_read_memory not set");
        Assert::IsNotNull((void*)pClient.set_get_time_millisecs, L"set_get_time_millisecs not set");
        Assert::IsNotNull((void*)pClient.set_host, L"set_host not set");

        Assert::IsNotNull((void*)pClient.set_hardcore_enabled, L"set_hardcore_enabled not set");
        Assert::IsNotNull((void*)pClient.get_hardcore_enabled, L"get_hardcore_enabled not set");
        Assert::IsNotNull((void*)pClient.set_unofficial_enabled, L"set_unofficial_enabled not set");
        Assert::IsNotNull((void*)pClient.get_unofficial_enabled, L"get_unofficial_enabled not set");
        Assert::IsNotNull((void*)pClient.set_encore_mode_enabled, L"set_encore_mode_enabled not set");
        Assert::IsNotNull((void*)pClient.get_encore_mode_enabled, L"get_encore_mode_enabled not set");
        Assert::IsNotNull((void*)pClient.set_spectator_mode_enabled, L"set_spectator_mode_enabled not set");
        Assert::IsNotNull((void*)pClient.get_spectator_mode_enabled, L"get_spectator_mode_enabled not set");

        Assert::IsNotNull((void*)pClient.abort_async, L"abort_async not set");

        Assert::IsNotNull((void*)pClient.begin_login_with_password, L"begin_login_with_password not set");
        Assert::IsNotNull((void*)pClient.begin_login_with_token, L"begin_login_with_token not set");
        Assert::IsNotNull((void*)pClient.logout, L"logout not set");
        Assert::IsNotNull((void*)pClient.get_user_info, L"get_user_info not set");

        Assert::IsNotNull((void*)pClient.begin_identify_and_load_game, L"begin_identify_and_load_game not set");
        Assert::IsNotNull((void*)pClient.begin_load_game, L"begin_load_game not set");
        Assert::IsNotNull((void*)pClient.get_game_info, L"get_game_info not set");
        Assert::IsNull((void*)pClient.begin_load_subset, L"begin_load_subset set");
        Assert::IsNotNull((void*)pClient.get_subset_info, L"get_subset_info not set");
        Assert::IsNotNull((void*)pClient.unload_game, L"unload_game not set");
        Assert::IsNotNull((void*)pClient.get_user_game_summary, L"get_user_game_summary not set");
        Assert::IsNotNull((void*)pClient.begin_change_media, L"begin_change_media not set");

        Assert::IsNotNull((void*)pClient.create_achievement_list, L"create_achievement_list not set");
        Assert::IsNotNull((void*)pClient.has_achievements, L"has_achievements not set");
        Assert::IsNotNull((void*)pClient.get_achievement_info, L"get_achievement_info not set");

        Assert::IsNotNull((void*)pClient.create_leaderboard_list, L"create_leaderboard_list not set");
        Assert::IsNotNull((void*)pClient.has_leaderboards, L"has_leaderboards not set");
        Assert::IsNotNull((void*)pClient.get_leaderboard_info, L"get_leaderboard_info not set");
        Assert::IsNotNull((void*)pClient.begin_fetch_leaderboard_entries, L"begin_fetch_leaderboard_entries not set");
        Assert::IsNotNull((void*)pClient.begin_fetch_leaderboard_entries_around_user, L"begin_fetch_leaderboard_entries_around_user not set");

        Assert::IsNotNull((void*)pClient.get_rich_presence_message, L"get_rich_presence_message not set");
        Assert::IsNotNull((void*)pClient.has_rich_presence, L"has_rich_presence not set");

        Assert::IsNotNull((void*)pClient.do_frame, L"do_frame not set");
        Assert::IsNotNull((void*)pClient.idle, L"idle not set");
        Assert::IsNotNull((void*)pClient.is_processing_required, L"is_processing_required not set");
        Assert::IsNotNull((void*)pClient.can_pause, L"can_pause not set");
        Assert::IsNotNull((void*)pClient.reset, L"reset not set");

        Assert::IsNotNull((void*)pClient.progress_size, L"progress_size not set");
        Assert::IsNotNull((void*)pClient.serialize_progress, L"serialize_progress not set");
        Assert::IsNotNull((void*)pClient.deserialize_progress, L"deserialize_progress not set");
    }

    TEST_METHOD(TestGetExternalClientV1)
    {
        rc_client_external_t pClient;
        memset(&pClient, 0, sizeof(pClient));

        _Rcheevos_GetExternalClient(&pClient, 1);

        AssertV1Exports(pClient);
    }

    TEST_METHOD(TestRAIntegrationGetMenu)
    {
        AchievementRuntimeHarness runtime;
        runtime.mockUserContext.Logout();

        const rc_client_raintegration_menu_t* pMenu;

        auto AssertMenuItem = [&pMenu](int nIndex, uint32_t nId, const char* label) {
            const auto* pItem = &pMenu->items[nIndex];
            Assert::AreEqual(nId, pItem->id);
            Assert::AreEqual(std::string(label), std::string(pItem->label));
            Assert::AreEqual({1}, pItem->enabled);
            Assert::AreEqual({0}, pItem->checked);
        };

        auto AssertMenuItemChecked = [&pMenu](int nIndex, uint32_t nId, const char* label) {
            const auto* pItem = &pMenu->items[nIndex];
            Assert::AreEqual(nId, pItem->id);
            Assert::AreEqual(std::string(label), std::string(pItem->label));
            Assert::AreEqual({1}, pItem->enabled);
            Assert::AreEqual({1}, pItem->checked);
        };

        auto AssertMenuSeparator = [&pMenu](int nIndex) {
            const auto* pItem = &pMenu->items[nIndex];
            Assert::AreEqual(0U, pItem->id);
            Assert::IsNull(pItem->label);
            Assert::AreEqual({1}, pItem->enabled);
            Assert::AreEqual({0}, pItem->checked);
        };

        pMenu = _Rcheevos_RAIntegrationGetMenu();
        Assert::AreEqual(11U, pMenu->num_items);
        AssertMenuItem(0, IDM_RA_HARDCORE_MODE, "&Hardcore Mode");
        AssertMenuItem(1, IDM_RA_NON_HARDCORE_WARNING, "Non-Hardcore &Warning");
        AssertMenuSeparator(2);
        AssertMenuItem(3, IDM_RA_FILES_OPENALL, "&Open All");
        AssertMenuItem(4, IDM_RA_FILES_ACHIEVEMENTS, "Assets Li&st");
        AssertMenuItem(5, IDM_RA_FILES_ACHIEVEMENTEDITOR, "Assets &Editor");
        AssertMenuItem(6, IDM_RA_FILES_MEMORYFINDER, "&Memory Inspector");
        AssertMenuItem(7, IDM_RA_FILES_MEMORYBOOKMARKS, "Memory &Bookmarks");
        AssertMenuItem(8, IDM_RA_FILES_POINTERFINDER, "Pointer &Finder");
        AssertMenuItem(9, IDM_RA_FILES_CODENOTES, "Code &Notes");
        AssertMenuItem(10, IDM_RA_PARSERICHPRESENCE, "Rich &Presence Monitor");

        runtime.mockUserContext.Initialize("User", "ApiToken");

        pMenu = _Rcheevos_RAIntegrationGetMenu();
        Assert::AreEqual(17U, pMenu->num_items);
        AssertMenuItem(0, IDM_RA_OPENUSERPAGE, "Open my &User Page");
        AssertMenuItem(1, IDM_RA_OPENGAMEPAGE, "Open this &Game's Page");
        AssertMenuSeparator(2);
        AssertMenuItem(3, IDM_RA_HARDCORE_MODE, "&Hardcore Mode");
        AssertMenuItem(4, IDM_RA_NON_HARDCORE_WARNING, "Non-Hardcore &Warning");
        AssertMenuSeparator(5);
        AssertMenuItem(6, IDM_RA_FILES_OPENALL, "&Open All");
        AssertMenuItem(7, IDM_RA_FILES_ACHIEVEMENTS, "Assets Li&st");
        AssertMenuItem(8, IDM_RA_FILES_ACHIEVEMENTEDITOR, "Assets &Editor");
        AssertMenuItem(9, IDM_RA_FILES_MEMORYFINDER, "&Memory Inspector");
        AssertMenuItem(10, IDM_RA_FILES_MEMORYBOOKMARKS, "Memory &Bookmarks");
        AssertMenuItem(11, IDM_RA_FILES_POINTERFINDER, "Pointer &Finder");
        AssertMenuItem(12, IDM_RA_FILES_CODENOTES, "Code &Notes");
        AssertMenuItem(13, IDM_RA_PARSERICHPRESENCE, "Rich &Presence Monitor");
        AssertMenuSeparator(14);
        AssertMenuItem(15, IDM_RA_REPORTBROKENACHIEVEMENTS, "&Report Achievement Problem");
        AssertMenuItem(16, IDM_RA_GETROMCHECKSUM, "View Game H&ash");

        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::NonHardcoreWarning, true);

        pMenu = _Rcheevos_RAIntegrationGetMenu();
        Assert::AreEqual(17U, pMenu->num_items);
        AssertMenuItem(0, IDM_RA_OPENUSERPAGE, "Open my &User Page");
        AssertMenuItem(1, IDM_RA_OPENGAMEPAGE, "Open this &Game's Page");
        AssertMenuSeparator(2);
        AssertMenuItemChecked(3, IDM_RA_HARDCORE_MODE, "&Hardcore Mode");
        AssertMenuItemChecked(4, IDM_RA_NON_HARDCORE_WARNING, "Non-Hardcore &Warning");
        AssertMenuSeparator(5);
        AssertMenuItem(6, IDM_RA_FILES_OPENALL, "&Open All");
        AssertMenuItem(7, IDM_RA_FILES_ACHIEVEMENTS, "Assets Li&st");
        AssertMenuItem(8, IDM_RA_FILES_ACHIEVEMENTEDITOR, "Assets &Editor");
        AssertMenuItem(9, IDM_RA_FILES_MEMORYFINDER, "&Memory Inspector");
        AssertMenuItem(10, IDM_RA_FILES_MEMORYBOOKMARKS, "Memory &Bookmarks");
        AssertMenuItem(11, IDM_RA_FILES_POINTERFINDER, "Pointer &Finder");
        AssertMenuItem(12, IDM_RA_FILES_CODENOTES, "Code &Notes");
        AssertMenuItem(13, IDM_RA_PARSERICHPRESENCE, "Rich &Presence Monitor");
        AssertMenuSeparator(14);
        AssertMenuItem(15, IDM_RA_REPORTBROKENACHIEVEMENTS, "&Report Achievement Problem");
        AssertMenuItem(16, IDM_RA_GETROMCHECKSUM, "View Game H&ash");
    }
#endif
};

} // namespace tests
} // namespace services
} // namespace ra
