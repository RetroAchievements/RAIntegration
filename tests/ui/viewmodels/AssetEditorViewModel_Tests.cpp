#include "CppUnitTest.h"

#include "ui\EditorTheme.hh"

#include "ui\BindingBase.hh"
#include "ui\viewmodels\AssetEditorViewModel.hh"
#include "ui\viewmodels\FileDialogViewModel.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\data\DataAsserts.hh"
#include "tests\ui\viewmodels\TriggerConditionAsserts.hh"

#include "tests\devkit\context\mocks\MockEmulatorMemoryContext.hh"
#include "tests\devkit\context\mocks\MockRcClient.hh"
#include "tests\devkit\services\mocks\MockClock.hh"
#include "tests\devkit\services\mocks\MockFileSystem.hh"
#include "tests\devkit\services\mocks\MockThreadPool.hh"
#include "tests\devkit\testutil\AssetAsserts.hh"
#include "tests\devkit\testutil\MemoryAsserts.hh"
#include "tests\mocks\MockAchievementRuntime.hh"
#include "tests\mocks\MockClipboard.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockImageRepository.hh"
#include "tests\mocks\MockOverlayManager.hh"
#include "tests\mocks\MockWindowManager.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using ra::data::models::AchievementModel;
using ra::data::models::AssetCategory;
using ra::data::models::AssetChanges;
using ra::data::models::AssetState;
using ra::data::models::AssetType;
using ra::data::models::LeaderboardModel;

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
            : m_pMockThemeOverride(&mockTheme, false)
        {
            // DoFrame tests expect the dialog to be visible
            GSL_SUPPRESS_F6 SetIsVisible(true);

            GSL_SUPPRESS_F6 mockRuntime.MockGame();
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

        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        ra::context::mocks::MockRcClient mockRcClient;
        ra::services::mocks::MockAchievementRuntime mockRuntime;
        ra::services::mocks::MockClock mockClock;
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::services::mocks::MockFileSystem mockFileSystem;
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::ui::mocks::MockDesktop mockDesktop;
        ra::ui::mocks::MockImageRepository mockImageRepository;
        ra::ui::viewmodels::mocks::MockOverlayManager mockOverlayManager;
        ra::ui::EditorTheme mockTheme;

        const std::wstring& GetWaitingLabel() const
        {
            return GetValue(WaitingLabelProperty);
        }

        const std::wstring& GetGroupsHeaderLabel() const
        {
            return GetValue(GroupsHeaderProperty);
        }

        void SetAssetValidationError(const std::wstring& sValue)
        {
            SetValue(AssetValidationErrorProperty, sValue);
        }

        int GetTriggerVersion()
        {
            TriggerVersionBinding binding(Trigger());
            return binding.GetVersion();
        }

    private:
        ra::services::ServiceLocator::ServiceOverride<ra::ui::EditorTheme> m_pMockThemeOverride;

        class TriggerVersionBinding : public ui::BindingBase
        {
        public:
            TriggerVersionBinding(TriggerViewModel& vmTrigger) noexcept : ui::BindingBase(vmTrigger)
            {
            }

            int GetVersion() const { return GetValue(TriggerViewModel::VersionProperty); }
        };
    };


public:
    TEST_METHOD(TestInitialValues)
    {
        AssetEditorViewModelHarness editor;

        Assert::AreEqual(std::wstring(L"Achievement Editor"), editor.GetWindowTitle());
        Assert::AreEqual(0U, editor.GetID());
        Assert::AreEqual(std::wstring(L"[No Achievement Loaded]"), editor.GetName());
        Assert::AreEqual(std::wstring(L"Open an achievement from the Achievements List"), editor.GetDescription());
        Assert::AreEqual(AssetCategory::Core, editor.GetCategory());
        Assert::AreEqual(AssetState::Inactive, editor.GetState());
        Assert::AreEqual(0, editor.GetPoints());
        Assert::AreEqual(std::wstring(L"00000"), editor.GetBadge());
        Assert::IsFalse(editor.IsPauseOnReset());
        Assert::IsFalse(editor.IsPauseOnTrigger());
        Assert::IsFalse(editor.IsDecimalPreferred());
        Assert::IsFalse(editor.IsAssetLoaded());
        Assert::IsFalse(editor.HasAssetValidationError());
        Assert::AreEqual(std::wstring(), editor.GetAssetValidationError());
        Assert::IsFalse(editor.HasMeasured());
        Assert::AreEqual(std::wstring(L"[Not Active]"), editor.GetMeasuredValue());
        Assert::IsTrue(editor.IsAchievement());
        Assert::IsFalse(editor.IsLeaderboard());
        Assert::IsFalse(editor.IsTrigger());
        Assert::AreEqual((int)AssetEditorViewModel::LeaderboardPart::Start, (int)editor.GetSelectedLeaderboardPart());
        Assert::AreEqual(ra::data::ValueFormat::Value, editor.GetValueFormat());

        Assert::AreEqual({ 4U }, editor.AchievementTypes().Count());
        Assert::AreEqual((int)ra::data::models::AchievementType::None, editor.AchievementTypes().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L""), editor.AchievementTypes().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)ra::data::models::AchievementType::Missable, editor.AchievementTypes().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"Missable"), editor.AchievementTypes().GetItemAt(1)->GetLabel());
        Assert::AreEqual((int)ra::data::models::AchievementType::Progression, editor.AchievementTypes().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"Progression"), editor.AchievementTypes().GetItemAt(2)->GetLabel());
        Assert::AreEqual((int)ra::data::models::AchievementType::Win, editor.AchievementTypes().GetItemAt(3)->GetId());
        Assert::AreEqual(std::wstring(L"Win"), editor.AchievementTypes().GetItemAt(3)->GetLabel());

        Assert::AreEqual({ 14U }, editor.Formats().Count());
        Assert::AreEqual((int)ra::data::ValueFormat::Score, editor.Formats().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"Score"), editor.Formats().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)ra::data::ValueFormat::Frames, editor.Formats().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"Time (Frames)"), editor.Formats().GetItemAt(1)->GetLabel());
        Assert::AreEqual((int)ra::data::ValueFormat::Centiseconds, editor.Formats().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"Time (Centiseconds)"), editor.Formats().GetItemAt(2)->GetLabel());
        Assert::AreEqual((int)ra::data::ValueFormat::Seconds, editor.Formats().GetItemAt(3)->GetId());
        Assert::AreEqual(std::wstring(L"Time (Seconds)"), editor.Formats().GetItemAt(3)->GetLabel());
        Assert::AreEqual((int)ra::data::ValueFormat::Minutes, editor.Formats().GetItemAt(4)->GetId());
        Assert::AreEqual(std::wstring(L"Time (Minutes)"), editor.Formats().GetItemAt(4)->GetLabel());
        Assert::AreEqual((int)ra::data::ValueFormat::SecondsAsMinutes, editor.Formats().GetItemAt(5)->GetId());
        Assert::AreEqual(std::wstring(L"Time (Seconds as Minutes)"), editor.Formats().GetItemAt(5)->GetLabel());
        Assert::AreEqual((int)ra::data::ValueFormat::Value, editor.Formats().GetItemAt(6)->GetId());
        Assert::AreEqual(std::wstring(L"Value"), editor.Formats().GetItemAt(6)->GetLabel());
        Assert::AreEqual((int)ra::data::ValueFormat::UnsignedValue, editor.Formats().GetItemAt(7)->GetId());
        Assert::AreEqual(std::wstring(L"Value (Unsigned)"), editor.Formats().GetItemAt(7)->GetLabel());
        Assert::AreEqual((int)ra::data::ValueFormat::Tens, editor.Formats().GetItemAt(8)->GetId());
        Assert::AreEqual(std::wstring(L"Value (Tens)"), editor.Formats().GetItemAt(8)->GetLabel());
        Assert::AreEqual((int)ra::data::ValueFormat::Hundreds, editor.Formats().GetItemAt(9)->GetId());
        Assert::AreEqual(std::wstring(L"Value (Hundreds)"), editor.Formats().GetItemAt(9)->GetLabel());
        Assert::AreEqual((int)ra::data::ValueFormat::Thousands, editor.Formats().GetItemAt(10)->GetId());
        Assert::AreEqual(std::wstring(L"Value (Thousands)"), editor.Formats().GetItemAt(10)->GetLabel());
        Assert::AreEqual((int)ra::data::ValueFormat::Fixed1, editor.Formats().GetItemAt(11)->GetId());
        Assert::AreEqual(std::wstring(L"Value (Fixed1)"), editor.Formats().GetItemAt(11)->GetLabel());
        Assert::AreEqual((int)ra::data::ValueFormat::Fixed2, editor.Formats().GetItemAt(12)->GetId());
        Assert::AreEqual(std::wstring(L"Value (Fixed2)"), editor.Formats().GetItemAt(12)->GetLabel());
        Assert::AreEqual((int)ra::data::ValueFormat::Fixed3, editor.Formats().GetItemAt(13)->GetId());
        Assert::AreEqual(std::wstring(L"Value (Fixed3)"), editor.Formats().GetItemAt(13)->GetLabel());

        Assert::AreEqual({ 4U }, editor.LeaderboardParts().Count());
        Assert::AreEqual((int)AssetEditorViewModel::LeaderboardPart::Start, editor.LeaderboardParts().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"Start"), editor.LeaderboardParts().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)AssetEditorViewModel::LeaderboardPart::Cancel, editor.LeaderboardParts().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"Cancel"), editor.LeaderboardParts().GetItemAt(1)->GetLabel());
        Assert::AreEqual((int)AssetEditorViewModel::LeaderboardPart::Submit, editor.LeaderboardParts().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"Submit"), editor.LeaderboardParts().GetItemAt(2)->GetLabel());
        Assert::AreEqual((int)AssetEditorViewModel::LeaderboardPart::Value, editor.LeaderboardParts().GetItemAt(3)->GetId());
        Assert::AreEqual(std::wstring(L"Value"), editor.LeaderboardParts().GetItemAt(3)->GetLabel());
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
        Assert::IsTrue(editor.IsAchievement());
        Assert::IsFalse(editor.IsLeaderboard());
        Assert::IsTrue(editor.IsTrigger());
        Assert::IsFalse(editor.HasMeasured());
        Assert::IsFalse(editor.HasAssetValidationError());
        Assert::IsFalse(editor.HasAssetValidationWarning());
        Assert::AreEqual(std::wstring(L"Groups:"), editor.GetGroupsHeaderLabel());

        editor.LoadAsset(nullptr);

        Assert::AreEqual(std::wstring(L"Achievement Editor"), editor.GetWindowTitle());
        Assert::AreEqual(0U, editor.GetID());
        Assert::AreEqual(std::wstring(L"[No Achievement Loaded]"), editor.GetName());
        Assert::AreEqual(std::wstring(L"Open an achievement from the Achievements List"), editor.GetDescription());
        Assert::AreEqual(AssetCategory::Core, editor.GetCategory());
        Assert::AreEqual(AssetState::Inactive, editor.GetState());
        Assert::AreEqual(0, editor.GetPoints());
        Assert::AreEqual(std::wstring(L"00000"), editor.GetBadge());
        Assert::IsFalse(editor.IsPauseOnReset());
        Assert::IsFalse(editor.IsPauseOnTrigger());
        Assert::IsFalse(editor.IsAssetLoaded());
        Assert::IsTrue(editor.IsAchievement());
        Assert::IsFalse(editor.IsLeaderboard());
        Assert::IsFalse(editor.IsTrigger());
        Assert::IsFalse(editor.HasMeasured());
        Assert::IsFalse(editor.HasAssetValidationError());
        Assert::IsFalse(editor.HasAssetValidationWarning());
        Assert::AreEqual(std::wstring(L"Groups:"), editor.GetGroupsHeaderLabel());
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

    TEST_METHOD(TestLoadAchievementWhileInvalid)
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
        achievement.SetDescription(L"Do something modified");
        Assert::AreEqual(AssetChanges::Modified, achievement.GetChanges());

        editor.LoadAsset(&achievement);

        editor.SetAssetValidationError(L"I am error.");
        bool bMessageSeen = false;
        editor.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bMessageSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessage)
        {
            bMessageSeen = true;
            Assert::AreEqual(std::wstring(L"Discard changes?"), vmMessage.GetHeader());
            Assert::AreEqual(std::wstring(L"The currently loaded asset has an error that cannot be saved. If you switch to another asset, your changes will be lost."), vmMessage.GetMessage());

            return DialogResult::No;
        });
        Assert::IsTrue(editor.HasAssetValidationError());

        // attempt to reload current asset does not show warning
        editor.LoadAsset(&achievement);
        Assert::IsFalse(bMessageSeen);
        Assert::AreEqual(1234U, editor.GetID());

        AchievementModel achievement2;
        achievement2.SetName(L"Test Achievement 2");
        achievement2.SetID(2345U);
        achievement2.SetState(AssetState::Waiting);
        achievement2.SetDescription(L"Do something else");
        achievement2.SetCategory(AssetCategory::Unofficial);
        achievement2.SetPoints(5);
        achievement2.SetBadge(L"83295");
        achievement2.CreateServerCheckpoint();
        achievement2.CreateLocalCheckpoint();

        // attempt to load alternate asset shows warning (set up to abort)
        editor.LoadAsset(&achievement2);
        Assert::IsTrue(bMessageSeen);
        bMessageSeen = false;

        Assert::AreEqual(1234U, editor.GetID());
        Assert::AreEqual(std::wstring(L"Test Achievement"), editor.GetName());
        Assert::AreEqual(std::wstring(L"Do something modified"), editor.GetDescription());
        Assert::AreEqual(AssetCategory::Unofficial, editor.GetCategory());
        Assert::AreEqual(AssetState::Active, editor.GetState());
        Assert::AreEqual(10, editor.GetPoints());
        Assert::AreEqual(std::wstring(L"58329"), editor.GetBadge());
        Assert::IsTrue(editor.IsAssetLoaded());
        Assert::IsTrue(editor.HasAssetValidationError());
        Assert::AreEqual(AssetChanges::Modified, achievement.GetChanges());

        // attempt to load no asset shows warning (set up to abort)
        editor.LoadAsset(nullptr);
        Assert::IsTrue(bMessageSeen);
        bMessageSeen = false;

        Assert::AreEqual(1234U, editor.GetID());
        Assert::AreEqual(std::wstring(L"Test Achievement"), editor.GetName());
        Assert::AreEqual(std::wstring(L"Do something modified"), editor.GetDescription());
        Assert::AreEqual(AssetCategory::Unofficial, editor.GetCategory());
        Assert::AreEqual(AssetState::Active, editor.GetState());
        Assert::AreEqual(10, editor.GetPoints());
        Assert::AreEqual(std::wstring(L"58329"), editor.GetBadge());
        Assert::IsTrue(editor.IsAssetLoaded());
        Assert::IsTrue(editor.HasAssetValidationError());
        Assert::AreEqual(AssetChanges::Modified, achievement.GetChanges());

        // change behavior to not abort
        editor.mockDesktop.ResetExpectedWindows();
        editor.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bMessageSeen](ra::ui::viewmodels::MessageBoxViewModel&)
        {
            bMessageSeen = true;
            return DialogResult::Yes;
        });

        editor.LoadAsset(&achievement2);
        Assert::IsTrue(bMessageSeen);

        Assert::AreEqual(2345U, editor.GetID());
        Assert::AreEqual(std::wstring(L"Test Achievement 2"), editor.GetName());
        Assert::AreEqual(std::wstring(L"Do something else"), editor.GetDescription());
        Assert::AreEqual(AssetCategory::Unofficial, editor.GetCategory());
        Assert::AreEqual(AssetState::Waiting, editor.GetState());
        Assert::AreEqual(5, editor.GetPoints());
        Assert::AreEqual(std::wstring(L"83295"), editor.GetBadge());
        Assert::IsTrue(editor.IsAssetLoaded());
        Assert::IsFalse(editor.HasAssetValidationError());
        Assert::AreEqual(std::wstring(L"Waiting"), editor.GetWaitingLabel());

        // changes should have been discarded
        Assert::AreEqual(AssetChanges::None, achievement.GetChanges());
        Assert::AreEqual(std::wstring(L"Do something cool"), achievement.GetDescription());
    }

    TEST_METHOD(TestLoadAchievementWaiting)
    {
        AssetEditorViewModelHarness editor;
        Assert::AreEqual(std::wstring(L"Active"), editor.GetWaitingLabel());

        AchievementModel achievement;
        achievement.SetName(L"Test Achievement");
        achievement.SetID(1234U);
        achievement.SetState(AssetState::Active);
        achievement.SetDescription(L"Do something cool");
        achievement.SetTrigger("M:0xH1234=6.11.");
        achievement.SetCategory(AssetCategory::Unofficial);
        achievement.SetPoints(10);
        achievement.SetBadge(L"58329");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();

        AchievementModel achievement2;
        achievement2.SetName(L"Test Achievement");
        achievement2.SetID(1234U);
        achievement2.SetState(AssetState::Waiting);
        achievement2.SetDescription(L"Do something cool");
        achievement2.SetTrigger("M:0xH1234=6.11.");
        achievement2.SetCategory(AssetCategory::Unofficial);
        achievement2.SetPoints(10);
        achievement2.SetBadge(L"58329");
        achievement2.CreateServerCheckpoint();
        achievement2.CreateLocalCheckpoint();

        editor.LoadAsset(&achievement);
        Assert::AreEqual(std::wstring(L"Active"), editor.GetWaitingLabel());

        editor.LoadAsset(nullptr);
        Assert::AreEqual(std::wstring(L"Active"), editor.GetWaitingLabel());

        editor.LoadAsset(&achievement2);
        Assert::AreEqual(std::wstring(L"Waiting"), editor.GetWaitingLabel());

        editor.LoadAsset(nullptr);
        Assert::AreEqual(std::wstring(L"Active"), editor.GetWaitingLabel());

        editor.LoadAsset(&achievement2);
        Assert::AreEqual(std::wstring(L"Waiting"), editor.GetWaitingLabel());

        editor.LoadAsset(&achievement);
        Assert::AreEqual(std::wstring(L"Active"), editor.GetWaitingLabel());
    }

    TEST_METHOD(TestLoadAchievementMeasured)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        achievement.SetName(L"Test Achievement");
        achievement.SetID(1234U);
        achievement.SetState(AssetState::Active);
        achievement.SetDescription(L"Do something cool");
        achievement.SetTrigger("M:0xH1234=6.11.");
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
        Assert::IsTrue(editor.HasMeasured());
        Assert::AreEqual(std::wstring(L"0/11"), editor.GetMeasuredValue());

        editor.LoadAsset(nullptr);

        Assert::AreEqual(std::wstring(L"Achievement Editor"), editor.GetWindowTitle());
        Assert::AreEqual(0U, editor.GetID());
        Assert::AreEqual(std::wstring(L"[No Achievement Loaded]"), editor.GetName());
        Assert::AreEqual(std::wstring(L"Open an achievement from the Achievements List"), editor.GetDescription());
        Assert::AreEqual(AssetCategory::Core, editor.GetCategory());
        Assert::AreEqual(AssetState::Inactive, editor.GetState());
        Assert::AreEqual(0, editor.GetPoints());
        Assert::AreEqual(std::wstring(L"00000"), editor.GetBadge());
        Assert::IsFalse(editor.IsPauseOnReset());
        Assert::IsFalse(editor.IsPauseOnTrigger());
        Assert::IsFalse(editor.IsAssetLoaded());
        Assert::IsFalse(editor.HasMeasured());
    }

    TEST_METHOD(TestLoadAchievementMeasuredFromRuntime)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        achievement.SetID(1234U);
        achievement.SetState(AssetState::Active);
        achievement.SetTrigger("M:0xH1234=6.11.");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();

        editor.mockRuntime.ActivateAchievement(achievement.GetID(), achievement.GetTrigger());

        editor.LoadAsset(&achievement);

        Assert::AreEqual(1234U, editor.GetID());
        Assert::AreEqual(AssetState::Active, editor.GetState());
        Assert::IsTrue(editor.HasMeasured());
        Assert::AreEqual(std::wstring(L"0/11"), editor.GetMeasuredValue());

        editor.LoadAsset(nullptr);

        Assert::AreEqual(0U, editor.GetID());
        Assert::IsFalse(editor.HasMeasured());
    }

    TEST_METHOD(TestLoadAchievementValidationWarning)
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
        achievement.SetTrigger("A:0xH1234"); // AddSource cannot be final condition
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();

        editor.LoadAsset(&achievement);

        Assert::IsFalse(editor.HasAssetValidationError());
        Assert::AreEqual(std::wstring(), editor.GetAssetValidationError());

        Assert::IsTrue(editor.HasAssetValidationWarning());
        Assert::AreEqual(std::wstring(L"Condition 1: AddSource condition type expects another condition to follow"), editor.GetAssetValidationWarning());

        editor.LoadAsset(nullptr);
        Assert::IsFalse(editor.mockDesktop.WasDialogShown());
        Assert::IsFalse(editor.HasAssetValidationError());
        Assert::AreEqual(std::wstring(), editor.GetAssetValidationError());
        Assert::IsFalse(editor.HasAssetValidationWarning());
        Assert::AreEqual(std::wstring(), editor.GetAssetValidationWarning());
    }

    TEST_METHOD(TestLoadAchievementValidationError)
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
        achievement.SetTrigger("M:0x1234=10");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();

        editor.LoadAsset(&achievement);
        editor.SetAssetValidationError(L"Multiple Measured"); // not really, just fake it

        Assert::IsTrue(editor.HasAssetValidationError());
        Assert::AreEqual(std::wstring(L"Multiple Measured"), editor.GetAssetValidationError());

        Assert::IsFalse(editor.HasAssetValidationWarning());
        Assert::AreEqual(std::wstring(), editor.GetAssetValidationWarning());

        editor.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Discard changes?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"The currently loaded asset has an error that cannot be saved. If you switch to another asset, your changes will be lost."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::Yes;
        });

        editor.LoadAsset(nullptr);
        Assert::IsTrue(editor.mockDesktop.WasDialogShown());
        Assert::IsFalse(editor.HasAssetValidationError());
        Assert::AreEqual(std::wstring(), editor.GetAssetValidationError());
        Assert::IsFalse(editor.HasAssetValidationWarning());
        Assert::AreEqual(std::wstring(), editor.GetAssetValidationWarning());
    }

    TEST_METHOD(TestLoadAchievementValidationErrorCancel)
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
        achievement.SetTrigger("M:0x1234=10");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();

        editor.LoadAsset(&achievement);
        editor.SetAssetValidationError(L"Multiple Measured"); // not really, just fake it

        Assert::IsTrue(editor.HasAssetValidationError());
        Assert::AreEqual(std::wstring(L"Multiple Measured"), editor.GetAssetValidationError());

        Assert::IsFalse(editor.HasAssetValidationWarning());
        Assert::AreEqual(std::wstring(), editor.GetAssetValidationWarning());

        editor.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Discard changes?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"The currently loaded asset has an error that cannot be saved. If you switch to another asset, your changes will be lost."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::No;
        });

        editor.LoadAsset(nullptr);
        Assert::IsTrue(editor.mockDesktop.WasDialogShown());

        Assert::IsTrue(editor.HasAssetValidationError());
        Assert::AreEqual(std::wstring(L"Multiple Measured"), editor.GetAssetValidationError());
        Assert::IsFalse(editor.HasAssetValidationWarning());
        Assert::AreEqual(std::wstring(), editor.GetAssetValidationWarning());
        Assert::IsTrue(editor.GetAsset() == &achievement);
    }

    TEST_METHOD(TestLoadAchievementValidationErrorDeleted)
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
        achievement.SetTrigger("M:0x1234=10");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();

        editor.LoadAsset(&achievement);
        editor.SetAssetValidationError(L"Multiple Measured"); // not really, just fake it

        Assert::IsTrue(editor.HasAssetValidationError());
        Assert::AreEqual(std::wstring(L"Multiple Measured"), editor.GetAssetValidationError());

        Assert::IsFalse(editor.HasAssetValidationWarning());
        Assert::AreEqual(std::wstring(), editor.GetAssetValidationWarning());

        achievement.SetDeleted();

        editor.LoadAsset(nullptr);
        Assert::IsFalse(editor.mockDesktop.WasDialogShown());
        Assert::IsFalse(editor.HasAssetValidationError());
        Assert::AreEqual(std::wstring(), editor.GetAssetValidationError());
        Assert::IsFalse(editor.HasAssetValidationWarning());
        Assert::AreEqual(std::wstring(), editor.GetAssetValidationWarning());
    }

    TEST_METHOD(TestLoadAchievementForcedValidationError)
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
        achievement.SetTrigger("M:0x1234=10");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();

        editor.LoadAsset(&achievement);
        editor.SetAssetValidationError(L"Multiple Measured"); // not really, just fake it

        Assert::IsTrue(editor.HasAssetValidationError());
        Assert::AreEqual(std::wstring(L"Multiple Measured"), editor.GetAssetValidationError());

        Assert::IsFalse(editor.HasAssetValidationWarning());
        Assert::AreEqual(std::wstring(), editor.GetAssetValidationWarning());

        editor.LoadAsset(nullptr, true);
        Assert::IsFalse(editor.mockDesktop.WasDialogShown());
        Assert::IsFalse(editor.HasAssetValidationError());
        Assert::AreEqual(std::wstring(), editor.GetAssetValidationError());
        Assert::IsFalse(editor.HasAssetValidationWarning());
        Assert::AreEqual(std::wstring(), editor.GetAssetValidationWarning());
    }

    TEST_METHOD(TestLoadLeaderboard)
    {
        AssetEditorViewModelHarness editor;
        LeaderboardModel leaderboard;
        leaderboard.SetName(L"Test Leaderboard");
        leaderboard.SetID(1234U);
        leaderboard.SetState(AssetState::Active);
        leaderboard.SetDescription(L"Do something cool");
        leaderboard.SetCategory(AssetCategory::Unofficial);
        leaderboard.SetStartTrigger("0xH1234=1");
        leaderboard.SetCancelTrigger("0xH1234=2");
        leaderboard.SetSubmitTrigger("0xH1234=3");
        leaderboard.SetValueDefinition("0xH2345");
        leaderboard.SetValueFormat(ra::data::ValueFormat::Centiseconds);
        leaderboard.SetLowerIsBetter(false);
        leaderboard.CreateServerCheckpoint();
        leaderboard.CreateLocalCheckpoint();

        editor.LoadAsset(&leaderboard);

        Assert::AreEqual(std::wstring(L"Leaderboard Editor"), editor.GetWindowTitle());
        Assert::AreEqual(1234U, editor.GetID());
        Assert::AreEqual(std::wstring(L"Test Leaderboard"), editor.GetName());
        Assert::AreEqual(std::wstring(L"Do something cool"), editor.GetDescription());
        Assert::AreEqual(AssetCategory::Unofficial, editor.GetCategory());
        Assert::AreEqual(AssetState::Active, editor.GetState());
        Assert::AreEqual(ra::data::ValueFormat::Centiseconds, editor.GetValueFormat());
        Assert::IsFalse(editor.IsLowerBetter());
        Assert::IsFalse(editor.IsPauseOnReset());
        Assert::IsFalse(editor.IsPauseOnTrigger());
        Assert::IsTrue(editor.IsAssetLoaded());
        Assert::IsFalse(editor.IsAchievement());
        Assert::IsTrue(editor.IsLeaderboard());
        Assert::IsTrue(editor.IsTrigger());
        Assert::IsTrue(editor.HasMeasured());
        Assert::IsFalse(editor.HasAssetValidationError());
        Assert::IsFalse(editor.HasAssetValidationWarning());
        Assert::AreEqual(std::wstring(L"Groups:"), editor.GetGroupsHeaderLabel());

        editor.LoadAsset(nullptr);

        Assert::AreEqual(std::wstring(L"Achievement Editor"), editor.GetWindowTitle());
        Assert::AreEqual(0U, editor.GetID());
        Assert::AreEqual(std::wstring(L"[No Achievement Loaded]"), editor.GetName());
        Assert::AreEqual(std::wstring(L"Open an achievement from the Achievements List"), editor.GetDescription());
        Assert::AreEqual(AssetCategory::Core, editor.GetCategory());
        Assert::AreEqual(AssetState::Inactive, editor.GetState());
        Assert::AreEqual(0, editor.GetPoints());
        Assert::AreEqual(std::wstring(L"00000"), editor.GetBadge());
        Assert::IsFalse(editor.IsPauseOnReset());
        Assert::IsFalse(editor.IsPauseOnTrigger());
        Assert::IsFalse(editor.IsAssetLoaded());
        Assert::IsFalse(editor.HasMeasured());
        Assert::IsFalse(editor.HasAssetValidationError());
        Assert::IsFalse(editor.HasAssetValidationWarning());
        Assert::AreEqual(std::wstring(L"Groups:"), editor.GetGroupsHeaderLabel());
    }

    TEST_METHOD(TestLoadLeaderboardTrigger)
    {
        AssetEditorViewModelHarness editor;
        LeaderboardModel leaderboard;
        leaderboard.SetStartTrigger("0xH1234=6.1.");
        leaderboard.CreateServerCheckpoint();
        leaderboard.CreateLocalCheckpoint();

        editor.LoadAsset(&leaderboard);

        Assert::AreEqual(std::string("0xH1234=6.1."), editor.Trigger().Serialize());
        Assert::IsFalse(leaderboard.IsModified());
    }

    TEST_METHOD(TestLoadNewLeaderboard)
    {
        AssetEditorViewModelHarness editor;
        LeaderboardModel leaderboard;
        leaderboard.SetID(1110000001U);
        leaderboard.CreateServerCheckpoint();
        leaderboard.SetState(AssetState::Inactive);
        leaderboard.SetCategory(AssetCategory::Local);
        leaderboard.SetNew();
        leaderboard.CreateLocalCheckpoint();

        editor.LoadAsset(&leaderboard);

        Assert::AreEqual(std::wstring(L"Leaderboard Editor"), editor.GetWindowTitle());
        Assert::AreEqual(0U, editor.GetID());
        Assert::AreEqual(std::wstring(L""), editor.GetName());
        Assert::AreEqual(std::wstring(L""), editor.GetDescription());
        Assert::AreEqual(AssetCategory::Local, editor.GetCategory());
        Assert::AreEqual(AssetState::Inactive, editor.GetState());
        Assert::AreEqual(ra::data::ValueFormat::Value, editor.GetValueFormat());
        Assert::IsFalse(editor.IsLowerBetter());
        Assert::IsFalse(editor.IsPauseOnReset());
        Assert::IsFalse(editor.IsPauseOnTrigger());
        Assert::IsTrue(editor.IsAssetLoaded());
        Assert::IsFalse(editor.IsAchievement());
        Assert::IsTrue(editor.IsLeaderboard());
        Assert::IsTrue(editor.IsTrigger());
        Assert::IsTrue(editor.HasMeasured());
        Assert::IsTrue(editor.HasAssetValidationError());
        Assert::AreEqual(std::wstring(L"Missing start condition"), editor.GetAssetValidationError());
        Assert::AreEqual(std::wstring(L"Groups:"), editor.GetGroupsHeaderLabel());

        ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager; // to copy address from memory viewer
        editor.Trigger().NewCondition();
        Assert::IsTrue(editor.HasAssetValidationError());
        Assert::AreEqual(std::wstring(L"Missing cancel condition"), editor.GetAssetValidationError());
    }

    TEST_METHOD(TestActivateNewLeaderboard)
    {
        AssetEditorViewModelHarness editor;
        ra::services::mocks::MockThreadPool mockThreadPool;

        editor.mockRuntime.MockLocalLeaderboard(111000001U, "Leaderboard");
        editor.mockGameContext.InitializeFromAchievementRuntime();
        auto* leaderboard = editor.mockGameContext.Assets().FindLeaderboard(111000001U);
        Expects(leaderboard != nullptr);

        editor.LoadAsset(leaderboard);

        Assert::IsTrue(editor.HasAssetValidationError());
        Assert::AreEqual(std::wstring(L"Missing start condition"), editor.GetAssetValidationError());

        // attempt to start incomplete leaderboard should fail
        bool bDialogSeen = false;
        editor.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](MessageBoxViewModel& vmMessageBox) {
            Assert::AreEqual(std::wstring(L"Activation failed."), vmMessageBox.GetMessage());
            bDialogSeen = true;
            return DialogResult::OK;
        });

        editor.SetState(AssetState::Waiting);
        Assert::IsTrue(bDialogSeen);

        // to work around a binding issue, changing the state back is done asynchronously.
        // advance time to ensure the state gets changed back.
        mockThreadPool.AdvanceTime(std::chrono::milliseconds(200));

        Assert::AreEqual(AssetState::Inactive, editor.GetState());

        // set additional properties so leaderboard is valid and attempt to activate again
        leaderboard->SetStartTrigger("0=1");
        leaderboard->SetCancelTrigger("0=1");
        leaderboard->SetSubmitTrigger("0=1");
        leaderboard->SetValueDefinition("0");
        leaderboard->Validate();
        Assert::IsFalse(editor.HasAssetValidationError());
        Assert::IsFalse(editor.HasAssetValidationWarning());

        bDialogSeen = false;
        editor.SetState(AssetState::Waiting);
        Assert::IsFalse(bDialogSeen);

        leaderboard->DoFrame();
        Assert::AreEqual(AssetState::Waiting, editor.GetState());
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

    TEST_METHOD(TestSyncValidationWarning)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        achievement.SetName(L"Test Achievement");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();

        editor.LoadAsset(&achievement);
        Assert::IsFalse(editor.HasAssetValidationWarning());
        Assert::AreEqual(std::wstring(L""), editor.GetAssetValidationWarning());

        achievement.SetTrigger("A:0x1234");
        Assert::IsFalse(editor.HasAssetValidationWarning());
        Assert::AreEqual(std::wstring(L""), editor.GetAssetValidationWarning());

        achievement.UpdateLocalCheckpoint();
        Assert::IsTrue(editor.HasAssetValidationWarning());
        Assert::AreEqual(std::wstring(L"Condition 1: AddSource condition type expects another condition to follow"), editor.GetAssetValidationWarning());
    }

    TEST_METHOD(TestSyncState)
    {
        AssetEditorViewModelHarness editor;
        editor.mockRuntime.ActivateAchievement(1234U, "0xH1234=1");
        editor.mockGameContext.InitializeFromAchievementRuntime();
        auto* vmAch = editor.mockGameContext.Assets().FindAchievement(1234U);
        editor.mockRuntime.SyncAssets();
        vmAch->SetState(AssetState::Active);

        editor.LoadAsset(vmAch);
        Assert::AreEqual(AssetState::Active, editor.GetState());
        Assert::AreEqual(AssetState::Active, vmAch->GetState());
        Assert::AreEqual(std::wstring(L"Active"), editor.GetWaitingLabel());

        editor.SetState(AssetState::Paused);
        Assert::AreEqual(AssetState::Paused, editor.GetState());
        Assert::AreEqual(AssetState::Paused, vmAch->GetState());
        Assert::AreEqual(std::wstring(L"Active"), editor.GetWaitingLabel());

        vmAch->SetState(AssetState::Primed);
        Assert::AreEqual(AssetState::Primed, editor.GetState());
        Assert::AreEqual(AssetState::Primed, vmAch->GetState());
        Assert::AreEqual(std::wstring(L"Active"), editor.GetWaitingLabel());

        editor.SetState(AssetState::Triggered);
        Assert::AreEqual(AssetState::Triggered, editor.GetState());
        Assert::AreEqual(AssetState::Triggered, vmAch->GetState());
        Assert::AreEqual(std::wstring(L"Active"), editor.GetWaitingLabel());

        vmAch->SetState(AssetState::Waiting);
        Assert::AreEqual(AssetState::Waiting, editor.GetState());
        Assert::AreEqual(AssetState::Waiting, vmAch->GetState());
        Assert::AreEqual(std::wstring(L"Waiting"), editor.GetWaitingLabel());

        vmAch->SetState(AssetState::Active);
        Assert::AreEqual(AssetState::Active, editor.GetState());
        Assert::AreEqual(AssetState::Active, vmAch->GetState());
        Assert::AreEqual(std::wstring(L"Active"), editor.GetWaitingLabel());
    }

    TEST_METHOD(TestSyncStateMeasured)
    {
        AssetEditorViewModelHarness editor;
        editor.mockRuntime.ActivateAchievement(1234U, "M:0xH1234=6.11.");
        editor.mockGameContext.InitializeFromAchievementRuntime();
        auto* vmAch = editor.mockGameContext.Assets().FindAchievement(1234U);
        editor.mockRuntime.SyncAssets();
        vmAch->SetState(AssetState::Inactive);

        editor.LoadAsset(vmAch);

        Assert::IsTrue(editor.HasMeasured());
        Assert::AreEqual(std::wstring(L"[Not Active]"), editor.GetMeasuredValue());

        vmAch->SetState(AssetState::Waiting);
        Assert::AreEqual(std::wstring(L"0/11"), editor.GetMeasuredValue());

        vmAch->SetState(AssetState::Active);
        Assert::AreEqual(std::wstring(L"0/11"), editor.GetMeasuredValue());

        vmAch->SetState(AssetState::Triggered);
        Assert::AreEqual(std::wstring(L"[Not Active]"), editor.GetMeasuredValue());
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

    TEST_METHOD(TestSyncAchievementType)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        achievement.SetAchievementType(ra::data::models::AchievementType::Progression);
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();

        editor.LoadAsset(&achievement);
        Assert::AreEqual(ra::data::models::AchievementType::Progression, editor.GetAchievementType());
        Assert::AreEqual(ra::data::models::AchievementType::Progression, achievement.GetAchievementType());

        editor.SetAchievementType(ra::data::models::AchievementType::None);
        Assert::AreEqual(ra::data::models::AchievementType::None, editor.GetAchievementType());
        Assert::AreEqual(ra::data::models::AchievementType::None, achievement.GetAchievementType());

        editor.SetAchievementType(ra::data::models::AchievementType::Win);
        Assert::AreEqual(ra::data::models::AchievementType::Win, editor.GetAchievementType());
        Assert::AreEqual(ra::data::models::AchievementType::Win, achievement.GetAchievementType());

        editor.SetAchievementType(ra::data::models::AchievementType::Missable);
        Assert::AreEqual(ra::data::models::AchievementType::Missable, editor.GetAchievementType());
        Assert::AreEqual(ra::data::models::AchievementType::Missable, achievement.GetAchievementType());

        achievement.SetAchievementType(ra::data::models::AchievementType::None);
        Assert::AreEqual(ra::data::models::AchievementType::None, editor.GetAchievementType());
        Assert::AreEqual(ra::data::models::AchievementType::None, achievement.GetAchievementType());
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

    TEST_METHOD(TestSyncValueFormat)
    {
        AssetEditorViewModelHarness editor;
        LeaderboardModel leaderboard;
        leaderboard.SetValueFormat(ra::data::ValueFormat::Score);
        leaderboard.CreateServerCheckpoint();
        leaderboard.CreateLocalCheckpoint();

        editor.LoadAsset(&leaderboard);
        Assert::AreEqual(ra::data::ValueFormat::Score, editor.GetValueFormat());
        Assert::AreEqual(ra::data::ValueFormat::Score, leaderboard.GetValueFormat());

        editor.SetValueFormat(ra::data::ValueFormat::Frames);
        Assert::AreEqual(ra::data::ValueFormat::Frames, editor.GetValueFormat());
        Assert::AreEqual(ra::data::ValueFormat::Frames, leaderboard.GetValueFormat());

        leaderboard.SetValueFormat(ra::data::ValueFormat::Centiseconds);
        Assert::AreEqual(ra::data::ValueFormat::Centiseconds, editor.GetValueFormat());
        Assert::AreEqual(ra::data::ValueFormat::Centiseconds, leaderboard.GetValueFormat());
    }

    TEST_METHOD(TestSyncLowerIsBetter)
    {
        AssetEditorViewModelHarness editor;
        LeaderboardModel leaderboard;
        leaderboard.SetLowerIsBetter(true);
        leaderboard.CreateServerCheckpoint();
        leaderboard.CreateLocalCheckpoint();

        editor.LoadAsset(&leaderboard);
        Assert::IsTrue(editor.IsLowerBetter());
        Assert::IsTrue(leaderboard.IsLowerBetter());

        editor.SetLowerIsBetter(false);
        Assert::IsFalse(editor.IsLowerBetter());
        Assert::IsFalse(leaderboard.IsLowerBetter());

        leaderboard.SetLowerIsBetter(true);
        Assert::IsTrue(editor.IsLowerBetter());
        Assert::IsTrue(leaderboard.IsLowerBetter());
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

    TEST_METHOD(TestSyncPauseOnResetLeaderboardStart)
    {
        AssetEditorViewModelHarness editor;
        LeaderboardModel leaderboard;
        leaderboard.SetPauseOnReset(LeaderboardModel::LeaderboardParts::Start);
        leaderboard.CreateServerCheckpoint();
        leaderboard.CreateLocalCheckpoint();

        editor.LoadAsset(&leaderboard);
        editor.SetSelectedLeaderboardPart(AssetEditorViewModel::LeaderboardPart::Start);
        Assert::IsTrue(editor.IsPauseOnReset());
        Assert::AreEqual(LeaderboardModel::LeaderboardParts::Start, leaderboard.GetPauseOnReset());

        editor.SetPauseOnReset(false);
        Assert::IsFalse(editor.IsPauseOnReset());
        Assert::AreEqual(LeaderboardModel::LeaderboardParts::None, leaderboard.GetPauseOnReset());

        editor.SetPauseOnReset(true);
        Assert::IsTrue(editor.IsPauseOnReset());
        Assert::AreEqual(LeaderboardModel::LeaderboardParts::Start, leaderboard.GetPauseOnReset());

        editor.SetPauseOnReset(false);
        Assert::IsFalse(editor.IsPauseOnReset());
        Assert::AreEqual(LeaderboardModel::LeaderboardParts::None, leaderboard.GetPauseOnReset());
    }

    TEST_METHOD(TestSyncPauseOnResetLeaderboardSubmit)
    {
        AssetEditorViewModelHarness editor;
        LeaderboardModel leaderboard;
        leaderboard.SetPauseOnReset(LeaderboardModel::LeaderboardParts::Submit);
        leaderboard.CreateServerCheckpoint();
        leaderboard.CreateLocalCheckpoint();

        editor.LoadAsset(&leaderboard);
        editor.SetSelectedLeaderboardPart(AssetEditorViewModel::LeaderboardPart::Submit);
        Assert::IsTrue(editor.IsPauseOnReset());
        Assert::AreEqual(LeaderboardModel::LeaderboardParts::Submit, leaderboard.GetPauseOnReset());

        editor.SetPauseOnReset(false);
        Assert::IsFalse(editor.IsPauseOnReset());
        Assert::AreEqual(LeaderboardModel::LeaderboardParts::None, leaderboard.GetPauseOnReset());

        editor.SetPauseOnReset(true);
        Assert::IsTrue(editor.IsPauseOnReset());
        Assert::AreEqual(LeaderboardModel::LeaderboardParts::Submit, leaderboard.GetPauseOnReset());

        editor.SetPauseOnReset(false);
        Assert::IsFalse(editor.IsPauseOnReset());
        Assert::AreEqual(LeaderboardModel::LeaderboardParts::None, leaderboard.GetPauseOnReset());
    }

    TEST_METHOD(TestSyncPauseOnResetLeaderboardCancel)
    {
        AssetEditorViewModelHarness editor;
        LeaderboardModel leaderboard;
        leaderboard.SetPauseOnReset(LeaderboardModel::LeaderboardParts::Cancel);
        leaderboard.CreateServerCheckpoint();
        leaderboard.CreateLocalCheckpoint();

        editor.LoadAsset(&leaderboard);
        editor.SetSelectedLeaderboardPart(AssetEditorViewModel::LeaderboardPart::Cancel);
        Assert::IsTrue(editor.IsPauseOnReset());
        Assert::AreEqual(LeaderboardModel::LeaderboardParts::Cancel, leaderboard.GetPauseOnReset());

        editor.SetPauseOnReset(false);
        Assert::IsFalse(editor.IsPauseOnReset());
        Assert::AreEqual(LeaderboardModel::LeaderboardParts::None, leaderboard.GetPauseOnReset());

        editor.SetPauseOnReset(true);
        Assert::IsTrue(editor.IsPauseOnReset());
        Assert::AreEqual(LeaderboardModel::LeaderboardParts::Cancel, leaderboard.GetPauseOnReset());

        editor.SetPauseOnReset(false);
        Assert::IsFalse(editor.IsPauseOnReset());
        Assert::AreEqual(LeaderboardModel::LeaderboardParts::None, leaderboard.GetPauseOnReset());
    }

    TEST_METHOD(TestSyncPauseOnResetLeaderboardValue)
    {
        AssetEditorViewModelHarness editor;
        LeaderboardModel leaderboard;
        leaderboard.SetPauseOnReset(LeaderboardModel::LeaderboardParts::Value);
        leaderboard.CreateServerCheckpoint();
        leaderboard.CreateLocalCheckpoint();

        editor.LoadAsset(&leaderboard);
        editor.SetSelectedLeaderboardPart(AssetEditorViewModel::LeaderboardPart::Value);
        Assert::IsTrue(editor.IsPauseOnReset());
        Assert::AreEqual(LeaderboardModel::LeaderboardParts::Value, leaderboard.GetPauseOnReset());

        editor.SetPauseOnReset(false);
        Assert::IsFalse(editor.IsPauseOnReset());
        Assert::AreEqual(LeaderboardModel::LeaderboardParts::None, leaderboard.GetPauseOnReset());

        editor.SetPauseOnReset(true);
        Assert::IsTrue(editor.IsPauseOnReset());
        Assert::AreEqual(LeaderboardModel::LeaderboardParts::Value, leaderboard.GetPauseOnReset());

        editor.SetPauseOnReset(false);
        Assert::IsFalse(editor.IsPauseOnReset());
        Assert::AreEqual(LeaderboardModel::LeaderboardParts::None, leaderboard.GetPauseOnReset());
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
        editor.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
        editor.mockRuntime.ActivateAchievement(1234U, "0xH1234=1");
        editor.mockGameContext.InitializeFromAchievementRuntime();
        auto* vmAch = editor.mockGameContext.Assets().FindAchievement(1234U);
        editor.mockRuntime.SyncAssets();

        editor.LoadAsset(vmAch);

        // make sure the record got loaded into the runtime
        const auto* pTrigger = vmAch->GetRuntimeTrigger();
        Expects(pTrigger != nullptr);
        Assert::AreEqual(1U, pTrigger->requirement->conditions->operand2.value.num);

        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        auto* pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(ra::data::Memory::Size::EightBit, pCondition->GetSourceSize());
        Assert::AreEqual((int)TriggerOperandType::Address, (int)pCondition->GetSourceType());
        Assert::AreEqual(std::wstring(L"0x1234"), pCondition->GetSourceValue());
        Assert::AreEqual((int)TriggerOperatorType::Equals, (int)pCondition->GetOperator());
        Assert::AreEqual(ra::data::Memory::Size::ThirtyTwoBit, pCondition->GetTargetSize());
        Assert::AreEqual((int)TriggerOperandType::Value, (int)pCondition->GetTargetType());
        Assert::AreEqual(std::wstring(L"1"), pCondition->GetTargetValue());

        pCondition->SetTargetValue(2U);

        // make sure the trigger definition got updated
        Assert::AreEqual(std::string("0xH1234=2"), vmAch->GetTrigger());

        // make sure the runtime record got updated
        pTrigger = vmAch->GetRuntimeTrigger();
        Expects(pTrigger != nullptr);
        Assert::AreEqual(2U, pTrigger->requirement->conditions->operand2.value.num);

        // make sure the UI kept the updated value
        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(ra::data::Memory::Size::EightBit, pCondition->GetSourceSize());
        Assert::AreEqual((int)TriggerOperandType::Address, (int)pCondition->GetSourceType());
        Assert::AreEqual(std::wstring(L"0x1234"), pCondition->GetSourceValue());
        Assert::AreEqual((int)TriggerOperatorType::Equals, (int)pCondition->GetOperator());
        Assert::AreEqual(ra::data::Memory::Size::ThirtyTwoBit, pCondition->GetTargetSize());
        Assert::AreEqual((int)TriggerOperandType::Value, (int)pCondition->GetTargetType());
        Assert::AreEqual(std::wstring(L"2"), pCondition->GetTargetValue());
    }

    TEST_METHOD(TestUpdateInactiveTrigger)
    {
        AssetEditorViewModelHarness editor;
        editor.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
        editor.mockRuntime.ActivateAchievement(1234U, "0xH1234=1");
        editor.mockGameContext.InitializeFromAchievementRuntime();
        auto* vmAch = editor.mockGameContext.Assets().FindAchievement(1234U);
        editor.mockRuntime.SyncAssets();
        vmAch->Deactivate();

        editor.LoadAsset(vmAch);

        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        auto* pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(ra::data::Memory::Size::EightBit, pCondition->GetSourceSize());
        Assert::AreEqual((int)TriggerOperandType::Address, (int)pCondition->GetSourceType());
        Assert::AreEqual(std::wstring(L"0x1234"), pCondition->GetSourceValue());
        Assert::AreEqual((int)TriggerOperatorType::Equals, (int)pCondition->GetOperator());
        Assert::AreEqual(ra::data::Memory::Size::ThirtyTwoBit, pCondition->GetTargetSize());
        Assert::AreEqual((int)TriggerOperandType::Value, (int)pCondition->GetTargetType());
        Assert::AreEqual(std::wstring(L"1"), pCondition->GetTargetValue());

        pCondition->SetTargetValue(2U);

        // make sure the trigger definition got updated
        Assert::AreEqual(std::string("0xH1234=2"), vmAch->GetTrigger());

        // make sure the UI kept the updated value
        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(ra::data::Memory::Size::EightBit, pCondition->GetSourceSize());
        Assert::AreEqual((int)TriggerOperandType::Address, (int)pCondition->GetSourceType());
        Assert::AreEqual(std::wstring(L"0x1234"), pCondition->GetSourceValue());
        Assert::AreEqual((int)TriggerOperatorType::Equals, (int)pCondition->GetOperator());
        Assert::AreEqual(ra::data::Memory::Size::ThirtyTwoBit, pCondition->GetTargetSize());
        Assert::AreEqual((int)TriggerOperandType::Value, (int)pCondition->GetTargetType());
        Assert::AreEqual(std::wstring(L"2"), pCondition->GetTargetValue());
    }

    TEST_METHOD(TestTriggerUpdated)
    {
        AssetEditorViewModelHarness editor;
        editor.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
        editor.mockRuntime.ActivateAchievement(1234U, "0xH1234=1");
        editor.mockGameContext.InitializeFromAchievementRuntime();
        auto* vmAch = editor.mockGameContext.Assets().FindAchievement(1234U);
        editor.mockRuntime.SyncAssets();

        editor.LoadAsset(vmAch);

        // make sure the record got loaded into the runtime
        const auto* pTrigger = vmAch->GetRuntimeTrigger();
        Expects(pTrigger != nullptr);
        Assert::AreEqual(1U, pTrigger->requirement->conditions->operand2.value.num);

        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        auto* pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(ra::data::Memory::Size::EightBit, pCondition->GetSourceSize());
        Assert::AreEqual((int)TriggerOperandType::Address, (int)pCondition->GetSourceType());
        Assert::AreEqual(std::wstring(L"0x1234"), pCondition->GetSourceValue());
        Assert::AreEqual((int)TriggerOperatorType::Equals, (int)pCondition->GetOperator());
        Assert::AreEqual(ra::data::Memory::Size::ThirtyTwoBit, pCondition->GetTargetSize());
        Assert::AreEqual((int)TriggerOperandType::Value, (int)pCondition->GetTargetType());
        Assert::AreEqual(std::wstring(L"1"), pCondition->GetTargetValue());

        vmAch->SetTrigger("0xH1234=2");

        // make sure the trigger definition got updated
        Assert::AreEqual(std::string("0xH1234=2"), vmAch->GetTrigger());

        // make sure the runtime record got updated
        pTrigger = vmAch->GetRuntimeTrigger();
        Expects(pTrigger != nullptr);
        Assert::AreEqual(2U, pTrigger->requirement->conditions->operand2.value.num);

        // make sure the UI picked up the updated value
        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(ra::data::Memory::Size::EightBit, pCondition->GetSourceSize());
        Assert::AreEqual((int)TriggerOperandType::Address, (int)pCondition->GetSourceType());
        Assert::AreEqual(std::wstring(L"0x1234"), pCondition->GetSourceValue());
        Assert::AreEqual((int)TriggerOperatorType::Equals, (int)pCondition->GetOperator());
        Assert::AreEqual(ra::data::Memory::Size::ThirtyTwoBit, pCondition->GetTargetSize());
        Assert::AreEqual((int)TriggerOperandType::Value, (int)pCondition->GetTargetType());
        Assert::AreEqual(std::wstring(L"2"), pCondition->GetTargetValue());
    }

    TEST_METHOD(TestTriggerUpdatedInvalid)
    {
        AssetEditorViewModelHarness editor;
        editor.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
        editor.mockRuntime.ActivateAchievement(1234U, "M:0xH1234=1.1._0xH2345=1.2.");
        editor.mockGameContext.InitializeFromAchievementRuntime();
        auto* vmAch = editor.mockGameContext.Assets().FindAchievement(1234U);
        editor.mockRuntime.SyncAssets();

        editor.LoadAsset(vmAch);

        // make sure the record got loaded into the runtime
        const auto* pTrigger = vmAch->GetRuntimeTrigger();
        Expects(pTrigger != nullptr);

        Assert::AreEqual({ 2U }, editor.Trigger().Conditions().Count());
        editor.Trigger().Conditions().GetItemAt(1)->SetType(ra::ui::viewmodels::TriggerConditionType::Measured);

        // make sure the trigger definition got updated
        Assert::AreEqual(std::string("M:0xH1234=1.1._M:0xH2345=1.2."), vmAch->GetTrigger());

        // make sure the error got set
        Assert::AreEqual(std::wstring(L"Multiple measured targets"), editor.GetAssetValidationError());
        Assert::IsTrue(editor.HasAssetValidationError());

        // make sure the achievement was disabled
        Assert::AreEqual(AssetState::Inactive, vmAch->GetState());

        // make sure the UI kept the change
        Assert::AreEqual({ 2U }, editor.Trigger().Conditions().Count());
        auto* pCondition = editor.Trigger().Conditions().GetItemAt(1);
        Expects(pCondition != nullptr);
        Assert::AreEqual(TriggerConditionType::Measured, pCondition->GetType());

        // fix the error
        editor.Trigger().Conditions().GetItemAt(0)->SetType(ra::ui::viewmodels::TriggerConditionType::Standard);

        // make sure the trigger definition got updated
        Assert::AreEqual(std::string("0xH1234=1.1._M:0xH2345=1.2."), vmAch->GetTrigger());

        // make sure the error got cleared
        Assert::AreEqual(std::wstring(L""), editor.GetAssetValidationError());
        Assert::IsFalse(editor.HasAssetValidationError());

        // the achievement should have remained disabled
        Assert::AreEqual(AssetState::Inactive, vmAch->GetState());

        // make sure the UI kept the change
        Assert::AreEqual({ 2U }, editor.Trigger().Conditions().Count());
        pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(TriggerConditionType::Standard, pCondition->GetType());
        pCondition = editor.Trigger().Conditions().GetItemAt(1);
        Expects(pCondition != nullptr);
        Assert::AreEqual(TriggerConditionType::Measured, pCondition->GetType());
    }

    TEST_METHOD(TestTriggerUpdatedInvalidAlts)
    {
        AssetEditorViewModelHarness editor;
        editor.mockRuntime.ActivateAchievement(1234U, "0=0SM:0xH1234=1.1.S0xH2345=1.2.");
        editor.mockGameContext.InitializeFromAchievementRuntime();
        auto* vmAch = editor.mockGameContext.Assets().FindAchievement(1234U);
        editor.mockRuntime.SyncAssets();

        editor.LoadAsset(vmAch);

        // make sure the record got loaded into the runtime
        const auto* pTrigger = vmAch->GetRuntimeTrigger();
        Expects(pTrigger != nullptr);

        editor.Trigger().SetSelectedGroupIndex(2);
        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        editor.Trigger().Conditions().GetItemAt(0)->SetType(ra::ui::viewmodels::TriggerConditionType::Measured);

        // make sure the trigger definition got updated
        Assert::AreEqual(std::string("0=0SM:0xH1234=1.1.SM:0xH2345=1.2."), vmAch->GetTrigger());

        // make sure the error got set
        Assert::AreEqual(std::wstring(L"Multiple measured targets"), editor.GetAssetValidationError());
        Assert::IsTrue(editor.HasAssetValidationError());

        // make sure the achievement was disabled
        Assert::AreEqual(AssetState::Inactive, vmAch->GetState());

        // make sure the UI kept the change
        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        auto* pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(TriggerConditionType::Measured, pCondition->GetType());

        // select the other alt
        editor.Trigger().SetSelectedGroupIndex(1);
        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(TriggerConditionType::Measured, pCondition->GetType());
        Assert::AreEqual(1U, pCondition->GetRequiredHits());
        Assert::AreEqual(std::wstring(L"Multiple measured targets"), editor.GetAssetValidationError());
        Assert::IsTrue(editor.HasAssetValidationError());

        // select the broken alt
        editor.Trigger().SetSelectedGroupIndex(2);
        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(TriggerConditionType::Measured, pCondition->GetType());
        Assert::AreEqual(2U, pCondition->GetRequiredHits());
        Assert::AreEqual(std::wstring(L"Multiple measured targets"), editor.GetAssetValidationError());
        Assert::IsTrue(editor.HasAssetValidationError());

        // select the other alt
        editor.Trigger().SetSelectedGroupIndex(1);
        Assert::AreEqual(1U, pCondition->GetRequiredHits());

        // fix the error
        pCondition->SetRequiredHits(2);

        // make sure the trigger definition got updated
        Assert::AreEqual(std::string("0=0SM:0xH1234=1.2.SM:0xH2345=1.2."), vmAch->GetTrigger());

        // make sure the error got cleared
        Assert::AreEqual(std::wstring(L""), editor.GetAssetValidationError());
        Assert::IsFalse(editor.HasAssetValidationError());

        // the achievement should have remained disabled
        Assert::AreEqual(AssetState::Inactive, vmAch->GetState());

        // make sure the UI kept the change
        pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(TriggerConditionType::Measured, pCondition->GetType());
        Assert::AreEqual(2U, pCondition->GetRequiredHits());

        // select the other alt (was broken)
        pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(TriggerConditionType::Measured, pCondition->GetType());
        Assert::AreEqual(2U, pCondition->GetRequiredHits());
    }

    TEST_METHOD(TestTriggerUpdatedValueToDelta)
    {
        // When a condition changes from value to delta, two properties are changed (type and size)
        // Expect a single change notification and make sure the condition is referenced by the updated condition

        AssetEditorViewModelHarness editor;
        editor.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
        editor.mockRuntime.ActivateAchievement(1234U, "0xH1234=1");
        editor.mockGameContext.InitializeFromAchievementRuntime();
        auto* vmAch = editor.mockGameContext.Assets().FindAchievement(1234U);
        editor.mockRuntime.SyncAssets();

        editor.LoadAsset(vmAch);

        editor.Trigger().SetSelectedGroupIndex(0);
        auto& condition = *editor.Trigger().Conditions().GetItemAt(0);
        auto& group = *editor.Trigger().Groups().GetItemAt(0);
        const auto initialVersion = editor.GetTriggerVersion();
        Assert::IsNotNull(group.m_pConditionSet);

        // change to address switches size to match source
        condition.SetTargetType(TriggerOperandType::Delta);
        Assert::AreEqual(std::string("0xH1234=d0xH0001"), vmAch->GetTrigger());
        Assert::AreEqual(initialVersion + 1, editor.GetTriggerVersion());
        Assert::IsNotNull(group.m_pConditionSet);

        // change to value sets size back to 32-bit
        condition.SetTargetType(TriggerOperandType::Value);
        Assert::AreEqual(std::string("0xH1234=1"), vmAch->GetTrigger());
        Assert::AreEqual(initialVersion + 2, editor.GetTriggerVersion());
        Assert::IsNotNull(group.m_pConditionSet);
    }

    TEST_METHOD(TestTriggerUnmodifiedUpdate)
    {
        AssetEditorViewModelHarness editor;
        editor.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
        editor.mockRuntime.ActivateAchievement(1234U, "0xH1234=1_0xH1235=2");
        editor.mockGameContext.InitializeFromAchievementRuntime();
        auto* vmAch = editor.mockGameContext.Assets().FindAchievement(1234U);
        editor.mockRuntime.SyncAssets();

        editor.LoadAsset(vmAch);

        // make sure the record got loaded into the runtime
        const auto* pTrigger = vmAch->GetRuntimeTrigger();
        Expects(pTrigger != nullptr);

        Assert::AreEqual({ 2U }, editor.Trigger().Conditions().Count());
        editor.Trigger().SelectRange(1, 1, true);        
        editor.Trigger().MoveSelectedConditionsUp();

        // make sure the trigger definition got updated
        Assert::AreEqual(std::string("0xH1235=2_0xH1234=1"), vmAch->GetTrigger());
        Assert::IsNotNull(editor.Trigger().Groups().GetItemAt(0)->m_pConditionSet);

        // make sure the UI picked up the updated value
        Assert::AreEqual({ 2U }, editor.Trigger().Conditions().Count());
        const auto* pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(std::wstring(L"0x1235"), pCondition->GetSourceValue());
        Assert::AreEqual(std::wstring(L"2"), pCondition->GetTargetValue());
        pCondition = editor.Trigger().Conditions().GetItemAt(1);
        Expects(pCondition != nullptr);
        Assert::AreEqual(std::wstring(L"0x1234"), pCondition->GetSourceValue());
        Assert::AreEqual(std::wstring(L"1"), pCondition->GetTargetValue());

        // second attempt to move will not actually do anything
        Assert::IsTrue(editor.Trigger().Conditions().GetItemAt(0)->IsSelected());
        Assert::IsFalse(editor.Trigger().Conditions().GetItemAt(1)->IsSelected());
        editor.Trigger().MoveSelectedConditionsUp();

        // make sure the trigger definition did not update
        Assert::AreEqual(std::string("0xH1235=2_0xH1234=1"), vmAch->GetTrigger());
        Assert::IsNotNull(editor.Trigger().Groups().GetItemAt(0)->m_pConditionSet);

        // make sure the UI did not update
        Assert::AreEqual({ 2U }, editor.Trigger().Conditions().Count());
        pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(std::wstring(L"0x1235"), pCondition->GetSourceValue());
        Assert::AreEqual(std::wstring(L"2"), pCondition->GetTargetValue());
        pCondition = editor.Trigger().Conditions().GetItemAt(1);
        Expects(pCondition != nullptr);
        Assert::AreEqual(std::wstring(L"0x1234"), pCondition->GetSourceValue());
        Assert::AreEqual(std::wstring(L"1"), pCondition->GetTargetValue());
    }

    TEST_METHOD(TestValueUpdated)
    {
        AssetEditorViewModelHarness editor;
        const std::string sDefinition = "STA:0x8000=0::SUB:0x8000=1::CAN:0x8000=2::VAL:M:0xH1234";
        editor.mockRuntime.ActivateLeaderboard(1234U, sDefinition);
        editor.mockGameContext.InitializeFromAchievementRuntime();
        auto* vmLbd = editor.mockGameContext.Assets().FindLeaderboard(1234U);
        vmLbd->SetDefinition(sDefinition);
        editor.mockRuntime.SyncAssets();

        editor.LoadAsset(vmLbd);
        editor.SetSelectedLeaderboardPart(AssetEditorViewModel::LeaderboardPart::Value);

        // make sure the record got loaded into the runtime
        const auto* pLeaderboard = vmLbd->GetRuntimeLeaderboard();
        Expects(pLeaderboard != nullptr);

        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        auto* pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(ra::data::Memory::Size::EightBit, pCondition->GetSourceSize());
        Assert::AreEqual((int)TriggerOperandType::Address, (int)pCondition->GetSourceType());
        Assert::AreEqual(std::wstring(L"0x1234"), pCondition->GetSourceValue());
        Assert::AreEqual((int)TriggerOperatorType::None, (int)pCondition->GetOperator());

        vmLbd->SetValueDefinition("M:0xH2222");

        // make sure the value definition got updated
        Assert::AreEqual(std::string("M:0xH2222"), vmLbd->GetValueDefinition());

        // make sure the runtime record got updated
        pLeaderboard = vmLbd->GetRuntimeLeaderboard();
        Expects(pLeaderboard != nullptr);
        Assert::AreEqual(0x2222U, pLeaderboard->value.conditions->conditions->operand1.value.memref->address);

        // make sure the UI picked up the updated value
        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(ra::data::Memory::Size::EightBit, pCondition->GetSourceSize());
        Assert::AreEqual((int)TriggerOperandType::Address, (int)pCondition->GetSourceType());
        Assert::AreEqual(std::wstring(L"0x2222"), pCondition->GetSourceValue());
        Assert::AreEqual((int)TriggerOperatorType::None, (int)pCondition->GetOperator());

        // modify the UI
        pCondition->SetSourceValue(0x3333U);

        // make sure the value definition got updated
        Assert::AreEqual(std::string("M:0xH3333"), vmLbd->GetValueDefinition());

        // make sure the runtime record got updated
        pLeaderboard = vmLbd->GetRuntimeLeaderboard();
        Expects(pLeaderboard != nullptr);
        Assert::AreEqual(0x3333U, pLeaderboard->value.conditions->conditions->operand1.value.memref->address);
    }

    TEST_METHOD(TestDoFrameUpdatesHitsFromActiveAchievement)
    {
        AssetEditorViewModelHarness editor;
        editor.mockRuntime.ActivateAchievement(1234U, "0xH1234=1");
        editor.mockGameContext.InitializeFromAchievementRuntime();
        auto* vmAch = editor.mockGameContext.Assets().FindAchievement(1234U);
        editor.mockRuntime.SyncAssets();

        editor.LoadAsset(vmAch);

        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        auto* pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(0U, pCondition->GetCurrentHits());

        const auto* pTrigger = vmAch->GetRuntimeTrigger();
        Expects(pTrigger != nullptr);
        pTrigger->requirement->conditions->current_hits = 6U;
        Assert::AreEqual(0U, pCondition->GetCurrentHits());

        editor.DoFrame();
        Assert::AreEqual(6U, pCondition->GetCurrentHits());
    }

    TEST_METHOD(TestDoFrameUpdatesHitsFromActiveAchievementWhenHidden)
    {
        AssetEditorViewModelHarness editor;
        editor.mockRuntime.ActivateAchievement(1234U, "0xH1234=1");
        editor.mockGameContext.InitializeFromAchievementRuntime();
        auto* vmAch = editor.mockGameContext.Assets().FindAchievement(1234U);
        editor.mockRuntime.SyncAssets();

        editor.SetIsVisible(false);
        editor.LoadAsset(vmAch);

        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        auto* pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(0U, pCondition->GetCurrentHits());

        const auto* pTrigger = vmAch->GetRuntimeTrigger();
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

    TEST_METHOD(TestDoFrameUpdatesMeasuredFromActiveAchievement)
    {
        AssetEditorViewModelHarness editor;
        editor.mockRuntime.ActivateAchievement(1234U, "M:0xH1234=1.99.");
        editor.mockGameContext.InitializeFromAchievementRuntime();
        auto* vmAch = editor.mockGameContext.Assets().FindAchievement(1234U);
        editor.mockRuntime.SyncAssets();

        editor.LoadAsset(vmAch);

        auto* pTrigger = vmAch->GetMutableRuntimeTrigger();
        Expects(pTrigger != nullptr);
        pTrigger->measured_value = 6U;
        Assert::AreEqual(std::wstring(L"0/99"), editor.GetMeasuredValue());

        editor.DoFrame();
        Assert::AreEqual(std::wstring(L"6/99"), editor.GetMeasuredValue());
    }

    TEST_METHOD(TestDoFrameUpdatesMeasuredFromActiveLeaderboard)
    {
        AssetEditorViewModelHarness editor;
        editor.mockRuntime.ActivateLeaderboard(1234U, "STA:1=1::SUB:1=0::CAN:0=1::VAL:M:0xH1234")->format = RC_FORMAT_CENTISECS;
        editor.mockGameContext.InitializeFromAchievementRuntime();
        auto* vmLbd = editor.mockGameContext.Assets().FindLeaderboard(1234U);
        editor.mockRuntime.SyncAssets();

        editor.LoadAsset(vmLbd);

        auto* pDefinition = vmLbd->GetMutableRuntimeLeaderboard();
        Expects(pDefinition != nullptr);
        pDefinition->value.value.value = 6U;
        Assert::AreEqual(std::wstring(L"0"), editor.GetMeasuredValue());
        Assert::AreEqual(std::wstring(L"0:00.00"), editor.GetFormattedValue());

        editor.DoFrame();
        Assert::AreEqual(std::wstring(L"6"), editor.GetMeasuredValue());
        Assert::AreEqual(std::wstring(L"0:00.06"), editor.GetFormattedValue());

        pDefinition->value.value.value = 12345U;
        editor.DoFrame();
        Assert::AreEqual(std::wstring(L"12345"), editor.GetMeasuredValue());
        Assert::AreEqual(std::wstring(L"2:03.45"), editor.GetFormattedValue());
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

    TEST_METHOD(TestDecimalPreferredUpdatesConditions)
    {
        AssetEditorViewModelHarness editor;
        editor.mockRuntime.ActivateAchievement(1234U, "0xH1234=1_fF2222=f2.3");
        editor.mockGameContext.InitializeFromAchievementRuntime();
        auto* vmAch = editor.mockGameContext.Assets().FindAchievement(1234U);
        editor.mockRuntime.SyncAssets();

        const auto* pTrigger = vmAch->GetRuntimeTrigger();
        Expects(pTrigger != nullptr);
        pTrigger->requirement->conditions->current_hits = 6U;

        editor.LoadAsset(vmAch);
        const auto* pCondition1 = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition1 != nullptr);
        const auto* pCondition2 = editor.Trigger().Conditions().GetItemAt(1);
        Expects(pCondition2 != nullptr);

        Assert::IsFalse(editor.IsDecimalPreferred());
        Assert::AreEqual(std::wstring(L"0x1234"), pCondition1->GetSourceValue());
        Assert::AreEqual(std::wstring(L"0x01"), pCondition1->GetTargetValue());
        Assert::AreEqual(std::wstring(L"0x2222"), pCondition2->GetSourceValue());
        Assert::AreEqual(std::wstring(L"2.3"), pCondition2->GetTargetValue());

        editor.SetDecimalPreferred(true);
        Assert::IsTrue(editor.IsDecimalPreferred());
        Assert::AreEqual(std::wstring(L"0x1234"), pCondition1->GetSourceValue());
        Assert::AreEqual(std::wstring(L"1"), pCondition1->GetTargetValue());
        Assert::AreEqual(std::wstring(L"0x2222"), pCondition2->GetSourceValue());
        Assert::AreEqual(std::wstring(L"2.3"), pCondition2->GetTargetValue());

        editor.SetDecimalPreferred(false);
        Assert::IsFalse(editor.IsDecimalPreferred());
        Assert::AreEqual(std::wstring(L"0x1234"), pCondition1->GetSourceValue());
        Assert::AreEqual(std::wstring(L"0x01"), pCondition1->GetTargetValue());
        Assert::AreEqual(std::wstring(L"0x2222"), pCondition2->GetSourceValue());
        Assert::AreEqual(std::wstring(L"2.3"), pCondition2->GetTargetValue());
    }

    TEST_METHOD(TestDecimalPreferredUpdatesConditionsWhenError)
    {
        AssetEditorViewModelHarness editor;
        editor.mockRuntime.ActivateAchievement(1234U, "0xH1234=1_M:0xH2345=23");
        editor.mockGameContext.InitializeFromAchievementRuntime();
        auto* vmAch = editor.mockGameContext.Assets().FindAchievement(1234U);
        editor.mockRuntime.SyncAssets();

        const auto* pTrigger = vmAch->GetRuntimeTrigger();
        Expects(pTrigger != nullptr);

        editor.LoadAsset(vmAch);
        const auto* pCondition1 = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition1 != nullptr);
        const auto* pCondition2 = editor.Trigger().Conditions().GetItemAt(1);
        Expects(pCondition2 != nullptr);

        ra::services::mocks::MockClipboard mockClipboard;
        mockClipboard.SetText(L"M:0x2345=23");
        editor.Trigger().PasteFromClipboard();
        mockClipboard.SetText(L"");

        const auto* pCondition3 = editor.Trigger().Conditions().GetItemAt(2);
        Expects(pCondition3 != nullptr);
        Assert::IsTrue(editor.HasAssetValidationError());
        Assert::AreEqual(std::wstring(L"Multiple measured targets"), editor.GetAssetValidationError());

        Assert::IsFalse(editor.IsDecimalPreferred());
        Assert::AreEqual(std::wstring(L"0x1234"), pCondition1->GetSourceValue());
        Assert::AreEqual(std::wstring(L"0x01"), pCondition1->GetTargetValue());
        Assert::AreEqual(std::wstring(L"0x2345"), pCondition2->GetSourceValue());
        Assert::AreEqual(std::wstring(L"0x17"), pCondition2->GetTargetValue());
        Assert::AreEqual(std::wstring(L"0x2345"), pCondition3->GetSourceValue());
        Assert::AreEqual(std::wstring(L"0x17"), pCondition3->GetTargetValue());

        // when the editor is in an error state, there's no backing trigger.
        // the columns must be updated without being regenerated.
        editor.SetDecimalPreferred(true);
        Assert::IsTrue(editor.IsDecimalPreferred());
        Assert::AreEqual({3U}, editor.Trigger().Conditions().Count());
        Assert::AreEqual(std::wstring(L"0x1234"), pCondition1->GetSourceValue());
        Assert::AreEqual(std::wstring(L"1"), pCondition1->GetTargetValue());
        Assert::AreEqual(std::wstring(L"0x2345"), pCondition2->GetSourceValue());
        Assert::AreEqual(std::wstring(L"23"), pCondition2->GetTargetValue());
        Assert::AreEqual(std::wstring(L"0x2345"), pCondition3->GetSourceValue());
        Assert::AreEqual(std::wstring(L"23"), pCondition3->GetTargetValue());

        editor.SetDecimalPreferred(false);
        Assert::IsFalse(editor.IsDecimalPreferred());
        Assert::AreEqual({3U}, editor.Trigger().Conditions().Count());
        Assert::AreEqual(std::wstring(L"0x1234"), pCondition1->GetSourceValue());
        Assert::AreEqual(std::wstring(L"0x01"), pCondition1->GetTargetValue());
        Assert::AreEqual(std::wstring(L"0x2345"), pCondition2->GetSourceValue());
        Assert::AreEqual(std::wstring(L"0x17"), pCondition2->GetTargetValue());
        Assert::AreEqual(std::wstring(L"0x2345"), pCondition3->GetSourceValue());
        Assert::AreEqual(std::wstring(L"0x17"), pCondition3->GetTargetValue());
    }

    TEST_METHOD(TestCaptureRestoreHits)
    {
        AssetEditorViewModelHarness editor;
        editor.mockRuntime.ActivateAchievement(1234U, "0xH1234=1");
        editor.mockGameContext.InitializeFromAchievementRuntime();
        auto* vmAch = editor.mockGameContext.Assets().FindAchievement(1234U);
        editor.mockRuntime.SyncAssets();

        const auto* pTrigger = vmAch->GetRuntimeTrigger();
        Expects(pTrigger != nullptr);
        pTrigger->requirement->conditions->current_hits = 6U;

        editor.LoadAsset(vmAch);

        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        auto* pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(6U, pCondition->GetCurrentHits());

        pTrigger->requirement->conditions->current_hits = 7U;
        editor.DoFrame();
        Assert::AreEqual(7U, pCondition->GetCurrentHits());

        editor.LoadAsset(nullptr);
        Assert::AreEqual({ 0U }, editor.Trigger().Conditions().Count());

        editor.LoadAsset(vmAch);
        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(7U, pCondition->GetCurrentHits());
    }

    TEST_METHOD(TestCaptureRestoreHitsTriggered)
    {
        AssetEditorViewModelHarness editor;
        editor.mockRuntime.ActivateAchievement(1234U, "0xH1234=1");
        editor.mockGameContext.InitializeFromAchievementRuntime();
        auto* vmAch = editor.mockGameContext.Assets().FindAchievement(1234U);
        editor.mockRuntime.SyncAssets();

        auto* pTrigger = vmAch->GetMutableRuntimeTrigger();
        Expects(pTrigger != nullptr);
        pTrigger->requirement->conditions->current_hits = 6U;
        pTrigger->has_hits = 1;

        editor.LoadAsset(vmAch);

        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        auto* pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(6U, pCondition->GetCurrentHits());

        pTrigger->requirement->conditions->current_hits = 7U;
        pTrigger->state = RC_TRIGGER_STATE_TRIGGERED;
        vmAch->SetState(ra::data::models::AssetState::Triggered);

        // the updated state should still be reflected in the UI
        Assert::AreEqual(7U, pCondition->GetCurrentHits());

        editor.LoadAsset(nullptr);
        Assert::AreEqual({ 0U }, editor.Trigger().Conditions().Count());

        editor.LoadAsset(vmAch);
        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        // when reloaded, the updated state should be remembered
        Assert::AreEqual(7U, pCondition->GetCurrentHits());
    }

    TEST_METHOD(TestCaptureRestoreHitsTriggeredChanged)
    {
        AssetEditorViewModelHarness editor;
        editor.mockRuntime.ActivateAchievement(1234U, "0xH1234=1");
        editor.mockGameContext.InitializeFromAchievementRuntime();
        auto* vmAch = editor.mockGameContext.Assets().FindAchievement(1234U);
        editor.mockRuntime.SyncAssets();

        auto* pTrigger = vmAch->GetMutableRuntimeTrigger();
        Expects(pTrigger != nullptr);
        pTrigger->requirement->conditions->current_hits = 6U;
        pTrigger->has_hits = 1;

        editor.LoadAsset(vmAch);

        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        auto* pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(6U, pCondition->GetCurrentHits());

        pTrigger->requirement->conditions->current_hits = 7U;
        pTrigger->state = RC_TRIGGER_STATE_TRIGGERED;
        vmAch->SetState(ra::data::models::AssetState::Triggered);

        // but the updated state should still be reflected in the UI
        Assert::AreEqual(7U, pCondition->GetCurrentHits());

        editor.LoadAsset(nullptr);
        Assert::AreEqual({ 0U }, editor.Trigger().Conditions().Count());

        vmAch->SetTrigger("0xH1234=2");
        editor.LoadAsset(vmAch);
        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        // definition changed. when reloaded, the updated state should be discarded
        Assert::AreEqual(0U, pCondition->GetCurrentHits());
    }

    TEST_METHOD(TestSelectedLeaderboardPart)
    {
        AssetEditorViewModelHarness editor;
        LeaderboardModel leaderboard;
        leaderboard.SetStartTrigger("0xH1234=6.1.");
        leaderboard.SetSubmitTrigger("0xH2345!=99");
        leaderboard.SetCancelTrigger("0xH3456>3");
        leaderboard.SetValueDefinition("M:0xH4444");
        leaderboard.CreateServerCheckpoint();
        leaderboard.CreateLocalCheckpoint();

        editor.LoadAsset(&leaderboard);
        Assert::AreEqual(std::string("0xH1234=6.1."), editor.Trigger().Serialize());
        Assert::AreEqual(std::wstring(L"Groups:"), editor.GetGroupsHeaderLabel());
        Assert::IsFalse(editor.Trigger().IsValue());
        Assert::IsTrue(editor.IsTrigger());
        Assert::IsFalse(leaderboard.IsModified());

        editor.SetSelectedLeaderboardPart(AssetEditorViewModel::LeaderboardPart::Submit);
        Assert::AreEqual(std::string("0xH2345!=99"), editor.Trigger().Serialize());
        Assert::AreEqual(std::wstring(L"Groups:"), editor.GetGroupsHeaderLabel());
        Assert::IsFalse(editor.Trigger().IsValue());
        Assert::IsTrue(editor.IsTrigger());
        Assert::IsFalse(leaderboard.IsModified());

        editor.SetSelectedLeaderboardPart(AssetEditorViewModel::LeaderboardPart::Cancel);
        Assert::AreEqual(std::string("0xH3456>3"), editor.Trigger().Serialize());
        Assert::AreEqual(std::wstring(L"Groups:"), editor.GetGroupsHeaderLabel());
        Assert::IsFalse(editor.Trigger().IsValue());
        Assert::IsTrue(editor.IsTrigger());
        Assert::IsFalse(leaderboard.IsModified());

        editor.SetSelectedLeaderboardPart(AssetEditorViewModel::LeaderboardPart::Value);
        Assert::AreEqual(std::string("M:0xH4444"), editor.Trigger().Serialize());
        Assert::AreEqual(std::wstring(L"Max of:"), editor.GetGroupsHeaderLabel());
        Assert::IsTrue(editor.Trigger().IsValue());
        Assert::IsFalse(editor.IsTrigger());
        Assert::IsFalse(leaderboard.IsModified());

        editor.SetSelectedLeaderboardPart(AssetEditorViewModel::LeaderboardPart::Start);
        Assert::AreEqual(std::string("0xH1234=6.1."), editor.Trigger().Serialize());
        Assert::AreEqual(std::wstring(L"Groups:"), editor.GetGroupsHeaderLabel());
        Assert::IsFalse(editor.Trigger().IsValue());
        Assert::IsTrue(editor.IsTrigger());
        Assert::IsFalse(leaderboard.IsModified());
    }

    TEST_METHOD(TestSelectBadgeFileCancel)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        editor.LoadAsset(&achievement);
        const std::wstring sBadge = achievement.GetBadge();

        bool bDialogSeen = false;
        editor.mockDesktop.ExpectWindow<ra::ui::viewmodels::FileDialogViewModel>(
            [&bDialogSeen](ra::ui::viewmodels::FileDialogViewModel&)
        {
            bDialogSeen = true;

            return DialogResult::Cancel;
        });

        std::wstring sMessage;
        editor.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&sMessage](ra::ui::viewmodels::MessageBoxViewModel& vmMessage)
        {
            sMessage = vmMessage.GetMessage();
            return DialogResult::OK;
        });

        editor.SelectBadgeFile();

        Assert::AreEqual(sBadge, achievement.GetBadge());
        Assert::IsTrue(bDialogSeen);
        Assert::AreEqual(std::wstring(), sMessage);
    }

    TEST_METHOD(TestSelectBadgeFilePNG)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        editor.LoadAsset(&achievement);

        bool bDialogSeen = false;
        editor.mockDesktop.ExpectWindow<ra::ui::viewmodels::FileDialogViewModel>(
            [&bDialogSeen](ra::ui::viewmodels::FileDialogViewModel& vmFileDialog)
        {
            bDialogSeen = true;

            vmFileDialog.SetFileName(L"C:\\image.png");
            return DialogResult::OK;
        });

        std::wstring sMessage;
        editor.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&sMessage](ra::ui::viewmodels::MessageBoxViewModel& vmMessage)
        {
            sMessage = vmMessage.GetMessage();
            return DialogResult::OK;
        });

        const std::string sFileContents("\x89PNG\x0D\x0A\x1A\x0D\x00\x00\x00\x0DIHDR", 16);

        editor.mockFileSystem.MockFile(L"C:\\image.png", sFileContents);

        editor.SelectBadgeFile();

        Assert::AreEqual(std::wstring(L"REPO:C:\\image.png"), achievement.GetBadge());
        Assert::IsTrue(bDialogSeen);
        Assert::AreEqual(std::wstring(), sMessage);
    }

    TEST_METHOD(TestSelectBadgeFilePNGInvalid)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        editor.LoadAsset(&achievement);
        const std::wstring sBadge = achievement.GetBadge();

        bool bDialogSeen = false;
        editor.mockDesktop.ExpectWindow<ra::ui::viewmodels::FileDialogViewModel>(
            [&bDialogSeen](ra::ui::viewmodels::FileDialogViewModel& vmFileDialog)
        {
            bDialogSeen = true;

            vmFileDialog.SetFileName(L"C:\\image.png");
            return DialogResult::OK;
        });

        std::wstring sMessage;
        editor.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&sMessage](ra::ui::viewmodels::MessageBoxViewModel& vmMessage)
        {
            sMessage = vmMessage.GetMessage();
            return DialogResult::OK;
        });

        // this is from a BMP file, which the server doesn't support
        const std::string sFileContents("BM\x36\xC4\x12\x00\x00\x00\x00\x00\x36\x04\x00\x00\x28\x00", 16);

        editor.mockFileSystem.MockFile(L"C:\\image.png", sFileContents);

        editor.SelectBadgeFile();

        Assert::AreEqual(sBadge, achievement.GetBadge());
        Assert::IsTrue(bDialogSeen);
        Assert::AreEqual(std::wstring(L"File does not appear to be a valid png image."), sMessage);
    }

    TEST_METHOD(TestSelectBadgeFileGIF)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        editor.LoadAsset(&achievement);

        bool bDialogSeen = false;
        editor.mockDesktop.ExpectWindow<ra::ui::viewmodels::FileDialogViewModel>(
            [&bDialogSeen](ra::ui::viewmodels::FileDialogViewModel& vmFileDialog)
        {
            bDialogSeen = true;

            vmFileDialog.SetFileName(L"C:\\image.gif");
            return DialogResult::OK;
        });

        std::wstring sMessage;
        editor.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&sMessage](ra::ui::viewmodels::MessageBoxViewModel& vmMessage)
        {
            sMessage = vmMessage.GetMessage();
            return DialogResult::OK;
        });

        const std::string sFileContents("GIF89a\x39\x61\x64\x00\x37\x00\xc7\x00\x00\x00\x00\x00", 16);

        editor.mockFileSystem.MockFile(L"C:\\image.gif", sFileContents);

        editor.SelectBadgeFile();

        Assert::AreEqual(std::wstring(L"REPO:C:\\image.gif"), achievement.GetBadge());
        Assert::IsTrue(bDialogSeen);
        Assert::AreEqual(std::wstring(), sMessage);
    }

    TEST_METHOD(TestSelectBadgeFileGIFInvalid)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        editor.LoadAsset(&achievement);
        const std::wstring sBadge = achievement.GetBadge();

        bool bDialogSeen = false;
        editor.mockDesktop.ExpectWindow<ra::ui::viewmodels::FileDialogViewModel>(
            [&bDialogSeen](ra::ui::viewmodels::FileDialogViewModel& vmFileDialog)
        {
            bDialogSeen = true;

            vmFileDialog.SetFileName(L"C:\\image.gif");
            return DialogResult::OK;
        });

        std::wstring sMessage;
        editor.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&sMessage](ra::ui::viewmodels::MessageBoxViewModel& vmMessage)
        {
            sMessage = vmMessage.GetMessage();
            return DialogResult::OK;
        });

        // this is from a PNG file
        const std::string sFileContents("\x89PNG\x0D\x0A\x1A\x0D\x00\x00\x00\x0DIHDR", 16);

        editor.mockFileSystem.MockFile(L"C:\\image.gif", sFileContents);

        editor.SelectBadgeFile();

        Assert::AreEqual(sBadge, achievement.GetBadge());
        Assert::IsTrue(bDialogSeen);
        Assert::AreEqual(std::wstring(L"File does not appear to be a valid gif image."), sMessage);
    }

    TEST_METHOD(TestSelectBadgeFileJPG)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        editor.LoadAsset(&achievement);

        bool bDialogSeen = false;
        editor.mockDesktop.ExpectWindow<ra::ui::viewmodels::FileDialogViewModel>(
            [&bDialogSeen](ra::ui::viewmodels::FileDialogViewModel& vmFileDialog)
        {
            bDialogSeen = true;

            vmFileDialog.SetFileName(L"C:\\image.jpg");
            return DialogResult::OK;
        });

        std::wstring sMessage;
        editor.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&sMessage](ra::ui::viewmodels::MessageBoxViewModel& vmMessage)
        {
            sMessage = vmMessage.GetMessage();
            return DialogResult::OK;
        });

        const std::string sFileContents("\xFF\xD8\xFF\xE0\x00\x10JFIF\x00\x01\x00\x01\x00\x96", 16);

        editor.mockFileSystem.MockFile(L"C:\\image.jpg", sFileContents);

        editor.SelectBadgeFile();

        Assert::AreEqual(std::wstring(L"REPO:C:\\image.jpg"), achievement.GetBadge());
        Assert::IsTrue(bDialogSeen);
        Assert::AreEqual(std::wstring(), sMessage);
    }

    TEST_METHOD(TestSelectBadgeFileJPGInvalid)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        editor.LoadAsset(&achievement);
        const std::wstring sBadge = achievement.GetBadge();

        bool bDialogSeen = false;
        editor.mockDesktop.ExpectWindow<ra::ui::viewmodels::FileDialogViewModel>(
            [&bDialogSeen](ra::ui::viewmodels::FileDialogViewModel& vmFileDialog)
        {
            bDialogSeen = true;

            vmFileDialog.SetFileName(L"C:\\image.jpg");
            return DialogResult::OK;
        });

        std::wstring sMessage;
        editor.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&sMessage](ra::ui::viewmodels::MessageBoxViewModel& vmMessage)
        {
            sMessage = vmMessage.GetMessage();
            return DialogResult::OK;
        });

        // this is from a PNG file
        const std::string sFileContents("\x89PNG\x0D\x0A\x1A\x0D\x00\x00\x00\x0DIHDR", 16);

        editor.mockFileSystem.MockFile(L"C:\\image.jpg", sFileContents);

        editor.SelectBadgeFile();

        Assert::AreEqual(sBadge, achievement.GetBadge());
        Assert::IsTrue(bDialogSeen);
        Assert::AreEqual(std::wstring(L"File does not appear to be a valid jpg image."), sMessage);
    }

    TEST_METHOD(TestChangingGroupUpdatesDebugHighlights)
    {
        AssetEditorViewModelHarness editor;
        const auto nDefaultColor = ra::to_unsigned(TriggerConditionViewModel::RowColorProperty.GetDefaultValue());

        AchievementModel achievement;
        auto* info = editor.mockRuntime.ActivateAchievement(1, "0=1S0=1S1=1"); // must be in runtime to apply colors
        achievement.InitializeFromPublishedAchievement(*info, "0=1S0=1S1=1");
        achievement.SetLocalAchievementInfo(*info);
        editor.LoadAsset(&achievement);

        auto* pTrigger = achievement.GetRuntimeTrigger();
        Expects(pTrigger != nullptr);
        pTrigger->alternative->next->conditions->is_true = 1; // simulate evaluation of 1=1 being true

        Assert::AreEqual(0, editor.Trigger().GetSelectedGroupIndex());
        Assert::AreEqual(nDefaultColor, editor.Trigger().Conditions().GetItemAt(0)->GetRowColor().ARGB);

        editor.Trigger().SetSelectedGroupIndex(2);
        Assert::AreEqual(2, editor.Trigger().GetSelectedGroupIndex());
        Assert::AreEqual(nDefaultColor, editor.Trigger().Conditions().GetItemAt(0)->GetRowColor().ARGB);

        editor.Trigger().SetSelectedGroupIndex(1);
        Assert::AreEqual(1, editor.Trigger().GetSelectedGroupIndex());
        Assert::AreEqual(nDefaultColor, editor.Trigger().Conditions().GetItemAt(0)->GetRowColor().ARGB);

        editor.SetDebugHighlightsEnabled(true);
        Assert::AreEqual(1, editor.Trigger().GetSelectedGroupIndex());
        Assert::AreEqual(nDefaultColor, editor.Trigger().Conditions().GetItemAt(0)->GetRowColor().ARGB);

        editor.Trigger().SetSelectedGroupIndex(2);
        Assert::AreEqual(2, editor.Trigger().GetSelectedGroupIndex());
        Assert::AreEqual(editor.mockTheme.ColorTriggerIsTrue().ARGB, editor.Trigger().Conditions().GetItemAt(0)->GetRowColor().ARGB);

        editor.Trigger().SetSelectedGroupIndex(1);
        Assert::AreEqual(1, editor.Trigger().GetSelectedGroupIndex());
        Assert::AreEqual(nDefaultColor, editor.Trigger().Conditions().GetItemAt(0)->GetRowColor().ARGB);

        editor.Trigger().SetSelectedGroupIndex(2);
        Assert::AreEqual(2, editor.Trigger().GetSelectedGroupIndex());
        Assert::AreEqual(editor.mockTheme.ColorTriggerIsTrue().ARGB, editor.Trigger().Conditions().GetItemAt(0)->GetRowColor().ARGB);

        editor.SetDebugHighlightsEnabled(false);
        Assert::AreEqual(2, editor.Trigger().GetSelectedGroupIndex());
        Assert::AreEqual(nDefaultColor, editor.Trigger().Conditions().GetItemAt(0)->GetRowColor().ARGB);

        editor.Trigger().SetSelectedGroupIndex(1);
        Assert::AreEqual(1, editor.Trigger().GetSelectedGroupIndex());
        Assert::AreEqual(nDefaultColor, editor.Trigger().Conditions().GetItemAt(0)->GetRowColor().ARGB);

        editor.Trigger().SetSelectedGroupIndex(2);
        Assert::AreEqual(2, editor.Trigger().GetSelectedGroupIndex());
        Assert::AreEqual(nDefaultColor, editor.Trigger().Conditions().GetItemAt(0)->GetRowColor().ARGB);
    }

    TEST_METHOD(TestClearOutLeaderboardSubmit)
    {
        AssetEditorViewModelHarness editor;
        editor.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [](ra::ui::viewmodels::MessageBoxViewModel&)
            {
                return DialogResult::Yes;
            });

        LeaderboardModel leaderboard;
        leaderboard.SetDefinition("STA:0x1234=1::CAN:0x1234=2::SUB:0x1234=3::VAL=0x1235");
        leaderboard.SetValueFormat(ra::data::ValueFormat::Score);
        leaderboard.CreateServerCheckpoint();
        leaderboard.CreateLocalCheckpoint();

        editor.LoadAsset(&leaderboard);
        editor.SetSelectedLeaderboardPart(AssetEditorViewModel::LeaderboardPart::Submit);

        Assert::AreEqual({ 1 }, editor.Trigger().Conditions().Count());
        editor.Trigger().SelectRange(0, 0, true);
        editor.Trigger().RemoveSelectedConditions();
        Assert::AreEqual({ 0 }, editor.Trigger().Conditions().Count());

        editor.SetSelectedLeaderboardPart(AssetEditorViewModel::LeaderboardPart::Cancel);
        Assert::AreEqual({ 1 }, editor.Trigger().Conditions().Count());

        editor.SetSelectedLeaderboardPart(AssetEditorViewModel::LeaderboardPart::Submit);
        Assert::AreEqual({ 0 }, editor.Trigger().Conditions().Count());
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
