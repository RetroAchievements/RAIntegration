#include "RA_Core.h"

#include "RA_BuildVer.h"
#include "RA_ImageFactory.h"
#include "RA_Resource.h"
#include "RA_md5factory.h"

#include "RA_Dlg_AchEditor.h"   // RA_httpthread.h, services/ImageRepository.h
#include "RA_Dlg_Achievement.h" // RA_AchievementSet.h
#include "RA_Dlg_GameLibrary.h"
#include "RA_Dlg_Memory.h"

#include "data\ConsoleContext.hh"
#include "data\EmulatorContext.hh"
#include "data\GameContext.hh"
#include "data\SessionTracker.hh"
#include "data\UserContext.hh"

#include "services\AchievementRuntime.hh"
#include "services\IConfiguration.hh"
#include "services\IFileSystem.hh"
#include "services\IHttpRequester.hh"
#include "services\IThreadPool.hh"
#include "services\Initialization.hh"
#include "services\ServiceLocator.hh"

#include "ui\ImageReference.hh"
#include "ui\viewmodels\BrokenAchievementsViewModel.hh"
#include "ui\viewmodels\GameChecksumViewModel.hh"
#include "ui\viewmodels\LoginViewModel.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\OverlayManager.hh"
#include "ui\viewmodels\OverlaySettingsViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"
#include "ui\win32\Desktop.hh"
#include "ui\win32\OverlayWindow.hh"

std::wstring g_sHomeDir;
std::string g_sROMDirLocation;

HMODULE g_hThisDLLInst = nullptr;
HINSTANCE g_hRAKeysDLL = nullptr;
HWND g_RAMainWnd = nullptr;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, _UNUSED LPVOID)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            g_hThisDLLInst = hModule;
            ra::services::Initialization::RegisterCoreServices();
            break;

        case DLL_PROCESS_DETACH:
            IsolationAwareCleanup();
            break;
    }

    return TRUE;
}

API void CCONV _RA_UpdateHWnd(HWND hMainHWND)
{
    if (hMainHWND != g_RAMainWnd)
    {
        auto& pDesktop = dynamic_cast<ra::ui::win32::Desktop&>(ra::services::ServiceLocator::GetMutable<ra::ui::IDesktop>());
        pDesktop.SetMainHWnd(hMainHWND);
        g_RAMainWnd = hMainHWND;

        auto& pOverlayWindow = ra::services::ServiceLocator::GetMutable<ra::ui::win32::OverlayWindow>();
        pOverlayWindow.CreateOverlayWindow(hMainHWND);
    }
}

static void InitCommon(HWND hMainHWND, /*enum EmulatorID*/int nEmulatorID)
{
    ra::services::Initialization::RegisterServices(ra::itoe<EmulatorID>(nEmulatorID));

    // initialize global state
    _RA_UpdateHWnd(hMainHWND);

    auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
    g_sHomeDir = pFileSystem.BaseDirectory();

    //////////////////////////////////////////////////////////////////////////
    //	Dialogs:
    g_MemoryDialog.Init();
}

API BOOL CCONV _RA_InitOffline(HWND hMainHWND, /*enum EmulatorID*/int nEmulatorID, const char* /*sClientVer*/)
{
    InitCommon(hMainHWND, nEmulatorID);

    ra::services::ServiceLocator::GetMutable<ra::data::UserContext>().DisableLogin();

    return TRUE;
}

API BOOL CCONV _RA_InitI(HWND hMainHWND, /*enum EmulatorID*/int nEmulatorID, const char* sClientVer)
{
    InitCommon(hMainHWND, nEmulatorID);

    // Set the client version and User-Agent string
    ra::services::ServiceLocator::GetMutable<ra::data::EmulatorContext>().SetClientVersion(sClientVer);

    // validate version (async call)
    ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>().RunAsync([]
    {
        if (!ra::services::ServiceLocator::GetMutable<ra::data::EmulatorContext>().ValidateClientVersion())
            ra::services::ServiceLocator::GetMutable<ra::data::UserContext>().Logout();
    });

    return TRUE;
}

API int CCONV _RA_Shutdown()
{
    // if _RA_Init wasn't called, the services won't have been registered, so there's nothing to shut down
    if (ra::services::ServiceLocator::Exists<ra::services::IThreadPool>())
    {
        // notify the background threads as soon as possible so they start to wind down
        ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>().Shutdown(false);

        ra::services::ServiceLocator::Get<ra::services::IConfiguration>().Save();

        ra::services::ServiceLocator::GetMutable<ra::data::SessionTracker>().EndSession();

        ra::services::ServiceLocator::GetMutable<ra::data::GameContext>().LoadGame(0U);
    }

    if (g_AchievementsDialog.GetHWND() != nullptr)
    {
        DestroyWindow(g_AchievementsDialog.GetHWND());
        g_AchievementsDialog.InstallHWND(nullptr);
    }

    if (g_AchievementEditorDialog.GetHWND() != nullptr)
    {
        DestroyWindow(g_AchievementEditorDialog.GetHWND());
        g_AchievementEditorDialog.InstallHWND(nullptr);
    }

    if (g_MemoryDialog.GetHWND() != nullptr)
    {
        DestroyWindow(g_MemoryDialog.GetHWND());
        g_MemoryDialog.InstallHWND(nullptr);
    }
    g_MemoryDialog.Shutdown();

    if (g_GameLibrary.GetHWND() != nullptr)
    {
        DestroyWindow(g_GameLibrary.GetHWND());
        g_GameLibrary.InstallHWND(nullptr);
    }

    g_GameLibrary.KillThread();

    if (ra::services::ServiceLocator::Exists<ra::ui::viewmodels::WindowManager>())
    {
        auto& pWindowManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>();
        auto& pDesktop = ra::services::ServiceLocator::Get<ra::ui::IDesktop>();

        if (pWindowManager.MemoryBookmarks.IsVisible())
            pDesktop.CloseWindow(pWindowManager.MemoryBookmarks);
    }

    ra::services::Initialization::Shutdown();

    RA_LOG_INFO("Shutdown complete");

    return 0;
}

API bool CCONV _RA_ConfirmLoadNewRom(bool bQuittingApp)
{
    //	Returns true if we can go ahead and load the new rom.
    std::wstring sModifiedSet;

    bool bCoreModified = false;
    bool bUnofficialModified = false;
    bool bLocalModified = false;

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    pGameContext.EnumerateAchievements([&bCoreModified, &bUnofficialModified, &bLocalModified](const Achievement& pAchievement) noexcept
    {
        if (pAchievement.Modified())
        {
            switch (pAchievement.GetCategory())
            {
                case Achievement::Category::Local:
                    bLocalModified = true;
                    break;
                case Achievement::Category::Core:
                    bCoreModified = true;
                    break;
                case Achievement::Category::Unofficial:
                    bUnofficialModified = true;
                    break;
            }
        }

        return true;
    });

    if (bCoreModified)
        sModifiedSet = L"Core";
    else if (bUnofficialModified)
        sModifiedSet = L"Unofficial";
    else if (bLocalModified)
        sModifiedSet = L"Local";
    else
        return true;

    ra::ui::viewmodels::MessageBoxViewModel vmMessageBox;
    vmMessageBox.SetHeader(bQuittingApp ? L"Are you sure that you want to exit?" : L"Continue load?");
    vmMessageBox.SetMessage(ra::StringPrintf(L"You have unsaved changes in the %s achievements set. If you %s, you will lose these changes.",
        sModifiedSet, bQuittingApp ? L"quit now" : L"load a new ROM"));
    vmMessageBox.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);
    vmMessageBox.SetIcon(ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning);
    return (vmMessageBox.ShowModal() == ra::ui::DialogResult::Yes);
}

API bool CCONV _RA_WarnDisableHardcore(const char* sActivity)
{
    // already disabled, just return success
    auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
    if (!pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
        return true;

    // prompt. if user doesn't consent, return failure - caller should not continue
    ra::ui::viewmodels::MessageBoxViewModel vmMessageBox;
    vmMessageBox.SetHeader(L"Disable Hardcore mode?");
    vmMessageBox.SetMessage(L"You cannot " + ra::Widen(sActivity) + L" while Hardcore mode is active.");
    vmMessageBox.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);
    vmMessageBox.SetIcon(ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning);
    if (vmMessageBox.ShowModal() != ra::ui::DialogResult::Yes)
        return false;

    // user consented, switch to non-hardcore mode
    ra::services::ServiceLocator::GetMutable<ra::data::EmulatorContext>().DisableHardcoreMode();

    // return success
    return true;
}

API void CCONV _RA_OnReset()
{
    // if there's no game loaded, there shouldn't be any active achievements or popups to clear - except maybe the
    // logging in messages, which we don't want to clear.
    if (ra::services::ServiceLocator::Get<ra::data::GameContext>().GameId() == 0U)
        return;

    // Temporarily disable achievements while the system is resetting. They will automatically re-enable when
    // DoAchievementsFrame is called if the trigger is not active. Prevents most unexpected triggering caused
    // by resetting the emulator.
    ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>().ResetActiveAchievements();

    ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().ClearPopups();
}

//void FetchBinaryFromWeb( const char* sFilename )
//{
//	const unsigned int nBufferSize = (3*1024*1024);	//	3mb enough?
//
//	char* buffer = new char[nBufferSize];	
//	if( buffer != nullptr )
//	{
//		char sAddr[1024];
//		sprintf_s( sAddr, 1024, "/files/%s", sFilename );
//		char sOutput[1024];
//		sprintf_s( sOutput, 1024, "%s%s.new", g_sHomeDir, sFilename );
//
//		DWORD nBytesRead = 0;
//		if( RAWeb::DoBlockingHttpGet( sAddr, buffer, nBufferSize, &nBytesRead ) )
//			_WriteBufferToFile( sOutput, buffer, nBytesRead );
//
//		delete[] ( buffer );
//		buffer = nullptr;
//	}
//}

//	Following this function, an app should call AppendMenu to associate this submenu.
API HMENU CCONV _RA_CreatePopupMenu()
{
    HMENU hRA = CreatePopupMenu();
    if (ra::services::ServiceLocator::Get<ra::data::UserContext>().IsLoggedIn())
    {
        AppendMenu(hRA, MF_STRING, IDM_RA_FILES_LOGOUT, TEXT("Log&out"));
        AppendMenu(hRA, MF_SEPARATOR, 0U, nullptr);
        AppendMenu(hRA, MF_STRING, IDM_RA_OPENUSERPAGE, TEXT("Open my &User Page"));

        constexpr auto nGameFlags = ra::to_unsigned(MF_STRING);
        //if( g_pActiveAchievements->ra::GameID() == 0 )	//	Disabled til I can get this right: Snes9x doesn't call this?
        //	nGameFlags |= (MF_GRAYED|MF_DISABLED);

        auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();

        AppendMenu(hRA, nGameFlags, IDM_RA_OPENGAMEPAGE, TEXT("Open this &Game's Page"));
        AppendMenu(hRA, MF_SEPARATOR, 0U, nullptr);

        AppendMenu(hRA, pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore) ? MF_CHECKED : MF_UNCHECKED, IDM_RA_HARDCORE_MODE, TEXT("&Hardcore Mode"));
        AppendMenu(hRA, pConfiguration.IsFeatureEnabled(ra::services::Feature::NonHardcoreWarning) ? MF_CHECKED : MF_UNCHECKED, IDM_RA_NON_HARDCORE_WARNING, TEXT("Non-Hardcore &Warning"));
        AppendMenu(hRA, MF_SEPARATOR, 0U, nullptr);

        AppendMenu(hRA, pConfiguration.IsFeatureEnabled(ra::services::Feature::Leaderboards) ? MF_CHECKED : MF_UNCHECKED, IDM_RA_TOGGLELEADERBOARDS, TEXT("Enable &Leaderboards"));
        AppendMenu(hRA, MF_STRING, IDM_RA_OVERLAYSETTINGS, TEXT("O&verlay Settings"));
        AppendMenu(hRA, MF_SEPARATOR, 0U, nullptr);

        AppendMenu(hRA, MF_STRING, IDM_RA_FILES_ACHIEVEMENTS, TEXT("Achievement &Sets"));
        AppendMenu(hRA, MF_STRING, IDM_RA_FILES_ACHIEVEMENTEDITOR, TEXT("Achievement &Editor"));
        AppendMenu(hRA, MF_STRING, IDM_RA_FILES_MEMORYFINDER, TEXT("&Memory Inspector"));
        AppendMenu(hRA, MF_STRING, IDM_RA_FILES_MEMORYBOOKMARKS, TEXT("Memory &Bookmarks"));
        AppendMenu(hRA, MF_STRING, IDM_RA_PARSERICHPRESENCE, TEXT("Rich &Presence Monitor"));
        AppendMenu(hRA, MF_SEPARATOR, 0U, nullptr);

        AppendMenu(hRA, MF_STRING, IDM_RA_REPORTBROKENACHIEVEMENTS, TEXT("&Report Achievement Problem"));
        AppendMenu(hRA, MF_STRING, IDM_RA_GETROMCHECKSUM, TEXT("Get ROM &Checksum"));
        //AppendMenu(hRA, MF_STRING, IDM_RA_SCANFORGAMES, TEXT("Scan &for games"));
    }
    else
    {
        AppendMenu(hRA, MF_STRING, IDM_RA_FILES_LOGIN, TEXT("&Login to RA"));
    }

    return hRA;
}

void RestoreWindowPosition(HWND hDlg, const char* sDlgKey, bool bToRight, bool bToBottom)
{
    auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    const ra::ui::Position oPosition = pConfiguration.GetWindowPosition(std::string(sDlgKey));
    ra::ui::Size oSize = pConfiguration.GetWindowSize(std::string(sDlgKey));

    // if the remembered size is less than the default size, reset it
    RECT rc;
    GetWindowRect(hDlg, &rc);
    const int nDlgWidth = rc.right - rc.left;
    if (oSize.Width != INT32_MIN && oSize.Width < nDlgWidth)
        oSize.Width = INT32_MIN;
    const int nDlgHeight = rc.bottom - rc.top;
    if (oSize.Height != INT32_MIN && oSize.Height < nDlgHeight)
        oSize.Height = INT32_MIN;

    RECT rcMainWindow;
    GetWindowRect(g_RAMainWnd, &rcMainWindow);

    // determine where the window should be placed
    rc.left = (oPosition.X != INT32_MIN) ? (rcMainWindow.left + oPosition.X) : bToRight ? rcMainWindow.right : rcMainWindow.left;
    rc.right = (oSize.Width != INT32_MIN) ? (rc.left + oSize.Width) : (rc.left + nDlgWidth);
    rc.top = (oPosition.Y != INT32_MIN) ? (rcMainWindow.top + oPosition.Y) : bToBottom ? rcMainWindow.bottom : rcMainWindow.top;
    rc.bottom = (oSize.Height != INT32_MIN) ? (rc.top + oSize.Height) : (rc.top + nDlgHeight);

    // make sure the window is on screen
    RECT rcWorkArea;
    if (SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkArea, 0))
    {
        LONG offset = rc.right - rcWorkArea.right;
        if (offset > 0)
        {
            rc.left -= offset;
            rc.right -= offset;
        }
        offset = rcWorkArea.left - rc.left;
        if (offset > 0)
        {
            rc.left += offset;
            rc.right += offset;
        }

        offset = rc.bottom - rcWorkArea.bottom;
        if (offset > 0)
        {
            rc.top -= offset;
            rc.bottom -= offset;
        }
        offset = rcWorkArea.top - rc.top;
        if (offset > 0)
        {
            rc.top += offset;
            rc.bottom += offset;
        }
    }

    SetWindowPos(hDlg, nullptr, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 0);
}

void RememberWindowPosition(HWND hDlg, const char* sDlgKey)
{
    RECT rcMainWindow;
    GetWindowRect(g_RAMainWnd, &rcMainWindow);

    RECT rc;
    GetWindowRect(hDlg, &rc);

    ra::ui::Position oPosition;
    oPosition.X = rc.left - rcMainWindow.left;
    oPosition.Y = rc.top - rcMainWindow.top;

    ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>().SetWindowPosition(std::string(sDlgKey), oPosition);
}

void RememberWindowSize(HWND hDlg, const char* sDlgKey)
{
    RECT rc;
    GetWindowRect(hDlg, &rc);

    ra::ui::Size oSize;
    oSize.Width = rc.right - rc.left;
    oSize.Height = rc.bottom - rc.top;

    ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>().SetWindowSize(std::string(sDlgKey), oSize);
}

API void CCONV _RA_InvokeDialog(LPARAM nID)
{
    switch (nID)
    {
        case IDM_RA_FILES_ACHIEVEMENTS:
            if (g_AchievementsDialog.GetHWND() == nullptr)
                g_AchievementsDialog.InstallHWND(CreateDialog(g_hThisDLLInst, MAKEINTRESOURCE(IDD_RA_ACHIEVEMENTS), g_RAMainWnd, g_AchievementsDialog.s_AchievementsProc));
            if (g_AchievementsDialog.GetHWND() != nullptr)
                ShowWindow(g_AchievementsDialog.GetHWND(), SW_SHOW);
            break;

        case IDM_RA_FILES_ACHIEVEMENTEDITOR:
            if (g_AchievementEditorDialog.GetHWND() == nullptr)
                g_AchievementEditorDialog.InstallHWND(CreateDialog(g_hThisDLLInst, MAKEINTRESOURCE(IDD_RA_ACHIEVEMENTEDITOR), g_RAMainWnd, g_AchievementEditorDialog.s_AchievementEditorProc));
            if (g_AchievementEditorDialog.GetHWND() != nullptr)
                ShowWindow(g_AchievementEditorDialog.GetHWND(), SW_SHOW);
            break;

        case IDM_RA_FILES_MEMORYFINDER:
            if (g_MemoryDialog.GetHWND() == nullptr)
                g_MemoryDialog.InstallHWND(CreateDialog(g_hThisDLLInst, MAKEINTRESOURCE(IDD_RA_MEMORY), g_RAMainWnd, g_MemoryDialog.s_MemoryProc));
            if (g_MemoryDialog.GetHWND() != nullptr)
                ShowWindow(g_MemoryDialog.GetHWND(), SW_SHOW);
            break;

        case IDM_RA_FILES_MEMORYBOOKMARKS:
            ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryBookmarks.Show();
            break;

        case IDM_RA_FILES_LOGIN:
        {
            const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
            if (pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
            {
                auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::EmulatorContext>();
                if (!pEmulatorContext.ValidateClientVersion())
                {
                    // The version could not be validated, or the user has chosen to update. Don't login.
                    break;
                }
            }

            ra::ui::viewmodels::LoginViewModel vmLogin;
            vmLogin.ShowModal();
            break;
        }

        case IDM_RA_FILES_LOGOUT:
            ra::services::ServiceLocator::GetMutable<ra::data::UserContext>().Logout();
            break;

        case IDM_RA_HARDCORE_MODE:
        {
            auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::EmulatorContext>();
            const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
            if (pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
            {
                pEmulatorContext.DisableHardcoreMode();
            }
            else
            {
                if (!pEmulatorContext.EnableHardcoreMode())
                    break;
            }
        }
        break;

        case IDM_RA_NON_HARDCORE_WARNING:
        {
            auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
            pConfiguration.SetFeatureEnabled(ra::services::Feature::NonHardcoreWarning,
                !pConfiguration.IsFeatureEnabled(ra::services::Feature::NonHardcoreWarning));

            ra::services::ServiceLocator::Get<ra::data::EmulatorContext>().RebuildMenu();
        }
        break;

        case IDM_RA_REPORTBROKENACHIEVEMENTS:
        {
            ra::ui::viewmodels::BrokenAchievementsViewModel vmBrokenAchievements;
            if (vmBrokenAchievements.InitializeAchievements())
                vmBrokenAchievements.ShowModal();
        }
        break;

        case IDM_RA_GETROMCHECKSUM:
        {
            ra::ui::viewmodels::GameChecksumViewModel vmGameChecksum;
            vmGameChecksum.ShowModal();
            break;
        }

        case IDM_RA_OPENUSERPAGE:
        {
            const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::UserContext>();
            if (pUserContext.IsLoggedIn())
            {
                const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
                const auto sUrl = ra::StringPrintf("%s/user/%s", pConfiguration.GetHostUrl(), pUserContext.GetUsername());

                const auto& pDesktop = ra::services::ServiceLocator::Get<ra::ui::IDesktop>();
                pDesktop.OpenUrl(sUrl);
            }
            break;
        }

        case IDM_RA_OPENGAMEPAGE:
        {
            const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
            if (pGameContext.GameId() != 0)
            {
                const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
                const auto sUrl = ra::StringPrintf("%s/game/%u", pConfiguration.GetHostUrl(), pGameContext.GameId());

                const auto& pDesktop = ra::services::ServiceLocator::Get<ra::ui::IDesktop>();
                pDesktop.OpenUrl(sUrl);
            }
            else
            {
                MessageBox(nullptr, TEXT("No ROM loaded!"), TEXT("Error!"), MB_ICONWARNING);
            }
        }
        break;
/*
        case IDM_RA_SCANFORGAMES:

            if (g_sROMDirLocation.length() == 0)
            {
                g_sROMDirLocation = GetFolderFromDialog();
            }

            if (g_sROMDirLocation.length() > 0)
            {
                if (g_GameLibrary.GetHWND() == nullptr)
                {
                    _FetchGameHashLibraryFromWeb();		//	##BLOCKING##
                    _FetchMyProgressFromWeb();			//	##BLOCKING##

                    g_GameLibrary.InstallHWND(CreateDialog(g_hThisDLLInst, MAKEINTRESOURCE(IDD_RA_GAMELIBRARY), g_RAMainWnd, &Dlg_GameLibrary::s_GameLibraryProc));
                }

                if (g_GameLibrary.GetHWND() != nullptr)
                    ShowWindow(g_GameLibrary.GetHWND(), SW_SHOW);
            }
            break;
*/
        case IDM_RA_PARSERICHPRESENCE:
        {
            ra::services::ServiceLocator::GetMutable<ra::data::GameContext>().ReloadRichPresenceScript();

            auto& pWindowManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>();
            pWindowManager.RichPresenceMonitor.Show();
            pWindowManager.RichPresenceMonitor.StartMonitoring();

            break;
        }

        case IDM_RA_TOGGLELEADERBOARDS:
        {
            auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
            const bool bLeaderboardsActive = !pConfiguration.IsFeatureEnabled(ra::services::Feature::Leaderboards);
            pConfiguration.SetFeatureEnabled(ra::services::Feature::Leaderboards, bLeaderboardsActive);

            if (!bLeaderboardsActive)
            {
                ra::ui::viewmodels::MessageBoxViewModel::ShowMessage(L"Leaderboards are now disabled.");
                ra::services::ServiceLocator::GetMutable<ra::data::GameContext>().DeactivateLeaderboards();
            }
            else
            {
                const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
                std::wstring sMessage = L"Leaderboards are now enabled.";
                if (pGameContext.GameId() != 0)
                    sMessage += L"\nYou may need to reload the game to activate them.";
                ra::ui::viewmodels::MessageBoxViewModel::ShowMessage(sMessage);
            }

            ra::services::ServiceLocator::Get<ra::data::EmulatorContext>().RebuildMenu();
        }
        break;

        case IDM_RA_OVERLAYSETTINGS:
        {
            ra::ui::viewmodels::OverlaySettingsViewModel vmSettings;
            vmSettings.Initialize();
            if (vmSettings.ShowModal() == ra::ui::DialogResult::OK)
                vmSettings.Commit();
        }
        break;

        default:
            //	Unknown!
            break;
    }
}

API void CCONV _RA_SetPaused(bool bIsPaused)
{
    auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
    if (bIsPaused)
        pOverlayManager.ShowOverlay();
    else
        pOverlayManager.HideOverlay();
}

API void CCONV _RA_OnSaveState(const char* sFilename)
{
    if (ra::services::ServiceLocator::Get<ra::data::UserContext>().IsLoggedIn())
    {
        if (!ra::services::ServiceLocator::Get<ra::data::EmulatorContext>().WasMemoryModified())
            ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>().SaveProgress(sFilename);
    }
}

API void CCONV _RA_OnLoadState(const char* sFilename)
{
    //	Save State is being allowed by app (user was warned!)
    if (ra::services::ServiceLocator::Get<ra::data::UserContext>().IsLoggedIn())
    {
        auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
        if (pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Disabling Hardcore mode.", L"Loading save states is not allowed in Hardcore mode.");
            ra::services::ServiceLocator::GetMutable<ra::data::EmulatorContext>().DisableHardcoreMode();
        }

        ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>().LoadProgress(sFilename);
        ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().ClearPopups();

        g_MemoryDialog.Invalidate();

        if (g_AchievementEditorDialog.ActiveAchievement() != nullptr)
            g_AchievementEditorDialog.ActiveAchievement()->SetDirtyFlag(Achievement::DirtyFlags::Conditions);
    }
}

//////////////////////////////////////////////////////////////////////////

BOOL _ReadTil(const char nChar, char* restrict buffer, unsigned int nSize,
              gsl::not_null<DWORD* restrict> pCharsReadOut, gsl::not_null<FILE* restrict> pFile)
{
    Expects(buffer != nullptr);
    char pNextChar = '\0';
    memset(buffer, '\0', nSize);

    // Read title:
    (*pCharsReadOut) = 0;
    do
    {
        if (fread(&pNextChar, sizeof(char), 1, pFile) == 0)
            break;

        buffer[(*pCharsReadOut)++] = pNextChar;
    } while (pNextChar != nChar && (*pCharsReadOut) < nSize && !feof(pFile));
    
    Ensures(buffer != nullptr);
    return ((*pCharsReadOut) > 0);
}

char* _ReadStringTil(char nChar, char* restrict& pOffsetInOut, BOOL bTerminate)
{
    Expects(pOffsetInOut != nullptr);
    char* pStartString = pOffsetInOut;

    while ((*pOffsetInOut) != '\0' && (*pOffsetInOut) != nChar)
        pOffsetInOut++;
    if (bTerminate)
        (*pOffsetInOut) = '\0';

    pOffsetInOut++;

    return (pStartString);
}

void _ReadStringTil(std::string& value, char nChar, const char* restrict& pSource)
{
    Expects(pSource != nullptr);
    const char* pStartString = pSource;

    while (*pSource != '\0' && *pSource != nChar)
        pSource++;

    value.assign(pStartString, pSource - pStartString);
    pSource++;
}

void _WriteBufferToFile(const std::wstring& sFileName, const std::string& raw)
{
    std::ofstream ofile{ sFileName, std::ios::binary };
    if (!ofile.is_open())
        return;
    ofile.write(raw.c_str(), ra::to_signed(raw.length()));
}

_Use_decl_annotations_ bool _ReadBufferFromFile(std::string& buffer, const wchar_t* const restrict sFile)
{
    buffer.clear();
    std::ifstream file(sFile);
    if (file.fail())
        return false;

    file.seekg(0, std::ios::end);
    buffer.reserve(static_cast<size_t>(file.tellg()));
    file.seekg(0, std::ios::beg);

    buffer.assign((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));

    return true;
}

_Use_decl_annotations_
char* _MallocAndBulkReadFileToBuffer(const wchar_t* sFilename, long& nFileSizeOut) noexcept
{
    nFileSizeOut = 0L;
    FILE* pf = nullptr;
    _wfopen_s(&pf, sFilename, L"r");
    if (pf == nullptr)
        return nullptr;

    // Calculate filesize
    fseek(pf, 0L, SEEK_END);
    nFileSizeOut = ftell(pf);
    fseek(pf, 0L, SEEK_SET);

    if (nFileSizeOut <= 0)
    {
        //	No good content in this file.
        fclose(pf);
        return nullptr;
    }

    //	malloc() must be managed!
    //	NB. By adding +1, we allow for a single \0 character :)
    char* pRawFileOut = static_cast<char*>(std::malloc((nFileSizeOut + 1) * sizeof(char)));
    if (pRawFileOut)
    {
        ZeroMemory(pRawFileOut, nFileSizeOut + 1);
        fread(pRawFileOut, nFileSizeOut, sizeof(char), pf);
    }

    fclose(pf);

    return pRawFileOut;
}

namespace ra {

_Success_(return == 0)
_NODISCARD static auto CALLBACK
BrowseCallbackProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ _UNUSED LPARAM lParam, _In_ LPARAM lpData) noexcept // it might
{
    ASSERT(uMsg != BFFM_VALIDATEFAILED);
    if (uMsg == BFFM_INITIALIZED)
    {
        LPCTSTR path{};
        GSL_SUPPRESS_TYPE1 path = reinterpret_cast<LPCTSTR>(lpData);
        GSL_SUPPRESS_TYPE1 ::SendMessage(hwnd, ra::to_unsigned(BFFM_SETSELECTION), 0U, reinterpret_cast<LPARAM>(path));

    }
    return 0;
}

} /* namespace ra */

std::string GetFolderFromDialog()
{
    BROWSEINFO bi{};
    bi.hwndOwner = ::GetActiveWindow();

    std::array<TCHAR, RA_MAX_PATH> pDisplayName{};
    bi.pszDisplayName = pDisplayName.data();
    bi.lpszTitle = _T("Select ROM folder...");

    if (::OleInitialize(nullptr) != S_OK)
        return std::string();

    bi.ulFlags = BIF_USENEWUI | BIF_VALIDATE;
    bi.lpfn    = ra::BrowseCallbackProc;
    GSL_SUPPRESS_TYPE1 bi.lParam = reinterpret_cast<LPARAM>(g_sHomeDir.c_str());
    
    std::string ret;
    {
        const auto idlist_deleter =[](LPITEMIDLIST lpItemIdList) noexcept
        {
            ::CoTaskMemFree(lpItemIdList);
            lpItemIdList = nullptr;
        };

        using ItemListOwner = std::unique_ptr<ITEMIDLIST, decltype(idlist_deleter)>;
        ItemListOwner owner{ ::SHBrowseForFolder(&bi), idlist_deleter };
        if (!owner)
        {
            ::OleUninitialize();
            return std::string();
        }

        if (::SHGetPathFromIDList(owner.get(), bi.pszDisplayName) == 0)
        {
            ::OleUninitialize();
            return std::string();
        }
        ret = ra::Narrow(bi.pszDisplayName);
    }
    ::OleUninitialize();
    return ret;
}
