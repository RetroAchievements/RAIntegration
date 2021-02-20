#include "CppUnitTest.h"

#include "ui\viewmodels\AssetEditorViewModel.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\data\DataAsserts.hh"

#include "tests\mocks\MockAchievementRuntime.hh"
#include "tests\mocks\MockClock.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockGameContext.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using ra::data::models::AchievementModel;
using ra::data::models::AssetCategory;
using ra::data::models::AssetChanges;
using ra::data::models::AssetState;
using ra::data::models::AssetType;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(AssetEditorViewModel_Tests)
{
private:
    class AssetEditorViewModelHarness : public AssetEditorViewModel
    {
    public:
        AssetEditorViewModelHarness() noexcept
        {
            // DoFrame tests expect the dialog to be visible
            GSL_SUPPRESS_F6 SetIsVisible(true);
        }

        ~AssetEditorViewModelHarness()
        {
            // prevent exception if asset is destroyed before harness
            m_pAsset = nullptr;
        }

        AssetEditorViewModelHarness(const AssetEditorViewModelHarness&) noexcept = delete;
        AssetEditorViewModelHarness& operator=(const AssetEditorViewModelHarness&) noexcept = delete;
        AssetEditorViewModelHarness(AssetEditorViewModelHarness&&) noexcept = delete;
        AssetEditorViewModelHarness& operator=(AssetEditorViewModelHarness&&) noexcept = delete;

        ra::services::mocks::MockAchievementRuntime mockRuntime;
        ra::services::mocks::MockClock mockClock;
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::data::context::mocks::MockGameContext mockGameContext;

        const std::wstring& GetWaitingLabel() const
        {
            return GetValue(WaitingLabelProperty);
        }
    };


public:
    TEST_METHOD(TestInitialValues)
    {
        AssetEditorViewModelHarness editor;

        Assert::AreEqual(std::wstring(L"Asset Editor"), editor.GetWindowTitle());
        Assert::AreEqual(0U, editor.GetID());
        Assert::AreEqual(std::wstring(L"[No Asset Loaded]"), editor.GetName());
        Assert::AreEqual(std::wstring(L"Open an asset from the Assets List"), editor.GetDescription());
        Assert::AreEqual(AssetCategory::Core, editor.GetCategory());
        Assert::AreEqual(AssetState::Inactive, editor.GetState());
        Assert::AreEqual(0, editor.GetPoints());
        Assert::AreEqual(std::wstring(L"00000"), editor.GetBadge());
        Assert::IsFalse(editor.IsPauseOnReset());
        Assert::IsFalse(editor.IsPauseOnTrigger());
        Assert::IsFalse(editor.IsDecimalPreferred());
        Assert::IsFalse(editor.IsAssetLoaded());
    }

    TEST_METHOD(TestLoadAchievement)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        achievement.SetName(L"Test Achievement");
        achievement.SetID(1234U);
        achievement.SetState(AssetState::Active);
        achievement.SetDescription(L"Do something cool");
        achievement.SetCategory(AssetCategory::Unofficial);
        achievement.SetPoints(10);
        achievement.SetBadge(L"58329");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();

        editor.LoadAsset(&achievement);

        Assert::AreEqual(std::wstring(L"Achievement Editor"), editor.GetWindowTitle());
        Assert::AreEqual(1234U, editor.GetID());
        Assert::AreEqual(std::wstring(L"Test Achievement"), editor.GetName());
        Assert::AreEqual(std::wstring(L"Do something cool"), editor.GetDescription());
        Assert::AreEqual(AssetCategory::Unofficial, editor.GetCategory());
        Assert::AreEqual(AssetState::Active, editor.GetState());
        Assert::AreEqual(10, editor.GetPoints());
        Assert::AreEqual(std::wstring(L"58329"), editor.GetBadge());
        Assert::IsFalse(editor.IsPauseOnReset());
        Assert::IsFalse(editor.IsPauseOnTrigger());
        Assert::IsTrue(editor.IsAssetLoaded());

        editor.LoadAsset(nullptr);

        Assert::AreEqual(std::wstring(L"Asset Editor"), editor.GetWindowTitle());
        Assert::AreEqual(0U, editor.GetID());
        Assert::AreEqual(std::wstring(L"[No Asset Loaded]"), editor.GetName());
        Assert::AreEqual(std::wstring(L"Open an asset from the Assets List"), editor.GetDescription());
        Assert::AreEqual(AssetCategory::Core, editor.GetCategory());
        Assert::AreEqual(AssetState::Inactive, editor.GetState());
        Assert::AreEqual(0, editor.GetPoints());
        Assert::AreEqual(std::wstring(L"00000"), editor.GetBadge());
        Assert::IsFalse(editor.IsPauseOnReset());
        Assert::IsFalse(editor.IsPauseOnTrigger());
        Assert::IsFalse(editor.IsAssetLoaded());
    }

    TEST_METHOD(TestLoadAchievementTrigger)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        achievement.SetTrigger("0xH1234=6.1.");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();

        editor.LoadAsset(&achievement);

        Assert::AreEqual(std::string("0xH1234=6.1."), editor.Trigger().Serialize());
        Assert::IsFalse(achievement.IsModified());
    }

    TEST_METHOD(TestSyncId)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        achievement.SetID(1234U);
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();

        editor.LoadAsset(&achievement);
        Assert::AreEqual(1234U, editor.GetID());
        Assert::AreEqual(1234U, achievement.GetID());

        achievement.SetID(2345U);
        Assert::AreEqual(2345U, editor.GetID());
        Assert::AreEqual(2345U, achievement.GetID());
    }

    TEST_METHOD(TestLocalIdIgnored)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        achievement.SetCategory(AssetCategory::Local);
        achievement.SetID(11100002U);
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();

        editor.LoadAsset(&achievement);
        Assert::AreEqual(0U, editor.GetID());
        Assert::AreEqual(11100002U, achievement.GetID());

        achievement.SetID(2345U);
        Assert::AreEqual(2345U, editor.GetID());
        Assert::AreEqual(2345U, achievement.GetID());
    }

    TEST_METHOD(TestSyncName)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        achievement.SetName(L"Test Achievement");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();

        editor.LoadAsset(&achievement);
        Assert::AreEqual(std::wstring(L"Test Achievement"), editor.GetName());
        Assert::AreEqual(std::wstring(L"Test Achievement"), achievement.GetName());

        editor.SetName(L"New Name");
        Assert::AreEqual(std::wstring(L"New Name"), editor.GetName());
        Assert::AreEqual(std::wstring(L"New Name"), achievement.GetName());

        editor.SetName(L"New Name 2");
        Assert::AreEqual(std::wstring(L"New Name 2"), editor.GetName());
        Assert::AreEqual(std::wstring(L"New Name 2"), achievement.GetName());

        achievement.SetName(L"New Name 3");
        Assert::AreEqual(std::wstring(L"New Name 3"), editor.GetName());
        Assert::AreEqual(std::wstring(L"New Name 3"), achievement.GetName());
    }

    TEST_METHOD(TestSyncDescription)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        achievement.SetDescription(L"Do something cool");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();

        editor.LoadAsset(&achievement);
        Assert::AreEqual(std::wstring(L"Do something cool"), editor.GetDescription());
        Assert::AreEqual(std::wstring(L"Do something cool"), achievement.GetDescription());

        editor.SetDescription(L"New description");
        Assert::AreEqual(std::wstring(L"New description"), editor.GetDescription());
        Assert::AreEqual(std::wstring(L"New description"), achievement.GetDescription());

        editor.SetDescription(L"New description 2");
        Assert::AreEqual(std::wstring(L"New description 2"), editor.GetDescription());
        Assert::AreEqual(std::wstring(L"New description 2"), achievement.GetDescription());

        achievement.SetDescription(L"New description 3");
        Assert::AreEqual(std::wstring(L"New description 3"), editor.GetDescription());
        Assert::AreEqual(std::wstring(L"New description 3"), achievement.GetDescription());
    }

    TEST_METHOD(TestSyncState)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        achievement.SetState(AssetState::Active);
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();

        editor.LoadAsset(&achievement);
        Assert::AreEqual(AssetState::Active, editor.GetState());
        Assert::AreEqual(AssetState::Active, achievement.GetState());
        Assert::AreEqual(std::wstring(L"Active"), editor.GetWaitingLabel());

        editor.SetState(AssetState::Paused);
        Assert::AreEqual(AssetState::Paused, editor.GetState());
        Assert::AreEqual(AssetState::Paused, achievement.GetState());
        Assert::AreEqual(std::wstring(L"Active"), editor.GetWaitingLabel());

        editor.SetState(AssetState::Triggered);
        Assert::AreEqual(AssetState::Triggered, editor.GetState());
        Assert::AreEqual(AssetState::Triggered, achievement.GetState());
        Assert::AreEqual(std::wstring(L"Active"), editor.GetWaitingLabel());

        achievement.SetState(AssetState::Waiting);
        Assert::AreEqual(AssetState::Waiting, editor.GetState());
        Assert::AreEqual(AssetState::Waiting, achievement.GetState());
        Assert::AreEqual(std::wstring(L"Waiting"), editor.GetWaitingLabel());

        achievement.SetState(AssetState::Active);
        Assert::AreEqual(AssetState::Active, editor.GetState());
        Assert::AreEqual(AssetState::Active, achievement.GetState());
        Assert::AreEqual(std::wstring(L"Active"), editor.GetWaitingLabel());
    }

    TEST_METHOD(TestSyncCategory)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        achievement.SetCategory(AssetCategory::Unofficial);
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();

        editor.LoadAsset(&achievement);
        Assert::AreEqual(AssetCategory::Unofficial, editor.GetCategory());
        Assert::AreEqual(AssetCategory::Unofficial, achievement.GetCategory());

        editor.SetCategory(AssetCategory::Core);
        Assert::AreEqual(AssetCategory::Core, editor.GetCategory());
        Assert::AreEqual(AssetCategory::Core, achievement.GetCategory());

        editor.SetCategory(AssetCategory::Unofficial);
        Assert::AreEqual(AssetCategory::Unofficial, editor.GetCategory());
        Assert::AreEqual(AssetCategory::Unofficial, achievement.GetCategory());

        achievement.SetCategory(AssetCategory::Core);
        Assert::AreEqual(AssetCategory::Core, editor.GetCategory());
        Assert::AreEqual(AssetCategory::Core, achievement.GetCategory());
    }

    TEST_METHOD(TestSyncPoints)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        achievement.SetPoints(10);
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();

        editor.LoadAsset(&achievement);
        Assert::AreEqual(10, editor.GetPoints());
        Assert::AreEqual(10, achievement.GetPoints());

        editor.SetPoints(25);
        Assert::AreEqual(25, editor.GetPoints());
        Assert::AreEqual(25, achievement.GetPoints());

        editor.SetPoints(5);
        Assert::AreEqual(5, editor.GetPoints());
        Assert::AreEqual(5, achievement.GetPoints());

        editor.SetPoints(50);
        Assert::AreEqual(50, editor.GetPoints());
        Assert::AreEqual(50, achievement.GetPoints());
    }

    TEST_METHOD(TestSyncBadge)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        achievement.SetBadge(L"58329");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();

        editor.LoadAsset(&achievement);
        Assert::AreEqual(std::wstring(L"58329"), editor.GetBadge());
        Assert::AreEqual(std::wstring(L"58329"), achievement.GetBadge());

        editor.SetBadge(L"12345");
        Assert::AreEqual(std::wstring(L"12345"), editor.GetBadge());
        Assert::AreEqual(std::wstring(L"12345"), achievement.GetBadge());

        editor.SetBadge(L"34567");
        Assert::AreEqual(std::wstring(L"34567"), editor.GetBadge());
        Assert::AreEqual(std::wstring(L"34567"), achievement.GetBadge());

        achievement.SetBadge(L"67890");
        Assert::AreEqual(std::wstring(L"67890"), editor.GetBadge());
        Assert::AreEqual(std::wstring(L"67890"), achievement.GetBadge());
    }

    TEST_METHOD(TestSyncPauseOnReset)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        achievement.SetPauseOnReset(true);
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();

        editor.LoadAsset(&achievement);
        Assert::IsTrue(editor.IsPauseOnReset());
        Assert::IsTrue(achievement.IsPauseOnReset());

        editor.SetPauseOnReset(false);
        Assert::IsFalse(editor.IsPauseOnReset());
        Assert::IsFalse(achievement.IsPauseOnReset());

        editor.SetPauseOnReset(true);
        Assert::IsTrue(editor.IsPauseOnReset());
        Assert::IsTrue(achievement.IsPauseOnReset());

        editor.SetPauseOnReset(false);
        Assert::IsFalse(editor.IsPauseOnReset());
        Assert::IsFalse(achievement.IsPauseOnReset());
    }

    TEST_METHOD(TestSyncPauseOnTrigger)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        achievement.SetPauseOnTrigger(true);
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();

        editor.LoadAsset(&achievement);
        Assert::IsTrue(editor.IsPauseOnTrigger());
        Assert::IsTrue(achievement.IsPauseOnTrigger());

        editor.SetPauseOnTrigger(false);
        Assert::IsFalse(editor.IsPauseOnTrigger());
        Assert::IsFalse(achievement.IsPauseOnTrigger());

        editor.SetPauseOnTrigger(true);
        Assert::IsTrue(editor.IsPauseOnTrigger());
        Assert::IsTrue(achievement.IsPauseOnTrigger());

        editor.SetPauseOnTrigger(false);
        Assert::IsFalse(editor.IsPauseOnTrigger());
        Assert::IsFalse(achievement.IsPauseOnTrigger());
    }

    TEST_METHOD(TestSyncOneRecordAtATime)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement1;
        achievement1.SetName(L"Achievement1");
        achievement1.CreateServerCheckpoint();
        achievement1.CreateLocalCheckpoint();
        AchievementModel achievement2;
        achievement2.SetName(L"Achievement2");
        achievement2.CreateServerCheckpoint();
        achievement2.CreateLocalCheckpoint();

        editor.LoadAsset(&achievement1);
        Assert::AreEqual(std::wstring(L"Achievement1"), editor.GetName());
        Assert::AreEqual(std::wstring(L"Achievement1"), achievement1.GetName());
        Assert::AreEqual(std::wstring(L"Achievement2"), achievement2.GetName());

        editor.SetName(L"New Name");
        Assert::AreEqual(std::wstring(L"New Name"), editor.GetName());
        Assert::AreEqual(std::wstring(L"New Name"), achievement1.GetName());
        Assert::AreEqual(std::wstring(L"Achievement2"), achievement2.GetName());

        editor.LoadAsset(&achievement2);
        Assert::AreEqual(std::wstring(L"Achievement2"), editor.GetName());
        Assert::AreEqual(std::wstring(L"New Name"), achievement1.GetName());
        Assert::AreEqual(std::wstring(L"Achievement2"), achievement2.GetName());

        editor.SetName(L"New Name 2");
        Assert::AreEqual(std::wstring(L"New Name 2"), editor.GetName());
        Assert::AreEqual(std::wstring(L"New Name"), achievement1.GetName());
        Assert::AreEqual(std::wstring(L"New Name 2"), achievement2.GetName());

        achievement1.SetName(L"New Name 3");
        Assert::AreEqual(std::wstring(L"New Name 2"), editor.GetName());
        Assert::AreEqual(std::wstring(L"New Name 3"), achievement1.GetName());
        Assert::AreEqual(std::wstring(L"New Name 2"), achievement2.GetName());
    }

    TEST_METHOD(TestUpdateActiveTrigger)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        achievement.SetID(1234U);
        achievement.SetTrigger("0xH1234=1");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();
        achievement.Activate();

        editor.LoadAsset(&achievement);

        // make sure the record got loaded into the runtime
        const auto* pTrigger = editor.mockRuntime.GetAchievementTrigger(1234U);
        Expects(pTrigger != nullptr);
        Assert::AreEqual(1U, pTrigger->requirement->conditions->operand2.value.num);

        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        auto* pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(MemSize::EightBit, pCondition->GetSourceSize());
        Assert::AreEqual((int)TriggerOperandType::Address, (int)pCondition->GetSourceType());
        Assert::AreEqual(0x1234U, pCondition->GetSourceValue());
        Assert::AreEqual((int)TriggerOperatorType::Equals, (int)pCondition->GetOperator());
        Assert::AreEqual(MemSize::ThirtyTwoBit, pCondition->GetTargetSize());
        Assert::AreEqual((int)TriggerOperandType::Value, (int)pCondition->GetTargetType());
        Assert::AreEqual(1U, pCondition->GetTargetValue());

        pCondition->SetTargetValue(2U);

        // make sure the trigger definition got updated
        Assert::AreEqual(std::string("0xH1234=2"), achievement.GetTrigger());

        // make sure the runtime record got updated
        pTrigger = editor.mockRuntime.GetAchievementTrigger(1234U);
        Expects(pTrigger != nullptr);
        Assert::AreEqual(2U, pTrigger->requirement->conditions->operand2.value.num);

        // make sure the UI kept the updated value
        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(MemSize::EightBit, pCondition->GetSourceSize());
        Assert::AreEqual((int)TriggerOperandType::Address, (int)pCondition->GetSourceType());
        Assert::AreEqual(0x1234U, pCondition->GetSourceValue());
        Assert::AreEqual((int)TriggerOperatorType::Equals, (int)pCondition->GetOperator());
        Assert::AreEqual(MemSize::ThirtyTwoBit, pCondition->GetTargetSize());
        Assert::AreEqual((int)TriggerOperandType::Value, (int)pCondition->GetTargetType());
        Assert::AreEqual(2U, pCondition->GetTargetValue());
    }

    TEST_METHOD(TestUpdateInactiveTrigger)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        achievement.SetID(1234U);
        achievement.SetTrigger("0xH1234=1");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();
        achievement.Deactivate();

        editor.LoadAsset(&achievement);

        // make sure the record did not get loaded into the runtime
        const auto* pTrigger = editor.mockRuntime.GetAchievementTrigger(1234U);
        Assert::IsNull(pTrigger);

        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        auto* pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(MemSize::EightBit, pCondition->GetSourceSize());
        Assert::AreEqual((int)TriggerOperandType::Address, (int)pCondition->GetSourceType());
        Assert::AreEqual(0x1234U, pCondition->GetSourceValue());
        Assert::AreEqual((int)TriggerOperatorType::Equals, (int)pCondition->GetOperator());
        Assert::AreEqual(MemSize::ThirtyTwoBit, pCondition->GetTargetSize());
        Assert::AreEqual((int)TriggerOperandType::Value, (int)pCondition->GetTargetType());
        Assert::AreEqual(1U, pCondition->GetTargetValue());

        pCondition->SetTargetValue(2U);

        // make sure the trigger definition got updated
        Assert::AreEqual(std::string("0xH1234=2"), achievement.GetTrigger());

        // make sure the record did not get loaded into the runtime
        pTrigger = editor.mockRuntime.GetAchievementTrigger(1234U);
        Assert::IsNull(pTrigger);

        // make sure the UI kept the updated value
        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(MemSize::EightBit, pCondition->GetSourceSize());
        Assert::AreEqual((int)TriggerOperandType::Address, (int)pCondition->GetSourceType());
        Assert::AreEqual(0x1234U, pCondition->GetSourceValue());
        Assert::AreEqual((int)TriggerOperatorType::Equals, (int)pCondition->GetOperator());
        Assert::AreEqual(MemSize::ThirtyTwoBit, pCondition->GetTargetSize());
        Assert::AreEqual((int)TriggerOperandType::Value, (int)pCondition->GetTargetType());
        Assert::AreEqual(2U, pCondition->GetTargetValue());
    }

    TEST_METHOD(TestTriggerUpdated)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        achievement.SetID(1234U);
        achievement.SetTrigger("0xH1234=1");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();
        achievement.Activate();

        editor.LoadAsset(&achievement);

        // make sure the record got loaded into the runtime
        const auto* pTrigger = editor.mockRuntime.GetAchievementTrigger(1234U);
        Expects(pTrigger != nullptr);
        Assert::AreEqual(1U, pTrigger->requirement->conditions->operand2.value.num);

        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        auto* pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(MemSize::EightBit, pCondition->GetSourceSize());
        Assert::AreEqual((int)TriggerOperandType::Address, (int)pCondition->GetSourceType());
        Assert::AreEqual(0x1234U, pCondition->GetSourceValue());
        Assert::AreEqual((int)TriggerOperatorType::Equals, (int)pCondition->GetOperator());
        Assert::AreEqual(MemSize::ThirtyTwoBit, pCondition->GetTargetSize());
        Assert::AreEqual((int)TriggerOperandType::Value, (int)pCondition->GetTargetType());
        Assert::AreEqual(1U, pCondition->GetTargetValue());

        achievement.SetTrigger("0xH1234=2");

        // make sure the trigger definition got updated
        Assert::AreEqual(std::string("0xH1234=2"), achievement.GetTrigger());

        // make sure the runtime record got updated
        pTrigger = editor.mockRuntime.GetAchievementTrigger(1234U);
        Expects(pTrigger != nullptr);
        Assert::AreEqual(2U, pTrigger->requirement->conditions->operand2.value.num);

        // make sure the UI picked up the updated value
        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(MemSize::EightBit, pCondition->GetSourceSize());
        Assert::AreEqual((int)TriggerOperandType::Address, (int)pCondition->GetSourceType());
        Assert::AreEqual(0x1234U, pCondition->GetSourceValue());
        Assert::AreEqual((int)TriggerOperatorType::Equals, (int)pCondition->GetOperator());
        Assert::AreEqual(MemSize::ThirtyTwoBit, pCondition->GetTargetSize());
        Assert::AreEqual((int)TriggerOperandType::Value, (int)pCondition->GetTargetType());
        Assert::AreEqual(2U, pCondition->GetTargetValue());
    }

    TEST_METHOD(TestDoFrameUpdatesHitsFromActiveAchievement)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        achievement.SetID(1234U);
        achievement.SetTrigger("0xH1234=1");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();
        achievement.Activate();

        editor.LoadAsset(&achievement);

        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        auto* pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(0U, pCondition->GetCurrentHits());

        const auto* pTrigger = editor.mockRuntime.GetAchievementTrigger(1234U);
        Expects(pTrigger != nullptr);
        pTrigger->requirement->conditions->current_hits = 6U;
        Assert::AreEqual(0U, pCondition->GetCurrentHits());

        editor.DoFrame();
        Assert::AreEqual(6U, pCondition->GetCurrentHits());
    }

    TEST_METHOD(TestDoFrameUpdatesHitsFromActiveAchievementWhenHidden)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        achievement.SetID(1234U);
        achievement.SetTrigger("0xH1234=1");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();
        achievement.Activate();

        editor.SetIsVisible(false);
        editor.LoadAsset(&achievement);

        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        auto* pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(0U, pCondition->GetCurrentHits());

        const auto* pTrigger = editor.mockRuntime.GetAchievementTrigger(1234U);
        Expects(pTrigger != nullptr);
        pTrigger->requirement->conditions->current_hits = 6U;
        Assert::AreEqual(0U, pCondition->GetCurrentHits());

        editor.DoFrame();
        Assert::AreEqual(0U, pCondition->GetCurrentHits());

        editor.SetIsVisible(true);
        Assert::AreEqual(6U, pCondition->GetCurrentHits());

        pTrigger->requirement->conditions->current_hits = 7U;
        editor.DoFrame();
        Assert::AreEqual(7U, pCondition->GetCurrentHits());
    }

    TEST_METHOD(TestDecimalPreferredOnVisible)
    {
        AssetEditorViewModelHarness editor;
        Assert::IsFalse(editor.IsDecimalPreferred());

        editor.SetIsVisible(true);
        Assert::IsFalse(editor.IsDecimalPreferred());

        editor.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
        Assert::IsFalse(editor.IsDecimalPreferred());

        editor.SetIsVisible(false);
        Assert::IsFalse(editor.IsDecimalPreferred());

        editor.SetIsVisible(true);
        Assert::IsTrue(editor.IsDecimalPreferred());

        editor.SetIsVisible(false);
        Assert::IsTrue(editor.IsDecimalPreferred());
    }

    TEST_METHOD(TestDecimalPreferredUpdatesConfiguration)
    {
        AssetEditorViewModelHarness editor;
        Assert::IsFalse(editor.IsDecimalPreferred());
        Assert::IsFalse(editor.mockConfiguration.IsFeatureEnabled(ra::services::Feature::PreferDecimal));

        editor.SetDecimalPreferred(true);
        Assert::IsTrue(editor.IsDecimalPreferred());
        Assert::IsTrue(editor.mockConfiguration.IsFeatureEnabled(ra::services::Feature::PreferDecimal));

        editor.SetDecimalPreferred(false);
        Assert::IsFalse(editor.IsDecimalPreferred());
        Assert::IsFalse(editor.mockConfiguration.IsFeatureEnabled(ra::services::Feature::PreferDecimal));
    }

    TEST_METHOD(TestCaptureRestoreHits)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        achievement.SetID(1234U);
        achievement.SetTrigger("0xH1234=1");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();
        achievement.Activate();

        const auto* pTrigger = editor.mockRuntime.GetAchievementTrigger(1234U);
        Expects(pTrigger != nullptr);
        pTrigger->requirement->conditions->current_hits = 6U;

        editor.LoadAsset(&achievement);

        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        auto* pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(6U, pCondition->GetCurrentHits());

        pTrigger->requirement->conditions->current_hits = 7U;
        editor.DoFrame();
        Assert::AreEqual(7U, pCondition->GetCurrentHits());

        editor.LoadAsset(nullptr);
        Assert::AreEqual({ 0U }, editor.Trigger().Conditions().Count());

        editor.LoadAsset(&achievement);
        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(7U, pCondition->GetCurrentHits());
    }

    TEST_METHOD(TestCaptureRestoreHitsTriggered)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        achievement.SetID(1234U);
        achievement.SetTrigger("0xH1234=1");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();
        achievement.Activate();
        Assert::AreEqual(ra::data::models::AssetState::Waiting, achievement.GetState());

        auto* pTrigger = editor.mockRuntime.GetAchievementTrigger(1234U);
        Expects(pTrigger != nullptr);
        pTrigger->requirement->conditions->current_hits = 6U;
        pTrigger->has_hits = 1;

        editor.LoadAsset(&achievement);

        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        auto* pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(6U, pCondition->GetCurrentHits());

        pTrigger->requirement->conditions->current_hits = 7U;
        pTrigger->state = RC_TRIGGER_STATE_TRIGGERED;
        achievement.SetState(ra::data::models::AssetState::Triggered);
        // triggered achievement is removed from runtime
        Assert::IsNull(editor.mockRuntime.GetAchievementTrigger(1234U));

        // but the updated state should still be reflected in the UI
        Assert::AreEqual(7U, pCondition->GetCurrentHits());

        editor.LoadAsset(nullptr);
        Assert::AreEqual({ 0U }, editor.Trigger().Conditions().Count());

        editor.LoadAsset(&achievement);
        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        // when reloaded, the updated state should be remembered
        Assert::AreEqual(7U, pCondition->GetCurrentHits());
    }

    TEST_METHOD(TestCaptureRestoreHitsTriggeredChanged)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        achievement.SetID(1234U);
        achievement.SetTrigger("0xH1234=1");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();
        achievement.Activate();
        Assert::AreEqual(ra::data::models::AssetState::Waiting, achievement.GetState());

        auto* pTrigger = editor.mockRuntime.GetAchievementTrigger(1234U);
        Expects(pTrigger != nullptr);
        pTrigger->requirement->conditions->current_hits = 6U;
        pTrigger->has_hits = 1;

        editor.LoadAsset(&achievement);

        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        auto* pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(6U, pCondition->GetCurrentHits());

        pTrigger->requirement->conditions->current_hits = 7U;
        pTrigger->state = RC_TRIGGER_STATE_TRIGGERED;
        achievement.SetState(ra::data::models::AssetState::Triggered);
        // triggered achievement is removed from runtime
        Assert::IsNull(editor.mockRuntime.GetAchievementTrigger(1234U));

        // but the updated state should still be reflected in the UI
        Assert::AreEqual(7U, pCondition->GetCurrentHits());

        editor.LoadAsset(nullptr);
        Assert::AreEqual({ 0U }, editor.Trigger().Conditions().Count());

        achievement.SetTrigger("0xH1234=2");
        editor.LoadAsset(&achievement);
        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        // definition changed. when reloaded, the updated state should be discarded
        Assert::AreEqual(0U, pCondition->GetCurrentHits());
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
