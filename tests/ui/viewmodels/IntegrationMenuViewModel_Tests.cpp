#include "CppUnitTest.h"

#include "ui\viewmodels\IntegrationMenuViewModel.hh"

#include "RA_Resource.h"

#include "ui\viewmodels\AssetListViewModel.hh"
#include "ui\viewmodels\AssetEditorViewModel.hh"
#include "ui\viewmodels\BrokenAchievementsViewModel.hh"
#include "ui\viewmodels\GameChecksumViewModel.hh"
#include "ui\viewmodels\LoginViewModel.hh"
#include "ui\viewmodels\MemoryBookmarksViewModel.hh"
#include "ui\viewmodels\MemoryInspectorViewModel.hh"
#include "ui\viewmodels\OverlaySettingsViewModel.hh"
#include "ui\viewmodels\RichPresenceMonitorViewModel.hh"

#include "tests\ui\UIAsserts.hh"
#include "tests\mocks\MockAchievementRuntime.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockLocalStorage.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockThreadPool.hh"
#include "tests\mocks\MockUserContext.hh"
#include "tests\mocks\MockWindowManager.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(IntegrationMenuViewModel_Tests)
{
private:
    class IntegrationMenuViewModelHarness
    {
    public:
        ra::api::mocks::MockServer mockServer;
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::data::context::mocks::MockUserContext mockUserContext;
        ra::services::mocks::MockAchievementRuntime mockAchievementRuntime;
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::services::mocks::MockLocalStorage mockLocalStorage;
        ra::services::mocks::MockThreadPool mockThreadPool;
        ra::ui::mocks::MockDesktop mockDesktop;
        ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager;

        void BuildMenu()
        {
            IntegrationMenuViewModel::BuildMenu(m_vmItems);
        }

        void ActivateMenuItem(int nMenuItemId)
        {
            IntegrationMenuViewModel::ActivateMenuItem(nMenuItemId);
        }

        void AssertMenuSize(size_t nSize)
        {
            Assert::AreEqual(nSize, m_vmItems.Count());
        }

        void AssertMenuItem(gsl::index nIndex, int nId, const std::wstring& sLabel)
        {
            const auto* vmItem = m_vmItems.GetItemAt(nIndex);
            Expects(vmItem != nullptr);
            Assert::AreEqual(nId, vmItem->GetId());
            Assert::AreEqual(sLabel, vmItem->GetLabel());
        }

        void AssertMenuSeparator(gsl::index nIndex)
        {
            AssertMenuItem(nIndex, 0, L"-----");
        }

        template<typename T>
        void AssertShowWindow(int nMenuItemId, bool bHardcore, const std::string& sWarningMessage, DialogResult nConfirmResult)
        {
            mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, bHardcore);

            bool bDialogShown = false;
            mockDesktop.ExpectWindow<T>([&bDialogShown](T&)
            {
                bDialogShown = true;
                return DialogResult::None;
            });

            mockEmulatorContext.MockDisableHardcoreWarning(nConfirmResult);

            ActivateMenuItem(nMenuItemId);

            switch (nConfirmResult)
            {
                case DialogResult::None:
                    Assert::AreEqual(std::string(""), mockEmulatorContext.GetDisableHardcoreWarningMessage());
                    Assert::IsTrue(bDialogShown);
                    break;

                case DialogResult::Yes:
                    Assert::AreEqual(sWarningMessage, mockEmulatorContext.GetDisableHardcoreWarningMessage());
                    Assert::IsTrue(bDialogShown);
                    break;

                case DialogResult::No:
                    Assert::AreEqual(sWarningMessage, mockEmulatorContext.GetDisableHardcoreWarningMessage());
                    Assert::IsFalse(bDialogShown);
                    break;
            }
        }

    private:
        LookupItemViewModelCollection m_vmItems;
    };

public:
    TEST_METHOD(TestBuildMenuNotLoggedIn)
    {
        IntegrationMenuViewModelHarness menu;

        menu.BuildMenu();

        menu.AssertMenuSize(1);
        menu.AssertMenuItem(0, IDM_RA_FILES_LOGIN, L"&Login");
    }

    TEST_METHOD(TestBuildMenuLoggedIn)
    {
        IntegrationMenuViewModelHarness menu;
        menu.mockUserContext.Initialize("User", "TOKEN");

        menu.BuildMenu();

        menu.AssertMenuSize(19);
        menu.AssertMenuItem(0, IDM_RA_FILES_LOGOUT, L"Log&out");
        menu.AssertMenuSeparator(1);
        menu.AssertMenuItem(2, IDM_RA_OPENUSERPAGE, L"Open my &User Page");
        menu.AssertMenuItem(3, IDM_RA_OPENGAMEPAGE, L"Open this &Game's Page");
        menu.AssertMenuSeparator(4);
        menu.AssertMenuItem(5, IDM_RA_HARDCORE_MODE, L"&Hardcore Mode");
        menu.AssertMenuItem(6, IDM_RA_NON_HARDCORE_WARNING, L"Non-Hardcore &Warning");
        menu.AssertMenuSeparator(7);
        menu.AssertMenuItem(8, IDM_RA_TOGGLELEADERBOARDS, L"Enable &Leaderboards");
        menu.AssertMenuItem(9, IDM_RA_OVERLAYSETTINGS, L"O&verlay Settings");
        menu.AssertMenuSeparator(10);
        menu.AssertMenuItem(11, IDM_RA_FILES_ACHIEVEMENTS, L"Achievements Li&st");
        menu.AssertMenuItem(12, IDM_RA_FILES_ACHIEVEMENTEDITOR, L"Achievement &Editor");
        menu.AssertMenuItem(13, IDM_RA_FILES_MEMORYFINDER, L"&Memory Inspector");
        menu.AssertMenuItem(14, IDM_RA_FILES_MEMORYBOOKMARKS, L"Memory &Bookmarks");
        menu.AssertMenuItem(15, IDM_RA_PARSERICHPRESENCE, L"Rich &Presence Monitor");
        menu.AssertMenuSeparator(16);
        menu.AssertMenuItem(17, IDM_RA_REPORTBROKENACHIEVEMENTS, L"&Report Achievement Problem");
        menu.AssertMenuItem(18, IDM_RA_GETROMCHECKSUM, L"View Game H&ash");
    }

    TEST_METHOD(TestLoginHardcoreValidClient)
    {
        IntegrationMenuViewModelHarness menu;
        menu.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        menu.mockEmulatorContext.MockInvalidClient(false);

        bool bDialogSeen = false;
        menu.mockDesktop.ExpectWindow<ra::ui::viewmodels::LoginViewModel>([&bDialogSeen](ra::ui::viewmodels::LoginViewModel&)
        {
            bDialogSeen = true;
            return DialogResult::OK;
        });

        menu.ActivateMenuItem(IDM_RA_FILES_LOGIN);

        Assert::IsTrue(bDialogSeen);
    }

    TEST_METHOD(TestLoginHardcoreInvalidClient)
    {
        IntegrationMenuViewModelHarness menu;
        menu.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        menu.mockEmulatorContext.MockInvalidClient(true);

        bool bDialogSeen = false;
        menu.mockDesktop.ExpectWindow<ra::ui::viewmodels::LoginViewModel>([&bDialogSeen](ra::ui::viewmodels::LoginViewModel&)
        {
            bDialogSeen = true;
            return DialogResult::OK;
        });

        menu.ActivateMenuItem(IDM_RA_FILES_LOGIN);

        Assert::IsFalse(bDialogSeen);
    }

    TEST_METHOD(TestLoginNonHardcoreInvalidClient)
    {
        IntegrationMenuViewModelHarness menu;
        menu.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        menu.mockEmulatorContext.MockInvalidClient(true);

        bool bDialogSeen = false;
        menu.mockDesktop.ExpectWindow<ra::ui::viewmodels::LoginViewModel>([&bDialogSeen](ra::ui::viewmodels::LoginViewModel&)
        {
            bDialogSeen = true;
            return DialogResult::OK;
        });

        menu.ActivateMenuItem(IDM_RA_FILES_LOGIN);

        Assert::IsTrue(bDialogSeen);
    }

    TEST_METHOD(TestLogout)
    {
        IntegrationMenuViewModelHarness menu;
        menu.mockUserContext.Initialize("User", "TOKEN");
        Assert::IsTrue(menu.mockUserContext.IsLoggedIn());

        menu.ActivateMenuItem(IDM_RA_FILES_LOGOUT);

        Assert::IsFalse(menu.mockUserContext.IsLoggedIn());
    }

    TEST_METHOD(TestOpenUserPage)
    {
        IntegrationMenuViewModelHarness menu;
        menu.mockConfiguration.SetHostName("host");
        menu.mockUserContext.Initialize("User", "TOKEN");
        Assert::IsTrue(menu.mockUserContext.IsLoggedIn());

        menu.ActivateMenuItem(IDM_RA_OPENUSERPAGE);

        Assert::AreEqual(std::string("http://host/user/User"), menu.mockDesktop.LastOpenedUrl());
    }

    TEST_METHOD(TestOpenUserPageNotLoggedIn)
    {
        IntegrationMenuViewModelHarness menu;
        menu.mockConfiguration.SetHostName("host");
        Assert::IsFalse(menu.mockUserContext.IsLoggedIn());

        menu.ActivateMenuItem(IDM_RA_OPENUSERPAGE);

        Assert::AreEqual(std::string(""), menu.mockDesktop.LastOpenedUrl());
        Assert::IsFalse(menu.mockDesktop.WasDialogShown());
    }

    TEST_METHOD(TestOpenGamePage)
    {
        IntegrationMenuViewModelHarness menu;
        menu.mockConfiguration.SetHostName("host");
        menu.mockGameContext.SetGameId(1234);

        menu.ActivateMenuItem(IDM_RA_OPENGAMEPAGE);

        Assert::AreEqual(std::string("http://host/game/1234"), menu.mockDesktop.LastOpenedUrl());
    }

    TEST_METHOD(TestOpenGamePageNoGameLoaded)
    {
        IntegrationMenuViewModelHarness menu;
        menu.mockConfiguration.SetHostName("host");
        Assert::AreEqual(0U, menu.mockGameContext.GameId());

        bool bDialogShown = false;
        menu.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"No game loaded."), vmMessageBox.GetMessage());
            bDialogShown = true;
            return DialogResult::OK;
        });

        menu.ActivateMenuItem(IDM_RA_OPENGAMEPAGE);

        Assert::AreEqual(std::string(""), menu.mockDesktop.LastOpenedUrl());
        Assert::IsTrue(bDialogShown);
    }

    TEST_METHOD(TestToggleHardcoreModeEnabled)
    {
        IntegrationMenuViewModelHarness menu;
        menu.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);

        menu.ActivateMenuItem(IDM_RA_HARDCORE_MODE);

        Assert::IsTrue(menu.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
    }

    TEST_METHOD(TestToggleHardcoreModeDisabled)
    {
        IntegrationMenuViewModelHarness menu;
        menu.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);

        menu.ActivateMenuItem(IDM_RA_HARDCORE_MODE);

        Assert::IsFalse(menu.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
    }

    TEST_METHOD(TestToggleNonHardcoreWarningEnabled)
    {
        IntegrationMenuViewModelHarness menu;
        menu.mockConfiguration.SetFeatureEnabled(ra::services::Feature::NonHardcoreWarning, false);
        bool bMenuRebuilt = false;
        menu.mockEmulatorContext.SetRebuildMenuFunction([&bMenuRebuilt]() { bMenuRebuilt = true;  });

        menu.ActivateMenuItem(IDM_RA_NON_HARDCORE_WARNING);

        Assert::IsTrue(menu.mockConfiguration.IsFeatureEnabled(ra::services::Feature::NonHardcoreWarning));
        Assert::IsTrue(bMenuRebuilt);
    }

    TEST_METHOD(TestToggleNonHardcoreWarningDisabled)
    {
        IntegrationMenuViewModelHarness menu;
        menu.mockConfiguration.SetFeatureEnabled(ra::services::Feature::NonHardcoreWarning, true);
        bool bMenuRebuilt = false;
        menu.mockEmulatorContext.SetRebuildMenuFunction([&bMenuRebuilt]() { bMenuRebuilt = true;  });

        menu.ActivateMenuItem(IDM_RA_NON_HARDCORE_WARNING);

        Assert::IsFalse(menu.mockConfiguration.IsFeatureEnabled(ra::services::Feature::NonHardcoreWarning));
        Assert::IsTrue(bMenuRebuilt);
    }

    TEST_METHOD(TestToggleLeaderboardsEnabled)
    {
        IntegrationMenuViewModelHarness menu;
        menu.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Leaderboards, false);
        bool bMenuRebuilt = false;
        menu.mockEmulatorContext.SetRebuildMenuFunction([&bMenuRebuilt]() { bMenuRebuilt = true;  });

        bool bDialogShown = false;
        menu.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Leaderboards are now enabled."), vmMessageBox.GetMessage());
            bDialogShown = true;
            return DialogResult::OK;
        });

        menu.ActivateMenuItem(IDM_RA_TOGGLELEADERBOARDS);

        Assert::IsTrue(menu.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Leaderboards));
        Assert::IsTrue(bDialogShown);
        Assert::IsTrue(bMenuRebuilt);
    }

    TEST_METHOD(TestToggleLeaderboardsDisabled)
    {
        IntegrationMenuViewModelHarness menu;
        menu.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Leaderboards, true);
        bool bMenuRebuilt = false;
        menu.mockEmulatorContext.SetRebuildMenuFunction([&bMenuRebuilt]() { bMenuRebuilt = true;  });

        bool bDialogShown = false;
        menu.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Leaderboards are now disabled."), vmMessageBox.GetMessage());
            bDialogShown = true;
            return DialogResult::OK;
        });

        menu.ActivateMenuItem(IDM_RA_TOGGLELEADERBOARDS);

        Assert::IsFalse(menu.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Leaderboards));
        Assert::IsTrue(bDialogShown);
        Assert::IsTrue(bMenuRebuilt);
    }

    TEST_METHOD(TestShowOverlaySettingsOk)
    {
        IntegrationMenuViewModelHarness menu;
        menu.mockConfiguration.SetFeatureEnabled(ra::services::Feature::MasteryNotificationScreenshot, false);
        Assert::IsFalse(menu.mockConfiguration.IsFeatureEnabled(ra::services::Feature::MasteryNotificationScreenshot));

        bool bDialogShown = false;
        menu.mockDesktop.ExpectWindow<ra::ui::viewmodels::OverlaySettingsViewModel>([&bDialogShown](ra::ui::viewmodels::OverlaySettingsViewModel& vmOverlaySettings)
        {
            bDialogShown = true;
            vmOverlaySettings.SetScreenshotMastery(true);
            return DialogResult::OK;
        });

        menu.ActivateMenuItem(IDM_RA_OVERLAYSETTINGS);

        Assert::IsTrue(bDialogShown);
        Assert::IsTrue(menu.mockConfiguration.IsFeatureEnabled(ra::services::Feature::MasteryNotificationScreenshot));
    }

    TEST_METHOD(TestShowOverlaySettingsCancel)
    {
        IntegrationMenuViewModelHarness menu;
        menu.mockConfiguration.SetFeatureEnabled(ra::services::Feature::MasteryNotificationScreenshot, false);
        Assert::IsFalse(menu.mockConfiguration.IsFeatureEnabled(ra::services::Feature::MasteryNotificationScreenshot));

        bool bDialogShown = false;
        menu.mockDesktop.ExpectWindow<ra::ui::viewmodels::OverlaySettingsViewModel>([&bDialogShown](ra::ui::viewmodels::OverlaySettingsViewModel& vmOverlaySettings)
        {
            bDialogShown = true;
            vmOverlaySettings.SetScreenshotMastery(true);
            return DialogResult::Cancel;
        });

        menu.ActivateMenuItem(IDM_RA_OVERLAYSETTINGS);

        Assert::IsTrue(bDialogShown);
        Assert::IsFalse(menu.mockConfiguration.IsFeatureEnabled(ra::services::Feature::MasteryNotificationScreenshot));
    }

    TEST_METHOD(TestShowAssetListHardcore)
    {
        IntegrationMenuViewModelHarness menu;
        menu.AssertShowWindow<ra::ui::viewmodels::AssetListViewModel>(IDM_RA_FILES_ACHIEVEMENTS, true, "", DialogResult::None);
    }

    TEST_METHOD(TestShowAssetListNonHardcore)
    {
        IntegrationMenuViewModelHarness menu;
        menu.AssertShowWindow<ra::ui::viewmodels::AssetListViewModel>(IDM_RA_FILES_ACHIEVEMENTS, false, "", DialogResult::None);
    }

    TEST_METHOD(TestShowAssetEditorHardcore)
    {
        IntegrationMenuViewModelHarness menu;
        menu.AssertShowWindow<ra::ui::viewmodels::AssetEditorViewModel>(IDM_RA_FILES_ACHIEVEMENTEDITOR, true, "edit assets", DialogResult::Yes);
    }

    TEST_METHOD(TestShowAssetEditorHardcoreAbort)
    {
        IntegrationMenuViewModelHarness menu;
        menu.AssertShowWindow<ra::ui::viewmodels::AssetEditorViewModel>(IDM_RA_FILES_ACHIEVEMENTEDITOR, true, "edit assets", DialogResult::No);
    }

    TEST_METHOD(TestShowAssetEditorNonHardcore)
    {
        IntegrationMenuViewModelHarness menu;
        menu.AssertShowWindow<ra::ui::viewmodels::AssetEditorViewModel>(IDM_RA_FILES_ACHIEVEMENTEDITOR, false, "edit assets", DialogResult::None);
    }

    TEST_METHOD(TestShowMemoryInspectorHardcore)
    {
        IntegrationMenuViewModelHarness menu;
        menu.AssertShowWindow<ra::ui::viewmodels::MemoryInspectorViewModel>(IDM_RA_FILES_MEMORYFINDER, true, "inspect memory", DialogResult::Yes);
    }

    TEST_METHOD(TestShowMemoryInspectorHardcoreAbort)
    {
        IntegrationMenuViewModelHarness menu;
        menu.AssertShowWindow<ra::ui::viewmodels::MemoryInspectorViewModel>(IDM_RA_FILES_MEMORYFINDER, true, "inspect memory", DialogResult::No);
    }

    TEST_METHOD(TestShowMemoryInspectorNonHardcore)
    {
        IntegrationMenuViewModelHarness menu;
        menu.AssertShowWindow<ra::ui::viewmodels::MemoryInspectorViewModel>(IDM_RA_FILES_MEMORYFINDER, false, "inspect memory", DialogResult::None);
    }

    TEST_METHOD(TestShowMemoryBookmarksHardcore)
    {
        IntegrationMenuViewModelHarness menu;
        menu.AssertShowWindow<ra::ui::viewmodels::MemoryBookmarksViewModel>(IDM_RA_FILES_MEMORYBOOKMARKS, true, "view memory bookmarks", DialogResult::Yes);
    }

    TEST_METHOD(TestShowMemoryBookmarksHardcoreAbort)
    {
        IntegrationMenuViewModelHarness menu;
        menu.AssertShowWindow<ra::ui::viewmodels::MemoryBookmarksViewModel>(IDM_RA_FILES_MEMORYBOOKMARKS, true, "view memory bookmarks", DialogResult::No);
    }

    TEST_METHOD(TestShowMemoryBookmarksNonHardcore)
    {
        IntegrationMenuViewModelHarness menu;
        menu.AssertShowWindow<ra::ui::viewmodels::MemoryBookmarksViewModel>(IDM_RA_FILES_MEMORYBOOKMARKS, false, "view memory bookmarks", DialogResult::None);
    }

    TEST_METHOD(TestShowRichPresenceMonitorHardcore)
    {
        IntegrationMenuViewModelHarness menu;
        menu.AssertShowWindow<ra::ui::viewmodels::RichPresenceMonitorViewModel>(IDM_RA_PARSERICHPRESENCE, true, "", DialogResult::None);
    }

    TEST_METHOD(TestShowRichPresenceMonitorNonHardcore)
    {
        IntegrationMenuViewModelHarness menu;
        menu.AssertShowWindow<ra::ui::viewmodels::RichPresenceMonitorViewModel>(IDM_RA_PARSERICHPRESENCE, false, "", DialogResult::None);
    }

    TEST_METHOD(TestReportBrokenAchievements)
    {
        IntegrationMenuViewModelHarness menu;

        // at least one core achievement must exist in the filtered assets list
        // for the dialog to be shown.
        menu.mockGameContext.SetGameId(1234);
        auto& pAch = menu.mockGameContext.Assets().NewAchievement();
        pAch.SetCategory(ra::data::models::AssetCategory::Core);
        menu.mockWindowManager.AssetList.SetFilterCategory(ra::ui::viewmodels::AssetListViewModel::FilterCategory::All);

        menu.AssertShowWindow<ra::ui::viewmodels::BrokenAchievementsViewModel>(IDM_RA_REPORTBROKENACHIEVEMENTS, false, "", DialogResult::None);
    }

    TEST_METHOD(TestReportBrokenAchievementsNoGameLoaded)
    {
        IntegrationMenuViewModelHarness menu;

        bool bDialogShown = false;
        menu.mockDesktop.ExpectWindow<ra::ui::viewmodels::BrokenAchievementsViewModel>([&bDialogShown](ra::ui::viewmodels::BrokenAchievementsViewModel&)
        {
            bDialogShown = true;
            return DialogResult::None;
        });

        bool bMessageShown = false;
        menu.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bMessageShown](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"You must load a game before you can report achievement problems."), vmMessageBox.GetMessage());
            bMessageShown = true;
            return DialogResult::OK;
        });

        menu.ActivateMenuItem(IDM_RA_REPORTBROKENACHIEVEMENTS);

        Assert::IsTrue(bMessageShown);
        Assert::IsFalse(bDialogShown);
    }

    TEST_METHOD(TestShowGameHash)
    {
        IntegrationMenuViewModelHarness menu;
        menu.AssertShowWindow<ra::ui::viewmodels::GameChecksumViewModel>(IDM_RA_GETROMCHECKSUM, true, "", DialogResult::None);
    }

};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
