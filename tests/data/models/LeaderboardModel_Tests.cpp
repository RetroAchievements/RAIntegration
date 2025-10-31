#include "CppUnitTest.h"

#include "data\models\LeaderboardModel.hh"

#include "services\impl\StringTextWriter.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\data\DataAsserts.hh"

#include "tests\mocks\MockAchievementRuntime.hh"
#include "tests\mocks\MockGameContext.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace models {
namespace tests {

static bool g_bEventSeen = false; // must be global because callback cannot accept lambda

TEST_CLASS(LeaderboardModel_Tests)
{
private:
    class LeaderboardModelHarness : public LeaderboardModel
    {
    public:
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::services::impl::StringTextWriter textWriter;
        ra::services::mocks::MockAchievementRuntime mockRuntime;
    };

public:
    TEST_METHOD(TestInitialValues)
    {
        LeaderboardModelHarness leaderboard;

        Assert::AreEqual(AssetType::Leaderboard, leaderboard.GetType());
        Assert::AreEqual(0U, leaderboard.GetID());
        Assert::AreEqual(std::wstring(L""), leaderboard.GetName());
        Assert::AreEqual(std::wstring(L""), leaderboard.GetDescription());
        Assert::AreEqual(AssetCategory::Core, leaderboard.GetCategory());
        Assert::AreEqual(AssetState::Inactive, leaderboard.GetState());
        Assert::AreEqual(AssetChanges::None, leaderboard.GetChanges());
        Assert::AreEqual(std::string(""), leaderboard.GetStartTrigger());
        Assert::AreEqual(std::string(""), leaderboard.GetSubmitTrigger());
        Assert::AreEqual(std::string(""), leaderboard.GetCancelTrigger());
        Assert::AreEqual(std::string(""), leaderboard.GetValueDefinition());
    }

    TEST_METHOD(TestDeserialize)
    {
        LeaderboardModelHarness leaderboard;

        const std::string sSerialized = ":\"0xH1234=1\":\"0xH1234=2\":\"0xH1234=3\":\"0xH1234\":FRAMES:Title:Desc:0";
        ra::Tokenizer pTokenizer(sSerialized);
        pTokenizer.Consume(':');

        Assert::IsTrue(leaderboard.Deserialize(pTokenizer));

        Assert::AreEqual(std::string("0xH1234=1"), leaderboard.GetStartTrigger());
        Assert::AreEqual(std::string("0xH1234=2"), leaderboard.GetCancelTrigger());
        Assert::AreEqual(std::string("0xH1234=3"), leaderboard.GetSubmitTrigger());
        Assert::AreEqual(std::string("0xH1234"), leaderboard.GetValueDefinition());
        Assert::AreEqual(ValueFormat::Frames, leaderboard.GetValueFormat());
        Assert::AreEqual(std::wstring(L"Title"), leaderboard.GetName());
        Assert::AreEqual(std::wstring(L"Desc"), leaderboard.GetDescription());
        Assert::IsFalse(leaderboard.IsLowerBetter());
    }

    TEST_METHOD(TestDeserializeAlternateValues)
    {
        LeaderboardModelHarness leaderboard;

        const std::string sSerialized = ":\"0xH1234=1_P:0xH2345=2\":\"0xH1234<2\":\"0xH1234>3\":\"M:0xH1234\":MILLISECS:\"My Title\":\"My Desc\":1";
        ra::Tokenizer pTokenizer(sSerialized);
        pTokenizer.Consume(':');

        Assert::IsTrue(leaderboard.Deserialize(pTokenizer));

        Assert::AreEqual(std::string("0xH1234=1_P:0xH2345=2"), leaderboard.GetStartTrigger());
        Assert::AreEqual(std::string("0xH1234<2"), leaderboard.GetCancelTrigger());
        Assert::AreEqual(std::string("0xH1234>3"), leaderboard.GetSubmitTrigger());
        Assert::AreEqual(std::string("M:0xH1234"), leaderboard.GetValueDefinition());
        Assert::AreEqual(ValueFormat::Centiseconds, leaderboard.GetValueFormat());
        Assert::AreEqual(std::wstring(L"My Title"), leaderboard.GetName());
        Assert::AreEqual(std::wstring(L"My Desc"), leaderboard.GetDescription());
        Assert::IsTrue(leaderboard.IsLowerBetter());
    }

    TEST_METHOD(TestDeserializeNoLower)
    {
        LeaderboardModelHarness leaderboard;

        const std::string sSerialized = ":\"0xH1234=1\":\"0xH1234=2\":\"0xH1234=3\":\"0xH1234\":FRAMES:Title:Desc";
        ra::Tokenizer pTokenizer(sSerialized);
        pTokenizer.Consume(':');

        Assert::IsTrue(leaderboard.Deserialize(pTokenizer));

        Assert::AreEqual(std::string("0xH1234=1"), leaderboard.GetStartTrigger());
        Assert::AreEqual(std::string("0xH1234=2"), leaderboard.GetCancelTrigger());
        Assert::AreEqual(std::string("0xH1234=3"), leaderboard.GetSubmitTrigger());
        Assert::AreEqual(std::string("0xH1234"), leaderboard.GetValueDefinition());
        Assert::AreEqual(ValueFormat::Frames, leaderboard.GetValueFormat());
        Assert::AreEqual(std::wstring(L"Title"), leaderboard.GetName());
        Assert::AreEqual(std::wstring(L"Desc"), leaderboard.GetDescription());
        Assert::IsFalse(leaderboard.IsLowerBetter());
    }

    TEST_METHOD(TestDeserializeEmpty)
    {
        LeaderboardModelHarness leaderboard;

        const std::string sSerialized = ":\"\":\"\":\"\":\"\":::";
        ra::Tokenizer pTokenizer(sSerialized);
        pTokenizer.Consume(':');

        Assert::IsTrue(leaderboard.Deserialize(pTokenizer));

        Assert::AreEqual(std::string(), leaderboard.GetStartTrigger());
        Assert::AreEqual(std::string(), leaderboard.GetCancelTrigger());
        Assert::AreEqual(std::string(), leaderboard.GetSubmitTrigger());
        Assert::AreEqual(std::string(), leaderboard.GetValueDefinition());
        Assert::AreEqual(ValueFormat::Value, leaderboard.GetValueFormat());
        Assert::AreEqual(std::wstring(), leaderboard.GetName());
        Assert::AreEqual(std::wstring(), leaderboard.GetDescription());
        Assert::IsFalse(leaderboard.IsLowerBetter());
    }

    void TestSerializeValueFormat(ValueFormat nFormat, const std::string& sFormat)
    {
        LeaderboardModelHarness leaderboard;
        leaderboard.SetID(1U);
        leaderboard.SetName(L"Title");
        leaderboard.SetDescription(L"Desc");
        leaderboard.SetDefinition("STA:0xH1234=1::CAN:0xH1234=2::SUB:0xH1234=3::VAL:0xH1234");
        leaderboard.SetValueFormat(nFormat);
        leaderboard.CreateServerCheckpoint();
        leaderboard.CreateLocalCheckpoint();

        std::string sSerialized;
        ra::services::impl::StringTextWriter pTextWriter(sSerialized);
        leaderboard.Serialize(pTextWriter);

        std::string sExpected = ":\"0xH1234=1\":\"0xH1234=2\":\"0xH1234=3\":\"0xH1234\":" + sFormat + ":Title:Desc:0";
        Assert::AreEqual(sExpected, sSerialized);

        ra::Tokenizer pTokenizer(sSerialized);
        pTokenizer.Consume(':');
        LeaderboardModel leaderboard2;
        Assert::IsTrue(leaderboard2.Deserialize(pTokenizer));

        Assert::AreEqual(nFormat, leaderboard2.GetValueFormat());
    }

    TEST_METHOD(TestSerializeValueFormats)
    {
        TestSerializeValueFormat(ValueFormat::Score, "SCORE");
        TestSerializeValueFormat(ValueFormat::Value, "VALUE");
        TestSerializeValueFormat(ValueFormat::Frames, "TIME");
        TestSerializeValueFormat(ValueFormat::Centiseconds, "MILLISECS");
        TestSerializeValueFormat(ValueFormat::Seconds, "TIMESECS");
        TestSerializeValueFormat(ValueFormat::Minutes, "MINUTES");
        TestSerializeValueFormat(ValueFormat::SecondsAsMinutes, "SECS_AS_MINS");
        TestSerializeValueFormat(ValueFormat::Fixed1, "FIXED1");
        TestSerializeValueFormat(ValueFormat::Fixed2, "FIXED2");
        TestSerializeValueFormat(ValueFormat::Fixed3, "FIXED3");
        TestSerializeValueFormat(ValueFormat::Tens, "TENS");
        TestSerializeValueFormat(ValueFormat::Hundreds, "HUNDREDS");
        TestSerializeValueFormat(ValueFormat::Thousands, "THOUSANDS");
        TestSerializeValueFormat(ValueFormat::UnsignedValue, "UNSIGNED");
    }

    TEST_METHOD(TestTransactionalProperties)
    {
        LeaderboardModelHarness leaderboard;
        leaderboard.SetID(1U);
        leaderboard.SetName(L"Title");
        leaderboard.SetDescription(L"Desc");
        leaderboard.SetDefinition("STA:0xH1234=1::SUB:0xH1234=2::CAN:0xH1234=3::VAL:0xH1234");
        leaderboard.SetValueFormat(ValueFormat::Value);
        leaderboard.SetLowerIsBetter(true);
        leaderboard.CreateServerCheckpoint();
        leaderboard.CreateLocalCheckpoint();

        Assert::AreEqual(AssetChanges::None, leaderboard.GetChanges());

        leaderboard.SetValueFormat(ValueFormat::Score);
        Assert::AreEqual(AssetChanges::Modified, leaderboard.GetChanges());

        leaderboard.SetValueFormat(ValueFormat::Value);
        Assert::AreEqual(AssetChanges::None, leaderboard.GetChanges());

        leaderboard.SetLowerIsBetter(false);
        Assert::AreEqual(AssetChanges::Modified, leaderboard.GetChanges());

        leaderboard.SetLowerIsBetter(true);
        Assert::AreEqual(AssetChanges::None, leaderboard.GetChanges());
    }

    TEST_METHOD(TestValidateMissingField)
    {
        LeaderboardModelHarness leaderboard;
        leaderboard.SetID(1U);
        leaderboard.SetName(L"Title");
        leaderboard.SetDescription(L"Desc");
        leaderboard.SetValueFormat(ValueFormat::Value);
        leaderboard.SetLowerIsBetter(true);
        leaderboard.CreateServerCheckpoint();
        leaderboard.CreateLocalCheckpoint();

        Assert::IsFalse(leaderboard.Validate());
        Assert::AreEqual(std::wstring(L"No Start condition"), leaderboard.GetValidationError());

        leaderboard.SetStartTrigger("0xH1234=1");
        Assert::IsFalse(leaderboard.Validate());
        Assert::AreEqual(std::wstring(L"No Cancel condition"), leaderboard.GetValidationError());

        leaderboard.SetCancelTrigger("0xH1234=2");
        Assert::IsFalse(leaderboard.Validate());
        Assert::AreEqual(std::wstring(L"No Submit condition"), leaderboard.GetValidationError());

        leaderboard.SetSubmitTrigger("0xH1234=3");
        Assert::IsFalse(leaderboard.Validate());
        Assert::AreEqual(std::wstring(L"No Value definition"), leaderboard.GetValidationError());

        leaderboard.SetValueDefinition("0xH2345");
        Assert::IsTrue(leaderboard.Validate());
        Assert::AreEqual(std::wstring(), leaderboard.GetValidationError());
    }

    TEST_METHOD(TestValidateInvalidField)
    {
        LeaderboardModelHarness leaderboard;
        leaderboard.SetID(1U);
        leaderboard.SetName(L"Title");
        leaderboard.SetDescription(L"Desc");
        leaderboard.SetDefinition("STA:N:0xH1234=1::SUB:N:0xH1234=2::CAN:N:0xH1234=3::VAL:0xH1234");
        leaderboard.SetValueFormat(ValueFormat::Value);
        leaderboard.SetLowerIsBetter(true);
        leaderboard.CreateServerCheckpoint();
        leaderboard.CreateLocalCheckpoint();

        Assert::IsFalse(leaderboard.Validate());
        Assert::AreEqual(std::wstring(L"Start: Condition 1: AndNext condition type expects another condition to follow"), leaderboard.GetValidationError());

        leaderboard.SetStartTrigger("0xH1234=1");
        Assert::IsFalse(leaderboard.Validate());
        Assert::AreEqual(std::wstring(L"Cancel: Condition 1: AndNext condition type expects another condition to follow"), leaderboard.GetValidationError());

        leaderboard.SetCancelTrigger("0xH1234=2");
        Assert::IsFalse(leaderboard.Validate());
        Assert::AreEqual(std::wstring(L"Submit: Condition 1: AndNext condition type expects another condition to follow"), leaderboard.GetValidationError());

        leaderboard.SetSubmitTrigger("0xH1234=3");
        Assert::IsTrue(leaderboard.Validate());
        Assert::AreEqual(std::wstring(), leaderboard.GetValidationError());
    }

    TEST_METHOD(TestDeactivateHidesTracker)
    {
        LeaderboardModelHarness leaderboard;
        leaderboard.SetID(1U);
        leaderboard.SetName(L"Title");
        leaderboard.SetDescription(L"Desc");
        leaderboard.SetDefinition("STA:0xH1234=1::SUB:0xH1234=2::CAN:0xH1234=3::VAL:0xH1234");
        leaderboard.SetValueFormat(ValueFormat::Value);
        leaderboard.SetLowerIsBetter(true);
        leaderboard.CreateServerCheckpoint();
        leaderboard.CreateLocalCheckpoint();

        leaderboard.mockRuntime.MockGame();
        auto* leaderboard_info = leaderboard.mockRuntime.MockLeaderboardWithLboard(leaderboard.GetID());
        leaderboard.ReplaceAttached(*leaderboard_info);

        rc_client_allocate_leaderboard_tracker(leaderboard.mockRuntime.GetClient()->game, leaderboard_info);

        g_bEventSeen = false;
        leaderboard.mockRuntime.GetClient()->callbacks.event_handler =
            [](const rc_client_event_t* pEvent, rc_client_t*) {
                Assert::AreEqual({RC_CLIENT_EVENT_LEADERBOARD_TRACKER_HIDE}, pEvent->type);
                Assert::AreEqual({1}, pEvent->leaderboard_tracker->id);
                g_bEventSeen = true;
            };

        leaderboard.SetState(AssetState::Primed);
        Assert::IsTrue(leaderboard.IsActive());
        Assert::IsFalse(g_bEventSeen);

        leaderboard.SetState(AssetState::Inactive);
        Assert::AreEqual(AssetState::Inactive, leaderboard.GetState());
        Assert::IsFalse(leaderboard.IsActive());
        Assert::IsTrue(g_bEventSeen);
    }

    TEST_METHOD(TestDeactivateDoesntHideSharedTracker)
    {
        LeaderboardModelHarness leaderboard;
        leaderboard.SetID(1U);
        leaderboard.SetName(L"Title");
        leaderboard.SetDescription(L"Desc");
        leaderboard.SetDefinition("STA:0xH1234=1::SUB:0xH1234=2::CAN:0xH1234=3::VAL:0xH1234");
        leaderboard.SetValueFormat(ValueFormat::Value);
        leaderboard.SetLowerIsBetter(true);
        leaderboard.CreateServerCheckpoint();
        leaderboard.CreateLocalCheckpoint();

        leaderboard.mockRuntime.MockGame();
        auto* leaderboard_info = leaderboard.mockRuntime.MockLeaderboardWithLboard(leaderboard.GetID());
        leaderboard.ReplaceAttached(*leaderboard_info);

        rc_client_allocate_leaderboard_tracker(leaderboard.mockRuntime.GetClient()->game, leaderboard_info);
        auto* leaderboard_info2 = leaderboard.mockRuntime.MockLeaderboardWithLboard(99);
        rc_client_allocate_leaderboard_tracker(leaderboard.mockRuntime.GetClient()->game, leaderboard_info2);

        g_bEventSeen = false;
        leaderboard.mockRuntime.GetClient()->callbacks.event_handler =
            [](const rc_client_event_t*, rc_client_t*) {
                g_bEventSeen = true;
            };

        leaderboard.SetState(AssetState::Primed);
        Assert::IsTrue(leaderboard.IsActive());
        Assert::IsFalse(g_bEventSeen);

        leaderboard.SetState(AssetState::Inactive);
        Assert::AreEqual(AssetState::Inactive, leaderboard.GetState());
        Assert::IsFalse(leaderboard.IsActive());
        Assert::IsFalse(g_bEventSeen);
    }

    TEST_METHOD(TestDeleteHidesTracker)
    {
        LeaderboardModelHarness leaderboard;
        leaderboard.SetID(1U);
        leaderboard.SetName(L"Title");
        leaderboard.SetDescription(L"Desc");
        leaderboard.SetDefinition("STA:0xH1234=1::SUB:0xH1234=2::CAN:0xH1234=3::VAL:0xH1234");
        leaderboard.SetValueFormat(ValueFormat::Value);
        leaderboard.SetLowerIsBetter(true);
        leaderboard.CreateServerCheckpoint();
        leaderboard.CreateLocalCheckpoint();

        leaderboard.mockRuntime.MockGame();
        auto* leaderboard_info = leaderboard.mockRuntime.MockLeaderboardWithLboard(leaderboard.GetID());
        leaderboard.ReplaceAttached(*leaderboard_info);

        rc_client_allocate_leaderboard_tracker(leaderboard.mockRuntime.GetClient()->game, leaderboard_info);

        g_bEventSeen = false;
        leaderboard.mockRuntime.GetClient()->callbacks.event_handler =
            [](const rc_client_event_t* pEvent, rc_client_t*) {
                Assert::AreEqual({RC_CLIENT_EVENT_LEADERBOARD_TRACKER_HIDE}, pEvent->type);
                Assert::AreEqual({1}, pEvent->leaderboard_tracker->id);
                g_bEventSeen = true;
            };

        leaderboard.SetState(AssetState::Primed);
        Assert::IsTrue(leaderboard.IsActive());
        Assert::IsFalse(g_bEventSeen);

        leaderboard.SetDeleted();
        Assert::AreEqual(AssetChanges::Deleted, leaderboard.GetChanges());

        // deleting should deactivate the leaderboard, which should raise the indicator hide event
        Assert::AreEqual(AssetState::Inactive, leaderboard.GetState());
        Assert::IsFalse(leaderboard.IsActive());
        Assert::IsTrue(g_bEventSeen);
    }

    TEST_METHOD(TestEditMaintainsState)
    {
        LeaderboardModelHarness leaderboard;
        leaderboard.SetID(1U);
        leaderboard.SetName(L"Title");
        leaderboard.SetDescription(L"Desc");
        leaderboard.SetDefinition("STA:0xH1234=1::SUB:0xH1234=2::CAN:0xH1234=3::VAL:0xH1234");
        leaderboard.SetValueFormat(ValueFormat::Value);
        leaderboard.SetLowerIsBetter(true);
        leaderboard.CreateServerCheckpoint();
        leaderboard.CreateLocalCheckpoint();

        leaderboard.mockRuntime.MockGame();
        auto* leaderboard_info = leaderboard.mockRuntime.MockLeaderboard(leaderboard.GetID());
        leaderboard.ReplaceAttached(*leaderboard_info);

        // forcefully start the leaderboard
        leaderboard.SetState(AssetState::Primed);
        const auto* pLboard = leaderboard_info->lboard;
        Assert::IsNotNull(pLboard);
        rc_client_allocate_leaderboard_tracker(leaderboard.mockRuntime.GetClient()->game, leaderboard_info);

        Assert::IsTrue(leaderboard.IsActive());
        Assert::AreEqual({ RC_LBOARD_STATE_STARTED }, leaderboard_info->lboard->state);

        // change definition should maintain state
        leaderboard.SetDefinition("STA:0xH1234=2::SUB:0xH1234=2::CAN:0xH1234=3::VAL:0xH1234");
        Assert::AreEqual(AssetState::Primed, leaderboard.GetState());
        Assert::IsTrue(leaderboard.IsActive());
        Assert::AreEqual({RC_LBOARD_STATE_STARTED}, leaderboard_info->lboard->state);

        if (pLboard == leaderboard_info->lboard)
            Assert::Fail(L"new rc_lboard_t not allocated");
    }
};

} // namespace tests
} // namespace models
} // namespace data
} // namespace ra
