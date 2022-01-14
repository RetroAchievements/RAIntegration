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

    TEST_METHOD(TestDeserialize)
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

    TEST_METHOD(TestSetStartTrigger)
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
        leaderboard.Activate();
        Assert::AreEqual(AssetChanges::None, leaderboard.GetChanges());

        // modify
        leaderboard.SetStartTrigger("0xH5555=55");
        Assert::AreEqual(AssetChanges::Modified, leaderboard.GetChanges());

        // ensure runtime is updated
        const auto* pLeaderboard = leaderboard.mockRuntime.GetLeaderboardDefinition(1U);
        Assert::IsNotNull(pLeaderboard);
        Ensures(pLeaderboard != nullptr);
        Assert::AreEqual(0x5555U, pLeaderboard->start.requirement->conditions->operand1.value.memref->address);
    }

    TEST_METHOD(TestSetSubmitTrigger)
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
        leaderboard.Activate();
        Assert::AreEqual(AssetChanges::None, leaderboard.GetChanges());

        // modify
        leaderboard.SetSubmitTrigger("0xH5555=55");
        Assert::AreEqual(AssetChanges::Modified, leaderboard.GetChanges());

        // ensure runtime is updated
        const auto* pLeaderboard = leaderboard.mockRuntime.GetLeaderboardDefinition(1U);
        Assert::IsNotNull(pLeaderboard);
        Ensures(pLeaderboard != nullptr);
        Assert::AreEqual(0x5555U, pLeaderboard->submit.requirement->conditions->operand1.value.memref->address);
    }

    TEST_METHOD(TestSetCancelTrigger)
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
        leaderboard.Activate();
        Assert::AreEqual(AssetChanges::None, leaderboard.GetChanges());

        // modify
        leaderboard.SetCancelTrigger("0xH5555=55");
        Assert::AreEqual(AssetChanges::Modified, leaderboard.GetChanges());

        // ensure runtime is updated
        const auto* pLeaderboard = leaderboard.mockRuntime.GetLeaderboardDefinition(1U);
        Assert::IsNotNull(pLeaderboard);
        Ensures(pLeaderboard != nullptr);
        Assert::AreEqual(0x5555U, pLeaderboard->cancel.requirement->conditions->operand1.value.memref->address);
    }

    TEST_METHOD(TestSetValueDefinition)
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
        leaderboard.Activate();
        Assert::AreEqual(AssetChanges::None, leaderboard.GetChanges());

        // modify
        leaderboard.SetValueDefinition("M:0xH5555");
        Assert::AreEqual(AssetChanges::Modified, leaderboard.GetChanges());

        // ensure runtime is updated
        const auto* pLeaderboard = leaderboard.mockRuntime.GetLeaderboardDefinition(1U);
        Assert::IsNotNull(pLeaderboard);
        Ensures(pLeaderboard != nullptr);
        Assert::AreEqual(0x5555U, pLeaderboard->value.conditions->conditions->operand1.value.memref->address);
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
        Assert::AreEqual(std::wstring(L"Start: Final condition type expects another condition to follow"), leaderboard.GetValidationError());

        leaderboard.SetStartTrigger("0xH1234=1");
        Assert::IsFalse(leaderboard.Validate());
        Assert::AreEqual(std::wstring(L"Cancel: Final condition type expects another condition to follow"), leaderboard.GetValidationError());

        leaderboard.SetCancelTrigger("0xH1234=2");
        Assert::IsFalse(leaderboard.Validate());
        Assert::AreEqual(std::wstring(L"Submit: Final condition type expects another condition to follow"), leaderboard.GetValidationError());

        leaderboard.SetSubmitTrigger("0xH1234=3");
        Assert::IsTrue(leaderboard.Validate());
        Assert::AreEqual(std::wstring(), leaderboard.GetValidationError());
    }
};

} // namespace tests
} // namespace models
} // namespace data
} // namespace ra
