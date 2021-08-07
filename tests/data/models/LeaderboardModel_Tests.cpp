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

    TEST_METHOD(TestActivateDeactivate)
    {
        LeaderboardModelHarness leaderboard;
        leaderboard.SetID(1U);
        leaderboard.SetDefinition("STA:0xH1234=1::SUB:0xH1234=2::CAN:0xH1234=3::VAL:0xH1234");
        leaderboard.CreateServerCheckpoint();
        leaderboard.CreateLocalCheckpoint();

        Assert::AreEqual(std::string("0xH1234=1"), leaderboard.GetStartTrigger());
        Assert::AreEqual(std::string("0xH1234=2"), leaderboard.GetSubmitTrigger());
        Assert::AreEqual(std::string("0xH1234=3"), leaderboard.GetCancelTrigger());
        Assert::AreEqual(std::string("0xH1234"), leaderboard.GetValueDefinition());

        Assert::AreEqual(AssetState::Inactive, leaderboard.GetState());
        Assert::IsNull(leaderboard.mockRuntime.GetLeaderboardDefinition(1U));

        leaderboard.Activate();

        Assert::AreEqual(AssetState::Waiting, leaderboard.GetState());
        const auto* pLeaderboard = leaderboard.mockRuntime.GetLeaderboardDefinition(1U);
        Assert::IsNotNull(pLeaderboard);
        Assert::AreEqual((int)RC_LBOARD_STATE_WAITING, (int)pLeaderboard->state);
        Assert::AreEqual(1U, pLeaderboard->start.requirement->conditions->operand2.value.num);
        Assert::AreEqual(2U, pLeaderboard->submit.requirement->conditions->operand2.value.num);
        Assert::AreEqual(3U, pLeaderboard->cancel.requirement->conditions->operand2.value.num);

        leaderboard.Deactivate();

        Assert::AreEqual(AssetState::Inactive, leaderboard.GetState());
        pLeaderboard = leaderboard.mockRuntime.GetLeaderboardDefinition(1U);
        Assert::IsNull(pLeaderboard);
    }

    TEST_METHOD(TestActivateDeactivateActive)
    {
        LeaderboardModelHarness leaderboard;
        leaderboard.SetID(1U);
        leaderboard.SetDefinition("STA:0xH1234=1::SUB:0xH1234=2::CAN:0xH1234=3::VAL:0xH1234");
        leaderboard.CreateServerCheckpoint();
        leaderboard.CreateLocalCheckpoint();
        leaderboard.Activate();

        auto* pLeaderboard = leaderboard.mockRuntime.GetLeaderboardDefinition(1U);
        Expects(pLeaderboard != nullptr);
        pLeaderboard->state = RC_LBOARD_STATE_ACTIVE;
        leaderboard.DoFrame();
        Assert::AreEqual(AssetState::Active, leaderboard.GetState());

        leaderboard.Activate();
        Assert::AreEqual(AssetState::Active, leaderboard.GetState());
        pLeaderboard = leaderboard.mockRuntime.GetLeaderboardDefinition(1U);
        Assert::IsNotNull(pLeaderboard);
        Assert::AreEqual((int)RC_LBOARD_STATE_ACTIVE, (int)pLeaderboard->state);

        leaderboard.Deactivate();
        Assert::AreEqual(AssetState::Inactive, leaderboard.GetState());
        pLeaderboard = leaderboard.mockRuntime.GetLeaderboardDefinition(1U);
        Assert::IsNull(pLeaderboard);
    }

    TEST_METHOD(TestActivateDeactivateCanceled)
    {
        LeaderboardModelHarness leaderboard;
        leaderboard.SetID(1U);
        leaderboard.SetDefinition("STA:0xH1234=1::SUB:0xH1234=2::CAN:0xH1234=3::VAL:0xH1234");
        leaderboard.CreateServerCheckpoint();
        leaderboard.CreateLocalCheckpoint();
        leaderboard.Activate();

        // canceled means the leaderboard was primed, but was canceled before submitting.
        // it should still be active, waiting for the start condition to become false.
        auto* pLeaderboard = leaderboard.mockRuntime.GetLeaderboardDefinition(1U);
        Expects(pLeaderboard != nullptr);
        pLeaderboard->state = RC_LBOARD_STATE_CANCELED;
        leaderboard.DoFrame();
        Assert::AreEqual(AssetState::Waiting, leaderboard.GetState());

        leaderboard.Activate();
        Assert::AreEqual(AssetState::Waiting, leaderboard.GetState());
        pLeaderboard = leaderboard.mockRuntime.GetLeaderboardDefinition(1U);
        Assert::IsNotNull(pLeaderboard);
        Assert::AreEqual((int)RC_LBOARD_STATE_CANCELED, (int)pLeaderboard->state);

        leaderboard.Deactivate();
        Assert::AreEqual(AssetState::Inactive, leaderboard.GetState());
        pLeaderboard = leaderboard.mockRuntime.GetLeaderboardDefinition(1U);
        Assert::IsNull(pLeaderboard);
    }

    TEST_METHOD(TestActivateDeactivateTriggered)
    {
        LeaderboardModelHarness leaderboard;
        leaderboard.SetID(1U);
        leaderboard.SetDefinition("STA:0xH1234=1::SUB:0xH1234=2::CAN:0xH1234=3::VAL:0xH1234");
        leaderboard.CreateServerCheckpoint();
        leaderboard.CreateLocalCheckpoint();
        leaderboard.Activate();

        // triggered means the leaderboard was submitted.
        // it should still be active, waiting for the start condition to become false
        auto* pLeaderboard = leaderboard.mockRuntime.GetLeaderboardDefinition(1U);
        Expects(pLeaderboard != nullptr);
        pLeaderboard->state = RC_LBOARD_STATE_TRIGGERED;
        leaderboard.DoFrame();
        Assert::AreEqual(AssetState::Waiting, leaderboard.GetState());

        leaderboard.Activate();
        Assert::AreEqual(AssetState::Waiting, leaderboard.GetState());
        pLeaderboard = leaderboard.mockRuntime.GetLeaderboardDefinition(1U);
        Assert::IsNotNull(pLeaderboard);
        Assert::AreEqual((int)RC_LBOARD_STATE_TRIGGERED, (int)pLeaderboard->state);

        leaderboard.Deactivate();
        Assert::AreEqual(AssetState::Inactive, leaderboard.GetState());
        pLeaderboard = leaderboard.mockRuntime.GetLeaderboardDefinition(1U);
        Assert::IsNull(pLeaderboard);
    }

    TEST_METHOD(TestActivateDeactivatePrimed)
    {
        LeaderboardModelHarness leaderboard;
        leaderboard.SetID(1U);
        leaderboard.SetDefinition("STA:0xH1234=1::SUB:0xH1234=2::CAN:0xH1234=3::VAL:0xH1234");
        leaderboard.CreateServerCheckpoint();
        leaderboard.CreateLocalCheckpoint();
        leaderboard.Activate();

        // started means the leaderboard is engaged (showing on screen)
        auto* pLeaderboard = leaderboard.mockRuntime.GetLeaderboardDefinition(1U);
        Expects(pLeaderboard != nullptr);
        pLeaderboard->state = RC_LBOARD_STATE_STARTED;
        leaderboard.DoFrame();
        Assert::AreEqual(AssetState::Primed, leaderboard.GetState());

        leaderboard.Activate();
        Assert::AreEqual(AssetState::Primed, leaderboard.GetState());
        pLeaderboard = leaderboard.mockRuntime.GetLeaderboardDefinition(1U);
        Assert::IsNotNull(pLeaderboard);
        Assert::AreEqual((int)RC_LBOARD_STATE_STARTED, (int)pLeaderboard->state);

        leaderboard.Deactivate();
        Assert::AreEqual(AssetState::Inactive, leaderboard.GetState());
        pLeaderboard = leaderboard.mockRuntime.GetLeaderboardDefinition(1U);
        Assert::IsNull(pLeaderboard);
    }

    void TestSerializeValueFormat(ValueFormat nFormat, const std::string& sFormat)
    {
        LeaderboardModelHarness leaderboard;
        leaderboard.SetID(1U);
        leaderboard.SetName(L"Title");
        leaderboard.SetDescription(L"Desc");
        leaderboard.SetDefinition("STA:0xH1234=1::SUB:0xH1234=2::CAN:0xH1234=3::VAL:0xH1234");
        leaderboard.SetValueFormat(nFormat);
        leaderboard.CreateServerCheckpoint();
        leaderboard.CreateLocalCheckpoint();

        std::string sSerialized;
        ra::services::impl::StringTextWriter pTextWriter(sSerialized);
        leaderboard.Serialize(pTextWriter);

        std::string sExpected = ":\"0xH1234=1\":\"0xH1234=2\":\"0xH1234=3\":\"0xH1234\":" + sFormat + ":Title:Desc";
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
        TestSerializeValueFormat(ValueFormat::Frames, "FRAMES");
        TestSerializeValueFormat(ValueFormat::Centiseconds, "MILLISECS");
        TestSerializeValueFormat(ValueFormat::Seconds, "SECS");
        TestSerializeValueFormat(ValueFormat::Minutes, "MINUTES");
        TestSerializeValueFormat(ValueFormat::SecondsAsMinutes, "SECS_AS_MINS");
    }
};


} // namespace tests
} // namespace models
} // namespace data
} // namespace ra
