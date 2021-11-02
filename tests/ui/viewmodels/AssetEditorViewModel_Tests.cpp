#include "CppUnitTest.h"

#include "ui\EditorTheme.hh"

#include "ui\viewmodels\AssetEditorViewModel.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\data\DataAsserts.hh"
#include "tests\ui\viewmodels\TriggerConditionAsserts.hh"

#include "tests\mocks\MockAchievementRuntime.hh"
#include "tests\mocks\MockClock.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockDesktop.hh"

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
        ra::ui::mocks::MockDesktop mockDesktop;
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

    private:
        ra::services::ServiceLocator::ServiceOverride<ra::ui::EditorTheme> m_pMockThemeOverride;
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
        Assert::AreEqual((int)AssetEditorViewModel::LeaderboardPart::Start, (int)editor.GetSelectedLeaderboardPart());
        Assert::AreEqual(ra::data::ValueFormat::Value, editor.GetValueFormat());

        Assert::AreEqual({ 6U }, editor.Formats().Count());
        Assert::AreEqual((int)ra::data::ValueFormat::Value, editor.Formats().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"Value"), editor.Formats().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)ra::data::ValueFormat::Score, editor.Formats().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"Score"), editor.Formats().GetItemAt(1)->GetLabel());
        Assert::AreEqual((int)ra::data::ValueFormat::Frames, editor.Formats().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"Frames"), editor.Formats().GetItemAt(2)->GetLabel());
        Assert::AreEqual((int)ra::data::ValueFormat::Centiseconds, editor.Formats().GetItemAt(3)->GetId());
        Assert::AreEqual(std::wstring(L"Centiseconds"), editor.Formats().GetItemAt(3)->GetLabel());
        Assert::AreEqual((int)ra::data::ValueFormat::Seconds, editor.Formats().GetItemAt(4)->GetId());
        Assert::AreEqual(std::wstring(L"Seconds"), editor.Formats().GetItemAt(4)->GetLabel());
        Assert::AreEqual((int)ra::data::ValueFormat::Minutes, editor.Formats().GetItemAt(5)->GetId());
        Assert::AreEqual(std::wstring(L"Minutes"), editor.Formats().GetItemAt(5)->GetLabel());

        Assert::AreEqual({ 4U }, editor.LeaderboardParts().Count());
        Assert::AreEqual((int)AssetEditorViewModel::LeaderboardPart::Start, editor.LeaderboardParts().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"Start"), editor.LeaderboardParts().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)AssetEditorViewModel::LeaderboardPart::Submit, editor.LeaderboardParts().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"Submit"), editor.LeaderboardParts().GetItemAt(1)->GetLabel());
        Assert::AreEqual((int)AssetEditorViewModel::LeaderboardPart::Cancel, editor.LeaderboardParts().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"Cancel"), editor.LeaderboardParts().GetItemAt(2)->GetLabel());
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
        achievement.SetTrigger("A:0xH1234"); // AddSource cannot be final condition
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();

        editor.LoadAsset(&achievement);

        Assert::IsFalse(editor.HasAssetValidationError());
        Assert::AreEqual(std::wstring(), editor.GetAssetValidationError());

        Assert::IsTrue(editor.HasAssetValidationWarning());
        Assert::AreEqual(std::wstring(L"Final condition type expects another condition to follow"), editor.GetAssetValidationWarning());

        editor.LoadAsset(nullptr);
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
        Assert::AreEqual(std::wstring(L"Final condition type expects another condition to follow"), editor.GetAssetValidationWarning());
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

        achievement.SetState(AssetState::Primed);
        Assert::AreEqual(AssetState::Primed, editor.GetState());
        Assert::AreEqual(AssetState::Primed, achievement.GetState());
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

    TEST_METHOD(TestSyncStateMeasured)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        achievement.SetTrigger("M:0xH1234=6.11.");
        achievement.SetState(AssetState::Inactive);
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();
        editor.LoadAsset(&achievement);

        Assert::IsTrue(editor.HasMeasured());
        Assert::AreEqual(std::wstring(L"[Not Active]"), editor.GetMeasuredValue());

        achievement.SetState(AssetState::Waiting);
        Assert::AreEqual(std::wstring(L"0/11"), editor.GetMeasuredValue());

        achievement.SetState(AssetState::Active);
        Assert::AreEqual(std::wstring(L"0/11"), editor.GetMeasuredValue());

        achievement.SetState(AssetState::Triggered);
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
        Assert::AreEqual(std::wstring(L"0x1234"), pCondition->GetSourceValue());
        Assert::AreEqual((int)TriggerOperatorType::Equals, (int)pCondition->GetOperator());
        Assert::AreEqual(MemSize::ThirtyTwoBit, pCondition->GetTargetSize());
        Assert::AreEqual((int)TriggerOperandType::Value, (int)pCondition->GetTargetType());
        Assert::AreEqual(std::wstring(L"1"), pCondition->GetTargetValue());

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
        Assert::AreEqual(std::wstring(L"0x1234"), pCondition->GetSourceValue());
        Assert::AreEqual((int)TriggerOperatorType::Equals, (int)pCondition->GetOperator());
        Assert::AreEqual(MemSize::ThirtyTwoBit, pCondition->GetTargetSize());
        Assert::AreEqual((int)TriggerOperandType::Value, (int)pCondition->GetTargetType());
        Assert::AreEqual(std::wstring(L"2"), pCondition->GetTargetValue());
    }

    TEST_METHOD(TestUpdateInactiveTrigger)
    {
        AssetEditorViewModelHarness editor;
        editor.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
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
        Assert::AreEqual(std::wstring(L"0x1234"), pCondition->GetSourceValue());
        Assert::AreEqual((int)TriggerOperatorType::Equals, (int)pCondition->GetOperator());
        Assert::AreEqual(MemSize::ThirtyTwoBit, pCondition->GetTargetSize());
        Assert::AreEqual((int)TriggerOperandType::Value, (int)pCondition->GetTargetType());
        Assert::AreEqual(std::wstring(L"1"), pCondition->GetTargetValue());

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
        Assert::AreEqual(std::wstring(L"0x1234"), pCondition->GetSourceValue());
        Assert::AreEqual((int)TriggerOperatorType::Equals, (int)pCondition->GetOperator());
        Assert::AreEqual(MemSize::ThirtyTwoBit, pCondition->GetTargetSize());
        Assert::AreEqual((int)TriggerOperandType::Value, (int)pCondition->GetTargetType());
        Assert::AreEqual(std::wstring(L"2"), pCondition->GetTargetValue());
    }

    TEST_METHOD(TestTriggerUpdated)
    {
        AssetEditorViewModelHarness editor;
        editor.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
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
        Assert::AreEqual(std::wstring(L"0x1234"), pCondition->GetSourceValue());
        Assert::AreEqual((int)TriggerOperatorType::Equals, (int)pCondition->GetOperator());
        Assert::AreEqual(MemSize::ThirtyTwoBit, pCondition->GetTargetSize());
        Assert::AreEqual((int)TriggerOperandType::Value, (int)pCondition->GetTargetType());
        Assert::AreEqual(std::wstring(L"1"), pCondition->GetTargetValue());

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
        Assert::AreEqual(std::wstring(L"0x1234"), pCondition->GetSourceValue());
        Assert::AreEqual((int)TriggerOperatorType::Equals, (int)pCondition->GetOperator());
        Assert::AreEqual(MemSize::ThirtyTwoBit, pCondition->GetTargetSize());
        Assert::AreEqual((int)TriggerOperandType::Value, (int)pCondition->GetTargetType());
        Assert::AreEqual(std::wstring(L"2"), pCondition->GetTargetValue());
    }

    TEST_METHOD(TestTriggerUpdatedInvalid)
    {
        AssetEditorViewModelHarness editor;
        editor.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
        AchievementModel achievement;
        achievement.SetID(1234U);
        achievement.SetTrigger("M:0xH1234=1.1._0xH2345=1.2.");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();
        achievement.Activate();

        editor.LoadAsset(&achievement);

        // make sure the record got loaded into the runtime
        const auto* pTrigger = editor.mockRuntime.GetAchievementTrigger(1234U);
        Expects(pTrigger != nullptr);

        Assert::AreEqual({ 2U }, editor.Trigger().Conditions().Count());
        editor.Trigger().Conditions().GetItemAt(1)->SetType(ra::ui::viewmodels::TriggerConditionType::Measured);

        // make sure the trigger definition got updated
        Assert::AreEqual(std::string("M:0xH1234=1.1._M:0xH2345=1.2."), achievement.GetTrigger());

        // make sure the error got set
        Assert::AreEqual(std::wstring(L"Multiple measured targets"), editor.GetAssetValidationError());
        Assert::IsTrue(editor.HasAssetValidationError());

        // make sure the achievement was disabled
        Assert::AreEqual(AssetState::Inactive, achievement.GetState());
        pTrigger = editor.mockRuntime.GetAchievementTrigger(1234U);
        Assert::IsNull(pTrigger);

        // make sure the UI kept the change
        Assert::AreEqual({ 2U }, editor.Trigger().Conditions().Count());
        auto* pCondition = editor.Trigger().Conditions().GetItemAt(1);
        Expects(pCondition != nullptr);
        Assert::AreEqual(TriggerConditionType::Measured, pCondition->GetType());

        // fix the error
        editor.Trigger().Conditions().GetItemAt(0)->SetType(ra::ui::viewmodels::TriggerConditionType::Standard);

        // make sure the trigger definition got updated
        Assert::AreEqual(std::string("0xH1234=1.1._M:0xH2345=1.2."), achievement.GetTrigger());

        // make sure the error got cleared
        Assert::AreEqual(std::wstring(L""), editor.GetAssetValidationError());
        Assert::IsFalse(editor.HasAssetValidationError());

        // the achievement should have remained disabled
        Assert::AreEqual(AssetState::Inactive, achievement.GetState());
        pTrigger = editor.mockRuntime.GetAchievementTrigger(1234U);
        Assert::IsNull(pTrigger);

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
        AchievementModel achievement;
        achievement.SetID(1234U);
        achievement.SetTrigger("0=0SM:0xH1234=1.1.S0xH2345=1.2.");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();
        achievement.Activate();

        editor.LoadAsset(&achievement);

        // make sure the record got loaded into the runtime
        const auto* pTrigger = editor.mockRuntime.GetAchievementTrigger(1234U);
        Expects(pTrigger != nullptr);

        editor.Trigger().SetSelectedGroupIndex(2);
        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        editor.Trigger().Conditions().GetItemAt(0)->SetType(ra::ui::viewmodels::TriggerConditionType::Measured);

        // make sure the trigger definition got updated
        Assert::AreEqual(std::string("0=0SM:0xH1234=1.1.SM:0xH2345=1.2."), achievement.GetTrigger());

        // make sure the error got set
        Assert::AreEqual(std::wstring(L"Multiple measured targets"), editor.GetAssetValidationError());
        Assert::IsTrue(editor.HasAssetValidationError());

        // make sure the achievement was disabled
        Assert::AreEqual(AssetState::Inactive, achievement.GetState());
        pTrigger = editor.mockRuntime.GetAchievementTrigger(1234U);
        Assert::IsNull(pTrigger);

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
        Assert::AreEqual(std::string("0=0SM:0xH1234=1.2.SM:0xH2345=1.2."), achievement.GetTrigger());

        // make sure the error got cleared
        Assert::AreEqual(std::wstring(L""), editor.GetAssetValidationError());
        Assert::IsFalse(editor.HasAssetValidationError());

        // the achievement should have remained disabled
        Assert::AreEqual(AssetState::Inactive, achievement.GetState());
        pTrigger = editor.mockRuntime.GetAchievementTrigger(1234U);
        Assert::IsNull(pTrigger);

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

    TEST_METHOD(TestValueUpdated)
    {
        AssetEditorViewModelHarness editor;
        LeaderboardModel leaderboard;
        leaderboard.SetID(1234U);
        leaderboard.SetStartTrigger("0x8000=0");
        leaderboard.SetSubmitTrigger("0x8000=1");
        leaderboard.SetCancelTrigger("0x8000=2");
        leaderboard.SetValueDefinition("M:0xH1234");
        leaderboard.CreateServerCheckpoint();
        leaderboard.CreateLocalCheckpoint();
        leaderboard.Activate();

        editor.LoadAsset(&leaderboard);
        editor.SetSelectedLeaderboardPart(AssetEditorViewModel::LeaderboardPart::Value);

        // make sure the record got loaded into the runtime
        const auto* pLeaderboard = editor.mockRuntime.GetLeaderboardDefinition(1234U);
        Expects(pLeaderboard != nullptr);

        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        auto* pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(MemSize::EightBit, pCondition->GetSourceSize());
        Assert::AreEqual((int)TriggerOperandType::Address, (int)pCondition->GetSourceType());
        Assert::AreEqual(std::wstring(L"0x1234"), pCondition->GetSourceValue());
        Assert::AreEqual((int)TriggerOperatorType::None, (int)pCondition->GetOperator());

        leaderboard.SetValueDefinition("M:0xH2222");

        // make sure the value definition got updated
        Assert::AreEqual(std::string("M:0xH2222"), leaderboard.GetValueDefinition());

        // make sure the runtime record got updated
        pLeaderboard = editor.mockRuntime.GetLeaderboardDefinition(1234U);
        Expects(pLeaderboard != nullptr);
        Assert::AreEqual(0x2222U, pLeaderboard->value.conditions->conditions->operand1.value.memref->address);

        // make sure the UI picked up the updated value
        Assert::AreEqual({ 1U }, editor.Trigger().Conditions().Count());
        pCondition = editor.Trigger().Conditions().GetItemAt(0);
        Expects(pCondition != nullptr);
        Assert::AreEqual(MemSize::EightBit, pCondition->GetSourceSize());
        Assert::AreEqual((int)TriggerOperandType::Address, (int)pCondition->GetSourceType());
        Assert::AreEqual(std::wstring(L"0x2222"), pCondition->GetSourceValue());
        Assert::AreEqual((int)TriggerOperatorType::None, (int)pCondition->GetOperator());

        // modify the UI
        pCondition->SetSourceValue(0x3333U);

        // make sure the value definition got updated
        Assert::AreEqual(std::string("M:0xH3333"), leaderboard.GetValueDefinition());

        // make sure the runtime record got updated
        pLeaderboard = editor.mockRuntime.GetLeaderboardDefinition(1234U);
        Expects(pLeaderboard != nullptr);
        Assert::AreEqual(0x3333U, pLeaderboard->value.conditions->conditions->operand1.value.memref->address);
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

    TEST_METHOD(TestDoFrameUpdatesMeasuredFromActiveAchievement)
    {
        AssetEditorViewModelHarness editor;
        AchievementModel achievement;
        achievement.SetID(1234U);
        achievement.SetTrigger("M:0xH1234=1.99.");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();
        achievement.Activate();

        editor.LoadAsset(&achievement);

        auto* pTrigger = editor.mockRuntime.GetAchievementTrigger(1234U);
        Expects(pTrigger != nullptr);
        pTrigger->measured_value = 6U;
        Assert::AreEqual(std::wstring(L"0/99"), editor.GetMeasuredValue());

        editor.DoFrame();
        Assert::AreEqual(std::wstring(L"6/99"), editor.GetMeasuredValue());
    }

    TEST_METHOD(TestDoFrameUpdatesMeasuredFromActiveLeaderboard)
    {
        AssetEditorViewModelHarness editor;
        LeaderboardModel leaderboard;
        leaderboard.SetID(1234U);
        leaderboard.SetStartTrigger("1=1");
        leaderboard.SetSubmitTrigger("1=0");
        leaderboard.SetCancelTrigger("0=1");
        leaderboard.SetValueDefinition("M:0xH1234");
        leaderboard.SetValueFormat(ra::data::ValueFormat::Centiseconds);
        leaderboard.CreateServerCheckpoint();
        leaderboard.CreateLocalCheckpoint();
        leaderboard.Activate();

        editor.LoadAsset(&leaderboard);

        auto* pDefinition = editor.mockRuntime.GetLeaderboardDefinition(1234U);
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
        Assert::IsFalse(leaderboard.IsModified());

        editor.SetSelectedLeaderboardPart(AssetEditorViewModel::LeaderboardPart::Submit);
        Assert::AreEqual(std::string("0xH2345!=99"), editor.Trigger().Serialize());
        Assert::AreEqual(std::wstring(L"Groups:"), editor.GetGroupsHeaderLabel());
        Assert::IsFalse(editor.Trigger().IsValue());
        Assert::IsFalse(leaderboard.IsModified());

        editor.SetSelectedLeaderboardPart(AssetEditorViewModel::LeaderboardPart::Cancel);
        Assert::AreEqual(std::string("0xH3456>3"), editor.Trigger().Serialize());
        Assert::AreEqual(std::wstring(L"Groups:"), editor.GetGroupsHeaderLabel());
        Assert::IsFalse(editor.Trigger().IsValue());
        Assert::IsFalse(leaderboard.IsModified());

        editor.SetSelectedLeaderboardPart(AssetEditorViewModel::LeaderboardPart::Value);
        Assert::AreEqual(std::string("M:0xH4444"), editor.Trigger().Serialize());
        Assert::AreEqual(std::wstring(L"Max of:"), editor.GetGroupsHeaderLabel());
        Assert::IsTrue(editor.Trigger().IsValue());
        Assert::IsFalse(leaderboard.IsModified());

        editor.SetSelectedLeaderboardPart(AssetEditorViewModel::LeaderboardPart::Start);
        Assert::AreEqual(std::string("0xH1234=6.1."), editor.Trigger().Serialize());
        Assert::AreEqual(std::wstring(L"Groups:"), editor.GetGroupsHeaderLabel());
        Assert::IsFalse(editor.Trigger().IsValue());
        Assert::IsFalse(leaderboard.IsModified());
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
