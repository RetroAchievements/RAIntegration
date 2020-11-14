#include "CppUnitTest.h"

#include "ui\viewmodels\OverlaySettingsViewModel.hh"

#include "ui\viewmodels\FileDialogViewModel.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockDesktop.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

template<>
std::wstring ToString<ra::ui::viewmodels::PopupLocation>(
    const ra::ui::viewmodels::PopupLocation& nPopupLocation)
{
    switch (nPopupLocation)
    {
        case ra::ui::viewmodels::PopupLocation::None:
            return L"None";
        case ra::ui::viewmodels::PopupLocation::TopLeft:
            return L"TopLeft";
        case ra::ui::viewmodels::PopupLocation::TopMiddle:
            return L"TopMiddle";
        case ra::ui::viewmodels::PopupLocation::TopRight:
            return L"TopRight";
        case ra::ui::viewmodels::PopupLocation::BottomLeft:
            return L"BottomLeft";
        case ra::ui::viewmodels::PopupLocation::BottomMiddle:
            return L"BottomMiddle";
        case ra::ui::viewmodels::PopupLocation::BottomRight:
            return L"BottomRight";
        default:
            return std::to_wstring(static_cast<int>(nPopupLocation));
    }
}

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(OverlaySettingsViewModel_Tests)
{
private:
    class OverlaySettingsViewModelHarness : public OverlaySettingsViewModel
    {
    public:
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::ui::mocks::MockDesktop mockDesktop;
    };

    void ValidatePopupLocationInitialize(ra::ui::viewmodels::Popup nPopup, std::function<ra::ui::viewmodels::PopupLocation(OverlaySettingsViewModel&)> fGetValue)
    {
        OverlaySettingsViewModelHarness vmSettings;
        vmSettings.mockConfiguration.SetPopupLocation(nPopup, ra::ui::viewmodels::PopupLocation::None);
        vmSettings.Initialize();
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::None, fGetValue(vmSettings));

        vmSettings.mockConfiguration.SetPopupLocation(nPopup, ra::ui::viewmodels::PopupLocation::TopLeft);
        vmSettings.Initialize();
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::TopLeft, fGetValue(vmSettings));

        vmSettings.mockConfiguration.SetPopupLocation(nPopup, ra::ui::viewmodels::PopupLocation::TopMiddle);
        vmSettings.Initialize();
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::TopMiddle, fGetValue(vmSettings));

        vmSettings.mockConfiguration.SetPopupLocation(nPopup, ra::ui::viewmodels::PopupLocation::TopRight);
        vmSettings.Initialize();
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::TopRight, fGetValue(vmSettings));

        vmSettings.mockConfiguration.SetPopupLocation(nPopup, ra::ui::viewmodels::PopupLocation::BottomLeft);
        vmSettings.Initialize();
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::BottomLeft, fGetValue(vmSettings));

        vmSettings.mockConfiguration.SetPopupLocation(nPopup, ra::ui::viewmodels::PopupLocation::BottomMiddle);
        vmSettings.Initialize();
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::BottomMiddle, fGetValue(vmSettings));

        vmSettings.mockConfiguration.SetPopupLocation(nPopup, ra::ui::viewmodels::PopupLocation::BottomRight);
        vmSettings.Initialize();
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::BottomRight, fGetValue(vmSettings));
    }

    void ValidatePopupLocationCommit(ra::ui::viewmodels::Popup nPopup, std::function<void(OverlaySettingsViewModel&, ra::ui::viewmodels::PopupLocation)> fSetValue)
    {
        OverlaySettingsViewModelHarness vmSettings;
        fSetValue(vmSettings, ra::ui::viewmodels::PopupLocation::None);
        vmSettings.Commit();
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::None, vmSettings.mockConfiguration.GetPopupLocation(nPopup));

        fSetValue(vmSettings, ra::ui::viewmodels::PopupLocation::TopLeft);
        vmSettings.Commit();
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::TopLeft, vmSettings.mockConfiguration.GetPopupLocation(nPopup));

        fSetValue(vmSettings, ra::ui::viewmodels::PopupLocation::TopMiddle);
        vmSettings.Commit();
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::TopMiddle, vmSettings.mockConfiguration.GetPopupLocation(nPopup));

        fSetValue(vmSettings, ra::ui::viewmodels::PopupLocation::TopRight);
        vmSettings.Commit();
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::TopRight, vmSettings.mockConfiguration.GetPopupLocation(nPopup));

        fSetValue(vmSettings, ra::ui::viewmodels::PopupLocation::BottomLeft);
        vmSettings.Commit();
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::BottomLeft, vmSettings.mockConfiguration.GetPopupLocation(nPopup));

        fSetValue(vmSettings, ra::ui::viewmodels::PopupLocation::BottomMiddle);
        vmSettings.Commit();
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::BottomMiddle, vmSettings.mockConfiguration.GetPopupLocation(nPopup));

        fSetValue(vmSettings, ra::ui::viewmodels::PopupLocation::BottomRight);
        vmSettings.Commit();
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::BottomRight, vmSettings.mockConfiguration.GetPopupLocation(nPopup));
    }

    void ValidateFeatureInitialize(ra::services::Feature feature, std::function<bool(OverlaySettingsViewModel&)> fGetValue)
    {
        OverlaySettingsViewModelHarness vmSettings;
        vmSettings.mockConfiguration.SetFeatureEnabled(feature, false);
        vmSettings.Initialize();
        Assert::IsFalse(fGetValue(vmSettings));

        vmSettings.mockConfiguration.SetFeatureEnabled(feature, true);
        vmSettings.Initialize();
        Assert::IsTrue(fGetValue(vmSettings));
    }

    void ValidateFeatureCommit(ra::services::Feature feature, std::function<void(OverlaySettingsViewModel&, bool)> fSetValue)
    {
        OverlaySettingsViewModelHarness vmSettings;
        fSetValue(vmSettings, false);
        vmSettings.Commit();
        Assert::IsFalse(vmSettings.mockConfiguration.IsFeatureEnabled(feature));

        fSetValue(vmSettings, true);
        vmSettings.Commit();
        Assert::IsTrue(vmSettings.mockConfiguration.IsFeatureEnabled(feature));
    }

public:
    TEST_METHOD(TestInitialize)
    {
        ValidatePopupLocationInitialize(ra::ui::viewmodels::Popup::AchievementTriggered, [](OverlaySettingsViewModel& vm) { return vm.GetAchievementTriggerLocation(); });
        ValidateFeatureInitialize(ra::services::Feature::AchievementTriggeredScreenshot, [](OverlaySettingsViewModel& vm) { return vm.ScreenshotAchievementTrigger(); });
        ValidatePopupLocationInitialize(ra::ui::viewmodels::Popup::Mastery, [](OverlaySettingsViewModel& vm) { return vm.GetMasteryLocation(); });
        ValidateFeatureInitialize(ra::services::Feature::MasteryNotificationScreenshot, [](OverlaySettingsViewModel& vm) { return vm.ScreenshotMastery(); });
        ValidatePopupLocationInitialize(ra::ui::viewmodels::Popup::LeaderboardStarted, [](OverlaySettingsViewModel& vm) { return vm.GetLeaderboardStartedLocation(); });
        ValidatePopupLocationInitialize(ra::ui::viewmodels::Popup::LeaderboardCanceled, [](OverlaySettingsViewModel& vm) { return vm.GetLeaderboardCanceledLocation(); });
        ValidatePopupLocationInitialize(ra::ui::viewmodels::Popup::LeaderboardTracker, [](OverlaySettingsViewModel& vm) { return vm.GetLeaderboardTrackerLocation(); });
        ValidatePopupLocationInitialize(ra::ui::viewmodels::Popup::LeaderboardScoreboard, [](OverlaySettingsViewModel& vm) { return vm.GetLeaderboardScoreboardLocation(); });

        OverlaySettingsViewModelHarness vmSettings;
        vmSettings.mockConfiguration.SetScreenshotDirectory(L"C:\\Screenshots\\");
        vmSettings.Initialize();
        Assert::AreEqual(std::wstring(L"C:\\Screenshots\\"), vmSettings.ScreenshotLocation());
    }

    TEST_METHOD(TestCommit)
    {
        ValidatePopupLocationCommit(ra::ui::viewmodels::Popup::AchievementTriggered, [](OverlaySettingsViewModel& vm, ra::ui::viewmodels::PopupLocation nValue) { return vm.SetAchievementTriggerLocation(nValue); });
        ValidateFeatureCommit(ra::services::Feature::AchievementTriggeredScreenshot, [](OverlaySettingsViewModel& vm, bool bValue) { return vm.SetScreenshotAchievementTrigger(bValue); });
        ValidatePopupLocationCommit(ra::ui::viewmodels::Popup::Mastery, [](OverlaySettingsViewModel& vm, ra::ui::viewmodels::PopupLocation nValue) { return vm.SetMasteryLocation(nValue); });
        ValidateFeatureCommit(ra::services::Feature::MasteryNotificationScreenshot, [](OverlaySettingsViewModel& vm, bool bValue) { return vm.SetScreenshotMastery(bValue); });
        ValidatePopupLocationCommit(ra::ui::viewmodels::Popup::LeaderboardStarted, [](OverlaySettingsViewModel& vm, ra::ui::viewmodels::PopupLocation nValue) { return vm.SetLeaderboardStartedLocation(nValue); });
        ValidatePopupLocationCommit(ra::ui::viewmodels::Popup::LeaderboardCanceled, [](OverlaySettingsViewModel& vm, ra::ui::viewmodels::PopupLocation nValue) { return vm.SetLeaderboardCanceledLocation(nValue); });
        ValidatePopupLocationCommit(ra::ui::viewmodels::Popup::LeaderboardTracker, [](OverlaySettingsViewModel& vm, ra::ui::viewmodels::PopupLocation nValue) { return vm.SetLeaderboardTrackerLocation(nValue); });
        ValidatePopupLocationCommit(ra::ui::viewmodels::Popup::LeaderboardScoreboard, [](OverlaySettingsViewModel& vm, ra::ui::viewmodels::PopupLocation nValue) { return vm.SetLeaderboardScoreboardLocation(nValue); });

        OverlaySettingsViewModelHarness vmSettings;
        vmSettings.SetScreenshotLocation(L"C:\\Screenshots\\");
        vmSettings.Commit();
        Assert::AreEqual(std::wstring(L"C:\\Screenshots\\"), vmSettings.mockConfiguration.GetScreenshotDirectory());

        // make sure the path ends with a slash
        vmSettings.SetScreenshotLocation(L"C:\\Temp");
        vmSettings.Commit();
        Assert::AreEqual(std::wstring(L"C:\\Temp\\"), vmSettings.mockConfiguration.GetScreenshotDirectory());
    }

    TEST_METHOD(TestAchievementTriggerDependencies)
    {
        OverlaySettingsViewModelHarness vmSettings;
        vmSettings.SetAchievementTriggerLocation(ra::ui::viewmodels::PopupLocation::None);
        vmSettings.SetScreenshotAchievementTrigger(false);
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::None, vmSettings.GetAchievementTriggerLocation());
        Assert::IsFalse(vmSettings.ScreenshotAchievementTrigger());

        vmSettings.SetAchievementTriggerLocation(ra::ui::viewmodels::PopupLocation::BottomLeft);
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::BottomLeft, vmSettings.GetAchievementTriggerLocation());
        Assert::IsFalse(vmSettings.ScreenshotAchievementTrigger());

        vmSettings.SetScreenshotAchievementTrigger(true);
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::BottomLeft, vmSettings.GetAchievementTriggerLocation());
        Assert::IsTrue(vmSettings.ScreenshotAchievementTrigger());

        vmSettings.SetAchievementTriggerLocation(ra::ui::viewmodels::PopupLocation::None);
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::None, vmSettings.GetAchievementTriggerLocation());
        Assert::IsFalse(vmSettings.ScreenshotAchievementTrigger());

        vmSettings.SetScreenshotAchievementTrigger(true);
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::BottomLeft, vmSettings.GetAchievementTriggerLocation());
        Assert::IsTrue(vmSettings.ScreenshotAchievementTrigger());

        vmSettings.SetScreenshotAchievementTrigger(false);
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::BottomLeft, vmSettings.GetAchievementTriggerLocation());
        Assert::IsFalse(vmSettings.ScreenshotAchievementTrigger());

        vmSettings.SetAchievementTriggerLocation(ra::ui::viewmodels::PopupLocation::None);
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::None, vmSettings.GetAchievementTriggerLocation());
        Assert::IsFalse(vmSettings.ScreenshotAchievementTrigger());
    }

    TEST_METHOD(TestMasteryTriggerDependencies)
    {
        OverlaySettingsViewModelHarness vmSettings;
        vmSettings.SetMasteryLocation(ra::ui::viewmodels::PopupLocation::None);
        vmSettings.SetScreenshotMastery(false);
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::None, vmSettings.GetMasteryLocation());
        Assert::IsFalse(vmSettings.ScreenshotMastery());

        vmSettings.SetMasteryLocation(ra::ui::viewmodels::PopupLocation::BottomLeft);
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::BottomLeft, vmSettings.GetMasteryLocation());
        Assert::IsFalse(vmSettings.ScreenshotMastery());

        vmSettings.SetScreenshotMastery(true);
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::BottomLeft, vmSettings.GetMasteryLocation());
        Assert::IsTrue(vmSettings.ScreenshotMastery());

        vmSettings.SetMasteryLocation(ra::ui::viewmodels::PopupLocation::None);
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::None, vmSettings.GetMasteryLocation());
        Assert::IsFalse(vmSettings.ScreenshotMastery());

        vmSettings.SetScreenshotMastery(true);
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::TopMiddle, vmSettings.GetMasteryLocation());
        Assert::IsTrue(vmSettings.ScreenshotMastery());

        vmSettings.SetScreenshotMastery(false);
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::TopMiddle, vmSettings.GetMasteryLocation());
        Assert::IsFalse(vmSettings.ScreenshotMastery());

        vmSettings.SetMasteryLocation(ra::ui::viewmodels::PopupLocation::None);
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::None, vmSettings.GetMasteryLocation());
        Assert::IsFalse(vmSettings.ScreenshotMastery());
    }

    TEST_METHOD(TestBrowseLocation)
    {
        OverlaySettingsViewModelHarness vmSettings;
        vmSettings.SetScreenshotLocation(L"C:\\Screenshots\\");

        vmSettings.mockDesktop.ExpectWindow<ra::ui::viewmodels::FileDialogViewModel>([](ra::ui::viewmodels::FileDialogViewModel& vmFileDialog)
        {
            Assert::AreEqual(std::wstring(L"C:\\Screenshots\\"), vmFileDialog.GetInitialDirectory());
            vmFileDialog.SetFileName(L"C:\\NewFolder");
            return ra::ui::DialogResult::OK;
        });

        vmSettings.BrowseLocation();

        Assert::AreEqual(std::wstring(L"C:\\NewFolder\\"), vmSettings.ScreenshotLocation());
    }

    TEST_METHOD(TestBrowseLocationCancel)
    {
        OverlaySettingsViewModelHarness vmSettings;
        vmSettings.SetScreenshotLocation(L"C:\\Screenshots\\");

        vmSettings.mockDesktop.ExpectWindow<ra::ui::viewmodels::FileDialogViewModel>([](ra::ui::viewmodels::FileDialogViewModel& vmFileDialog)
        {
            Assert::AreEqual(std::wstring(L"C:\\Screenshots\\"), vmFileDialog.GetInitialDirectory());
            vmFileDialog.SetFileName(L"C:\\NewFolder");
            return ra::ui::DialogResult::Cancel;
        });

        vmSettings.BrowseLocation();

        Assert::AreEqual(std::wstring(L"C:\\Screenshots\\"), vmSettings.ScreenshotLocation());
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
