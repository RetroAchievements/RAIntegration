#include "IntegrationMenuViewModel.hh"

#include "RA_Log.h"
#include "RA_Resource.h"

#include "data/context/UserContext.hh"

#include "services/IConfiguration.hh"
#include "services/ServiceLocator.hh"

#include "ui/IDesktop.hh"

#include "ui/viewmodels/BrokenAchievementsViewModel.hh"
#include "ui/viewmodels/GameChecksumViewModel.hh"
#include "ui/viewmodels/LoginViewModel.hh"
#include "ui/viewmodels/MessageBoxViewModel.hh"
#include "ui/viewmodels/OverlaySettingsViewModel.hh"
#include "ui/viewmodels/WindowManager.hh"

namespace ra {
namespace ui {
namespace viewmodels {

void IntegrationMenuViewModel::BuildMenu(LookupItemViewModelCollection& vmMenu)
{
    if (ra::services::ServiceLocator::Get<ra::data::context::UserContext>().IsLoggedIn())
        BuildMenuLoggedIn(vmMenu);
    else
        BuildMenuLoggedOut(vmMenu);
}

void IntegrationMenuViewModel::BuildMenuLoggedOut(LookupItemViewModelCollection& vmMenu)
{
    vmMenu.Add(IDM_RA_FILES_LOGIN, L"&Login");
}

void IntegrationMenuViewModel::BuildMenuLoggedIn(LookupItemViewModelCollection& vmMenu)
{
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();

    vmMenu.Add(IDM_RA_FILES_LOGOUT, L"Log&out");
    vmMenu.Add(0, L"-----");
    vmMenu.Add(IDM_RA_OPENUSERPAGE, L"Open my &User Page");
    vmMenu.Add(IDM_RA_OPENGAMEPAGE, L"Open this &Game's Page");
    vmMenu.Add(0, L"-----");
    vmMenu.Add(IDM_RA_HARDCORE_MODE, L"&Hardcore Mode").SetSelected(pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
    vmMenu.Add(IDM_RA_NON_HARDCORE_WARNING, L"Non-Hardcore &Warning").SetSelected(pConfiguration.IsFeatureEnabled(ra::services::Feature::NonHardcoreWarning));
    vmMenu.Add(0, L"-----");
    vmMenu.Add(IDM_RA_TOGGLELEADERBOARDS, L"Enable &Leaderboards").SetSelected(pConfiguration.IsFeatureEnabled(ra::services::Feature::Leaderboards));
    vmMenu.Add(IDM_RA_OVERLAYSETTINGS, L"O&verlay Settings");
    vmMenu.Add(0, L"-----");
    vmMenu.Add(IDM_RA_FILES_ACHIEVEMENTS, L"Achievements Li&st");
    vmMenu.Add(IDM_RA_FILES_ACHIEVEMENTEDITOR, L"Achievement &Editor");
    vmMenu.Add(IDM_RA_FILES_MEMORYFINDER, L"&Memory Inspector");
    vmMenu.Add(IDM_RA_FILES_MEMORYBOOKMARKS, L"Memory &Bookmarks");
    vmMenu.Add(IDM_RA_PARSERICHPRESENCE, L"Rich &Presence Monitor");
    vmMenu.Add(0, L"-----");
    vmMenu.Add(IDM_RA_REPORTBROKENACHIEVEMENTS, L"&Report Achievement Problem");
    vmMenu.Add(IDM_RA_GETROMCHECKSUM, L"View Game H&ash");
}

void IntegrationMenuViewModel::ActivateMenuItem(int nMenuItemId)
{
    switch (nMenuItemId)
    {
        case IDM_RA_FILES_LOGIN:
            Login();
            break;

        case IDM_RA_FILES_LOGOUT:
            Logout();
            break;

        case IDM_RA_OPENUSERPAGE:
            OpenUserPage();
            break;

        case IDM_RA_OPENGAMEPAGE:
            OpenGamePage();
            break;

        case IDM_RA_HARDCORE_MODE:
            ToggleHardcoreMode();
            break;

        case IDM_RA_NON_HARDCORE_WARNING:
            ToggleNonHardcoreWarning();
            break;

        case IDM_RA_TOGGLELEADERBOARDS:
            ToggleLeaderboards();
            break;

        case IDM_RA_OVERLAYSETTINGS:
            ShowOverlaySettings();
            break;

        case IDM_RA_FILES_ACHIEVEMENTS:
            ShowAssetList();
            break;

        case IDM_RA_FILES_ACHIEVEMENTEDITOR:
            ShowAssetEditor();
            break;

        case IDM_RA_FILES_MEMORYFINDER:
            ShowMemoryInspector();
            break;

        case IDM_RA_FILES_MEMORYBOOKMARKS:
            ShowMemoryBookmarks();
            break;

        case IDM_RA_PARSERICHPRESENCE:
            ShowRichPresenceMonitor();
            break;

        case IDM_RA_REPORTBROKENACHIEVEMENTS:
            ReportBrokenAchievements();
            break;

        case IDM_RA_GETROMCHECKSUM:
            ShowGameHash();
            break;
    }
}

void IntegrationMenuViewModel::Login()
{
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
    {
        auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
        if (!pEmulatorContext.ValidateClientVersion())
        {
            // The version could not be validated, or the user has chosen to update. Don't login.
            return;
        }
    }

    ra::ui::viewmodels::LoginViewModel vmLogin;
    vmLogin.ShowModal();
}

void IntegrationMenuViewModel::Logout()
{
    auto& pUserContext = ra::services::ServiceLocator::GetMutable<ra::data::context::UserContext>();
    pUserContext.Logout();
}

void IntegrationMenuViewModel::OpenUserPage()
{
    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
    if (pUserContext.IsLoggedIn())
    {
        const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
        const auto sUrl = ra::StringPrintf("%s/user/%s", pConfiguration.GetHostUrl(), pUserContext.GetUsername());

        const auto& pDesktop = ra::services::ServiceLocator::Get<ra::ui::IDesktop>();
        pDesktop.OpenUrl(sUrl);
    }
}

void IntegrationMenuViewModel::OpenGamePage()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    if (pGameContext.GameId() != 0)
    {
        const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
        const auto sUrl = ra::StringPrintf("%s/game/%u", pConfiguration.GetHostUrl(), pGameContext.GameId());

        const auto& pDesktop = ra::services::ServiceLocator::Get<ra::ui::IDesktop>();
        pDesktop.OpenUrl(sUrl);
    }
    else
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"No game loaded.");
    }
}

void IntegrationMenuViewModel::ToggleHardcoreMode()
{
    auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
    {
        RA_LOG_INFO("Hardcore disabled by menu option");
        pEmulatorContext.DisableHardcoreMode();
    }
    else
    {
        pEmulatorContext.EnableHardcoreMode();
    }
}

void IntegrationMenuViewModel::ToggleNonHardcoreWarning()
{
    auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
    pConfiguration.SetFeatureEnabled(ra::services::Feature::NonHardcoreWarning,
        !pConfiguration.IsFeatureEnabled(ra::services::Feature::NonHardcoreWarning));

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    pEmulatorContext.RebuildMenu();
}

void IntegrationMenuViewModel::ToggleLeaderboards()
{
    auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
    const bool bLeaderboardsActive = !pConfiguration.IsFeatureEnabled(ra::services::Feature::Leaderboards);
    pConfiguration.SetFeatureEnabled(ra::services::Feature::Leaderboards, bLeaderboardsActive);

    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();

    if (!bLeaderboardsActive)
    {
        pGameContext.DeactivateLeaderboards();
        ra::ui::viewmodels::MessageBoxViewModel::ShowMessage(L"Leaderboards are now disabled.");
    }
    else
    {
        pGameContext.ActivateLeaderboards();
        ra::ui::viewmodels::MessageBoxViewModel::ShowMessage(L"Leaderboards are now enabled.");
    }

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    pEmulatorContext.RebuildMenu();

}

void IntegrationMenuViewModel::ShowOverlaySettings()
{
    ra::ui::viewmodels::OverlaySettingsViewModel vmSettings;
    vmSettings.Initialize();
    if (vmSettings.ShowModal() == ra::ui::DialogResult::OK)
        vmSettings.Commit();
}

void IntegrationMenuViewModel::ShowAssetList()
{
    auto& pWindowManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>();
    pWindowManager.AssetList.Show();
}

void IntegrationMenuViewModel::ShowAssetEditor()
{
    auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
    if (pEmulatorContext.WarnDisableHardcoreMode("edit assets"))
    {
        auto& pWindowManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>();
        pWindowManager.AssetEditor.Show();
    }
}

void IntegrationMenuViewModel::ShowMemoryInspector()
{
    auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
    if (pEmulatorContext.WarnDisableHardcoreMode("inspect memory"))
    {
        auto& pWindowManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>();
        pWindowManager.MemoryInspector.Show();
    }
}

void IntegrationMenuViewModel::ShowMemoryBookmarks()
{
    auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
    if (pEmulatorContext.WarnDisableHardcoreMode("view memory bookmarks"))
    {
        auto& pWindowManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>();
        pWindowManager.MemoryBookmarks.Show();
    }
}

void IntegrationMenuViewModel::ShowRichPresenceMonitor()
{
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    pGameContext.ReloadRichPresenceScript();

    auto& pWindowManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>();
    pWindowManager.RichPresenceMonitor.Show();
}

void IntegrationMenuViewModel::ReportBrokenAchievements()
{
    ra::ui::viewmodels::BrokenAchievementsViewModel vmBrokenAchievements;
    if (vmBrokenAchievements.InitializeAchievements())
        vmBrokenAchievements.ShowModal();
}

void IntegrationMenuViewModel::ShowGameHash()
{
    ra::ui::viewmodels::GameChecksumViewModel vmGameChecksum;
    vmGameChecksum.ShowModal();
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
