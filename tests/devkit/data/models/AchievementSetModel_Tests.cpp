#include "data/models/AchievementSetModel.hh"

#include "context/mocks/MockEmulatorMemoryContext.hh"
#include "context/mocks/MockGameContext.hh"
#include "context/mocks/MockRcClient.hh"
#include "context/mocks/MockUserContext.hh"

#include "data/models/GameAssets.hh"
#include "data/models/LocalBadgesModel.hh"

#include "services/impl/StringTextWriter.hh"
#include "services/mocks/MockClock.hh"

#include "testutil/AchievementAsserts.hh"
#include "testutil/AssetAsserts.hh"
#include "testutil/CppUnitTest.hh"

#include "ui/mocks/MockImageRepository.hh"

#include <rcheevos/src/rc_client_internal.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace models {
namespace tests {

static bool g_bEventSeen = false; // must be global because callback cannot accept lambda

TEST_CLASS(AchievementSetModel_Tests)
{
private:
    class AchievementSetModelHarness : public AchievementSetModel
    {
    public:
        AchievementSetModelHarness() noexcept
        {
            GSL_SUPPRESS_F6 mockRcClient.MockGame(1, "Game Title");
            GSL_SUPPRESS_F6 mockRcClient.MockCoreSubset();
        }

        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        ra::context::mocks::MockGameContext mockGameContext;
        ra::context::mocks::MockRcClient mockRcClient;
        ra::data::models::GameAssets assets;
        ra::ui::mocks::MockImageRepository mockImageRepository;

        void AddAchievementModel(rc_client_achievement_info_t& pAchievement)
        {
            auto vmAchievement = std::make_unique<ra::data::models::AchievementModel>();
            vmAchievement->InitializeFromPublishedAchievement(pAchievement, "0xH0000=1");
            vmAchievement->SetSubsetID(mockRcClient.GetClient()->game->subsets->public_.id);
            assets.Append(std::move(vmAchievement));
        }

        void AddLeaderboardModel(rc_client_leaderboard_info_t& pLeaderboard)
        {
            auto vmLeaderboard = std::make_unique<ra::data::models::LeaderboardModel>();
            vmLeaderboard->InitializeFromPublishedLeaderboard(pLeaderboard, "STA:0xH0000=1::SUB:1=1::CAN:0=1::VAL:0xH0001");
            vmLeaderboard->SetSubsetID(mockRcClient.GetClient()->game->subsets->public_.id);
            assets.Append(std::move(vmLeaderboard));
        }

        void SyncAssets()
        {
            SyncToRuntime(*mockRcClient.GetClient()->game->subsets, assets);
        }
    };

public:
    TEST_METHOD(TestSyncNothing)
    {
        AchievementSetModelHarness set;
        set.SyncAssets();

        const auto* pSubset = set.GetPublishedSubsetInfo();
        Expects(pSubset != nullptr);
        Assert::AreEqual("Game Title", pSubset->public_.title);
        Assert::AreEqual(1U, pSubset->public_.id);
        Assert::AreEqual("55443", pSubset->public_.badge_name);
        Assert::AreEqual(0U, pSubset->public_.num_achievements);
        Assert::AreEqual(0U, pSubset->public_.num_leaderboards);
        Assert::IsFalse(pSubset->active);

        pSubset = set.GetLocalSubsetInfo();
        Expects(pSubset != nullptr);
        Assert::AreEqual("Local", pSubset->public_.title);
        Assert::AreEqual(ra::data::models::AchievementSetModel::LocalId, pSubset->public_.id);
        Assert::AreEqual("55443", pSubset->public_.badge_name);
        Assert::AreEqual(0U, pSubset->public_.num_achievements);
        Assert::AreEqual(0U, pSubset->public_.num_leaderboards);
        Assert::IsFalse(pSubset->active);
        Assert::IsNull(pSubset->next);
    }

    TEST_METHOD(TestSyncPublishedAchievements)
    {
        AchievementSetModelHarness set;
        auto* ach3 = set.mockRcClient.MockAchievement(3U, "First");
        ach3->public_.points = 5;
        set.AddAchievementModel(*ach3);
        auto* ach6 = set.mockRcClient.MockAchievement(6U, "Second");
        ach6->public_.points = 10;
        set.AddAchievementModel(*ach6);

        set.SyncAssets();

        auto* pSubset = set.GetPublishedSubsetInfo();
        Expects(pSubset != nullptr);
        Assert::AreEqual("Game Title", pSubset->public_.title);
        Assert::AreEqual(1U, pSubset->public_.id);
        Assert::AreEqual("55443", pSubset->public_.badge_name);
        Assert::AreEqual(2U, pSubset->public_.num_achievements);
        Assert::AreEqual(0U, pSubset->public_.num_leaderboards);
        Assert::IsTrue(pSubset->active);

        auto* pAchievement = pSubset->achievements;
        Assert::AreEqual("First", pAchievement->public_.title);
        Assert::AreEqual(5U, pAchievement->public_.points);
        Assert::AreEqual(3U, pAchievement->public_.id);
        Assert::IsNotNull(pAchievement->trigger);
        ++pAchievement;
        Assert::AreEqual("Second", pAchievement->public_.title);
        Assert::AreEqual(10U, pAchievement->public_.points);
        Assert::AreEqual(6U, pAchievement->public_.id);
        Assert::IsNotNull(pAchievement->trigger);
    }

    TEST_METHOD(TestSyncLocalAchievement)
    {
        AchievementSetModelHarness set;

        auto vmAchievement = std::make_unique<ra::data::models::AchievementModel>();
        vmAchievement->CreateServerCheckpoint();
        vmAchievement->SetCategory(ra::data::models::AssetCategory::Local);
        vmAchievement->SetName(L"Achievement Name");
        vmAchievement->SetDescription(L"Do something cool");
        vmAchievement->SetPoints(25);
        vmAchievement->SetTrigger("0xH0000=1");
        vmAchievement->SetID(ra::data::models::GameAssets::FirstLocalId);
        vmAchievement->SetBadge(L"local\\ABCDEF0123456789.png");
        vmAchievement->CreateLocalCheckpoint();
        vmAchievement->SetState(ra::data::models::AssetState::Active);
        vmAchievement->SetSubsetID(1U);
        set.assets.Append(std::move(vmAchievement));

        set.SyncAssets();

        auto* pSubset = set.GetPublishedSubsetInfo();
        Expects(pSubset != nullptr);
        Assert::AreEqual(0U, pSubset->public_.num_achievements);
        Assert::IsFalse(pSubset->active);

        pSubset = set.GetLocalSubsetInfo();
        Expects(pSubset != nullptr);
        Assert::AreEqual("Local", pSubset->public_.title);
        Assert::AreEqual(1U, pSubset->public_.num_achievements);
        Assert::IsTrue(pSubset->active);

        const auto* pAchievement = pSubset->achievements;
        Assert::AreEqual("Achievement Name", pAchievement->public_.title);
        Assert::AreEqual("Do something cool", pAchievement->public_.description);
        Assert::AreEqual(25U, pAchievement->public_.points);
        Assert::AreEqual(ra::data::models::GameAssets::FirstLocalId, pAchievement->public_.id);
        Assert::AreEqual("L69db9c", pAchievement->public_.badge_name); // FirstLocalId as hex
        Assert::AreEqual("file://RACache/Badges/local/ABCDEF0123456789.png", pAchievement->public_.badge_url);
        Assert::AreEqual("file://RACache/Badges/local/ABCDEF0123456789.png", pAchievement->public_.badge_locked_url);
        Assert::IsNotNull(pAchievement->trigger);
    }

    TEST_METHOD(TestSyncPublishedAchievementWithLocalChanges)
    {
        AchievementSetModelHarness set;
        auto* ach3 = set.mockRcClient.MockAchievement(3U, "First");
        ach3->public_.points = 5;
        set.AddAchievementModel(*ach3);

        auto* vmAchievement = set.assets.FindAchievement(3U);
        Expects(vmAchievement != nullptr);
        vmAchievement->SetPoints(25);
        vmAchievement->SetBadge(L"local\\ABCDEF0123456789.png");

        set.SyncAssets();

        auto* pSubset = set.GetPublishedSubsetInfo();
        Expects(pSubset != nullptr);
        Assert::AreEqual(1U, pSubset->public_.num_achievements);
        Assert::IsTrue(pSubset->active);

        auto* pAchievement = pSubset->achievements;
        Assert::AreEqual("First", pAchievement->public_.title);
        Assert::AreEqual(25U, pAchievement->public_.points);
        Assert::AreEqual(3U, pAchievement->public_.id);
        Assert::AreEqual("L000003", pAchievement->public_.badge_name); // FirstLocalId as hex
        Assert::AreEqual("file://RACache/Badges/local/ABCDEF0123456789.png", pAchievement->public_.badge_url);
        Assert::AreEqual("file://RACache/Badges/local/ABCDEF0123456789.png", pAchievement->public_.badge_locked_url);
        Assert::IsNotNull(pAchievement->trigger);

        pSubset = set.GetLocalSubsetInfo();
        Expects(pSubset != nullptr);
        Assert::AreEqual(0U, pSubset->public_.num_achievements);
        Assert::IsFalse(pSubset->active);
    }

    TEST_METHOD(TestSyncPromotedAchievement)
    {
        AchievementSetModelHarness set;

        auto vmAchievement = std::make_unique<ra::data::models::AchievementModel>();
        vmAchievement->CreateServerCheckpoint();
        vmAchievement->SetCategory(ra::data::models::AssetCategory::Local);
        vmAchievement->SetName(L"Achievement Name");
        vmAchievement->SetDescription(L"Do something cool");
        vmAchievement->SetPoints(25);
        vmAchievement->SetTrigger("0xH0000=1");
        vmAchievement->SetID(ra::data::models::GameAssets::FirstLocalId);
        vmAchievement->SetBadge(L"local\\ABCDEF0123456789.png");
        vmAchievement->CreateLocalCheckpoint();
        vmAchievement->SetState(ra::data::models::AssetState::Active);
        vmAchievement->SetSubsetID(1U);
        set.assets.Append(std::move(vmAchievement));

        set.SyncAssets();

        auto* pSubset = set.GetPublishedSubsetInfo();
        Expects(pSubset != nullptr);
        Assert::AreEqual(0U, pSubset->public_.num_achievements);
        Assert::IsFalse(pSubset->active);

        pSubset = set.GetLocalSubsetInfo();
        Expects(pSubset != nullptr);
        Assert::AreEqual("Local", pSubset->public_.title);
        Assert::AreEqual(1U, pSubset->public_.num_achievements);
        Assert::IsTrue(pSubset->active);

        set.assets.FindAchievement(ra::data::models::GameAssets::FirstLocalId)->SetCategory(ra::data::models::AssetCategory::Unofficial);
        set.SyncAssets();

        pSubset = set.GetLocalSubsetInfo();
        Expects(pSubset != nullptr);
        Assert::AreEqual(0U, pSubset->public_.num_achievements);
        Assert::IsFalse(pSubset->active);

        pSubset = set.GetPublishedSubsetInfo();
        Expects(pSubset != nullptr);
        Assert::AreEqual(1U, pSubset->public_.num_achievements);
        Assert::IsTrue(pSubset->active);

        const auto* pAchievement = pSubset->achievements;
        Assert::AreEqual("Achievement Name", pAchievement->public_.title);
        Assert::AreEqual("Do something cool", pAchievement->public_.description);
        Assert::AreEqual(25U, pAchievement->public_.points);
        Assert::AreEqual(ra::data::models::GameAssets::FirstLocalId, pAchievement->public_.id);
        Assert::AreEqual("L69db9c", pAchievement->public_.badge_name); // FirstLocalId as hex
        Assert::AreEqual("file://RACache/Badges/local/ABCDEF0123456789.png", pAchievement->public_.badge_url);
        Assert::AreEqual("file://RACache/Badges/local/ABCDEF0123456789.png", pAchievement->public_.badge_locked_url);
        Assert::IsNotNull(pAchievement->trigger);
    }

    TEST_METHOD(TestSyncPublishedLeaderboards)
    {
        AchievementSetModelHarness set;
        auto* lbd3 = set.mockRcClient.MockLeaderboard(3U, "First");
        lbd3->format = RC_FORMAT_FIXED1;
        set.AddLeaderboardModel(*lbd3);
        auto* lbd6 = set.mockRcClient.MockLeaderboard(6U, "Second");
        lbd6->format = RC_FORMAT_FLOAT1;
        set.AddLeaderboardModel(*lbd6);

        set.SyncAssets();

        auto* pSubset = set.GetPublishedSubsetInfo();
        Expects(pSubset != nullptr);
        Assert::AreEqual(0U, pSubset->public_.num_achievements);
        Assert::AreEqual(2U, pSubset->public_.num_leaderboards);
        Assert::IsTrue(pSubset->active);

        auto* pLeaderboard = pSubset->leaderboards;
        Assert::AreEqual("First", pLeaderboard->public_.title);
        Assert::AreEqual({ RC_FORMAT_FIXED1 }, pLeaderboard->format);
        Assert::AreEqual(3U, pLeaderboard->public_.id);
        Assert::IsNotNull(pLeaderboard->lboard);
        ++pLeaderboard;
        Assert::AreEqual("Second", pLeaderboard->public_.title);
        Assert::AreEqual({ RC_FORMAT_FLOAT1 }, pLeaderboard->format);
        Assert::AreEqual(6U, pLeaderboard->public_.id);
        Assert::IsNotNull(pLeaderboard->lboard);
    }

    TEST_METHOD(TestSyncLocalLeaderboard)
    {
        AchievementSetModelHarness set;

        auto vmLeaderboard = std::make_unique<ra::data::models::LeaderboardModel>();
        vmLeaderboard->CreateServerCheckpoint();
        vmLeaderboard->SetCategory(ra::data::models::AssetCategory::Local);
        vmLeaderboard->SetName(L"Leaderboard Name");
        vmLeaderboard->SetDescription(L"Do something cool");
        vmLeaderboard->SetValueFormat(ra::data::Value::Format::Centiseconds);
        vmLeaderboard->SetDefinition("STA:0xH0000=1::SUB:1=1::CAN:0=1::VAL=0xH0001");
        vmLeaderboard->SetID(ra::data::models::GameAssets::FirstLocalId);
        vmLeaderboard->CreateLocalCheckpoint();
        vmLeaderboard->SetState(ra::data::models::AssetState::Active);
        vmLeaderboard->SetSubsetID(1U);
        set.assets.Append(std::move(vmLeaderboard));

        set.SyncAssets();

        auto* pSubset = set.GetPublishedSubsetInfo();
        Expects(pSubset != nullptr);
        Assert::AreEqual(0U, pSubset->public_.num_achievements);
        Assert::AreEqual(0U, pSubset->public_.num_leaderboards);
        Assert::IsFalse(pSubset->active);

        pSubset = set.GetLocalSubsetInfo();
        Expects(pSubset != nullptr);
        Assert::AreEqual("Local", pSubset->public_.title);
        Assert::AreEqual(0U, pSubset->public_.num_achievements);
        Assert::AreEqual(1U, pSubset->public_.num_leaderboards);
        Assert::IsTrue(pSubset->active);

        const auto* pLeaderboard = pSubset->leaderboards;
        Assert::AreEqual("Leaderboard Name", pLeaderboard->public_.title);
        Assert::AreEqual("Do something cool", pLeaderboard->public_.description);
        Assert::AreEqual({ RC_FORMAT_CENTISECS }, pLeaderboard->format);
        Assert::AreEqual({ RC_CLIENT_LEADERBOARD_FORMAT_TIME }, pLeaderboard->public_.format);
        Assert::AreEqual(ra::data::models::GameAssets::FirstLocalId, pLeaderboard->public_.id);
        Assert::IsNotNull(pLeaderboard->lboard);
    }

    TEST_METHOD(TestSyncPromotedLeaderboard)
    {
        AchievementSetModelHarness set;

        auto vmLeaderboard = std::make_unique<ra::data::models::LeaderboardModel>();
        vmLeaderboard->CreateServerCheckpoint();
        vmLeaderboard->SetCategory(ra::data::models::AssetCategory::Local);
        vmLeaderboard->SetName(L"Leaderboard Name");
        vmLeaderboard->SetDescription(L"Do something cool");
        vmLeaderboard->SetValueFormat(ra::data::Value::Format::Centiseconds);
        vmLeaderboard->SetDefinition("STA:0xH0000=1::SUB:1=1::CAN:0=1::VAL=0xH0001");
        vmLeaderboard->SetID(ra::data::models::GameAssets::FirstLocalId);
        vmLeaderboard->CreateLocalCheckpoint();
        vmLeaderboard->SetState(ra::data::models::AssetState::Active);
        vmLeaderboard->SetSubsetID(1U);
        set.assets.Append(std::move(vmLeaderboard));

        set.SyncAssets();

        auto* pSubset = set.GetPublishedSubsetInfo();
        Expects(pSubset != nullptr);
        Assert::AreEqual(0U, pSubset->public_.num_leaderboards);
        Assert::IsFalse(pSubset->active);

        pSubset = set.GetLocalSubsetInfo();
        Expects(pSubset != nullptr);
        Assert::AreEqual("Local", pSubset->public_.title);
        Assert::AreEqual(1U, pSubset->public_.num_leaderboards);
        Assert::IsTrue(pSubset->active);

        set.assets.FindLeaderboard(ra::data::models::GameAssets::FirstLocalId)->SetCategory(ra::data::models::AssetCategory::Core);
        set.SyncAssets();

        pSubset = set.GetLocalSubsetInfo();
        Expects(pSubset != nullptr);
        Assert::AreEqual(0U, pSubset->public_.num_leaderboards);
        Assert::IsFalse(pSubset->active);

        pSubset = set.GetPublishedSubsetInfo();
        Expects(pSubset != nullptr);
        Assert::AreEqual(1U, pSubset->public_.num_leaderboards);
        Assert::IsTrue(pSubset->active);

        const auto* pLeaderboard = pSubset->leaderboards;
        Assert::AreEqual("Leaderboard Name", pLeaderboard->public_.title);
        Assert::AreEqual("Do something cool", pLeaderboard->public_.description);
        Assert::AreEqual({ RC_FORMAT_CENTISECS }, pLeaderboard->format);
        Assert::AreEqual({ RC_CLIENT_LEADERBOARD_FORMAT_TIME }, pLeaderboard->public_.format);
        Assert::AreEqual(ra::data::models::GameAssets::FirstLocalId, pLeaderboard->public_.id);
        Assert::IsNotNull(pLeaderboard->lboard);
    }

    TEST_METHOD(TestSyncPublishedLeaderboardWithLocalChanges)
    {
        AchievementSetModelHarness set;
        auto* lbd3 = set.mockRcClient.MockLeaderboard(3U, "First");
        lbd3->format = RC_FORMAT_SCORE;
        set.AddLeaderboardModel(*lbd3);

        auto* vmLeaderboard
        = set.assets.FindLeaderboard(3U);
        Expects(vmLeaderboard != nullptr);
        vmLeaderboard->SetValueFormat(ra::data::Value::Format::UnsignedValue);

        set.SyncAssets();

        auto* pSubset = set.GetPublishedSubsetInfo();
        Expects(pSubset != nullptr);
        Assert::AreEqual(1U, pSubset->public_.num_leaderboards);
        Assert::IsTrue(pSubset->active);

        auto* pLeaderboard = pSubset->leaderboards;
        Assert::AreEqual("First", pLeaderboard->public_.title);
        Assert::AreEqual({ RC_FORMAT_UNSIGNED_VALUE }, pLeaderboard->format);
        Assert::AreEqual(3U, pLeaderboard->public_.id);
        Assert::IsNotNull(pLeaderboard->lboard);

        pSubset = set.GetLocalSubsetInfo();
        Expects(pSubset != nullptr);
        Assert::AreEqual(0U, pSubset->public_.num_leaderboards);
        Assert::IsFalse(pSubset->active);
    }

};


} // namespace tests
} // namespace models
} // namespace data
} // namespace ra
