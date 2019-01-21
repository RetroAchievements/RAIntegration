#include "RA_Core.h"

#include "RA_AchievementOverlay.h" // RA_User
#include "RA_CodeNotes.h"
#include "RA_ImageFactory.h"
#include "RA_MemManager.h"
#include "RA_Resource.h"
#include "RA_RichPresence.h"
#include "RA_httpthread.h"
#include "RA_md5factory.h"

#include "RA_Dlg_AchEditor.h"   // RA_httpthread.h, services/ImageRepository.h
#include "RA_Dlg_Achievement.h" // RA_AchievementSet.h
#include "RA_Dlg_AchievementsReporter.h"
#include "RA_Dlg_GameLibrary.h"
#include "RA_Dlg_GameTitle.h"
#include "RA_Dlg_MemBookmark.h"
#include "RA_Dlg_Memory.h"

#include "api\Logout.hh"
#include "api\ResolveHash.hh"

#include "data\EmulatorContext.hh"
#include "data\GameContext.hh"
#include "data\SessionTracker.hh"
#include "data\UserContext.hh"

#include "services\AchievementRuntime.hh"
#include "services\IAudioSystem.hh"
#include "services\IConfiguration.hh"
#include "services\IFileSystem.hh"
#include "services\ILeaderboardManager.hh"
#include "services\IThreadPool.hh"
#include "services\Initialization.hh"
#include "services\ServiceLocator.hh"

// for SubmitEntry callback
#include "services\impl\LeaderboardManager.hh" // services/IConfiguration.hh, services/ILeaderboardManager.hh

#include "ui\ImageReference.hh"
#include "ui\viewmodels\GameChecksumViewModel.hh"
#include "ui\viewmodels\LoginViewModel.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\OverlayManager.hh"
#include "ui\viewmodels\WindowManager.hh"

std::wstring g_sHomeDir;
std::string g_sROMDirLocation;

HMODULE g_hThisDLLInst = nullptr;
HINSTANCE g_hRAKeysDLL = nullptr;
HWND g_RAMainWnd = nullptr;
ConsoleID g_ConsoleID = ConsoleID::UnknownConsoleID;	//	Currently active Console ID
bool g_bRAMTamperedWith = false;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, _UNUSED LPVOID)
{
    if (dwReason == DLL_PROCESS_ATTACH)
        g_hThisDLLInst = hModule;

    ra::services::Initialization::RegisterCoreServices();

    return TRUE;
}

static void InitCommon(HWND hMainHWND, /*enum EmulatorID*/int nEmulatorID)
{
    ra::services::Initialization::RegisterServices(ra::itoe<EmulatorID>(nEmulatorID));

    // initialize global state
    g_RAMainWnd = hMainHWND;

    auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
    g_sHomeDir = pFileSystem.BaseDirectory();

    //////////////////////////////////////////////////////////////////////////
    //	Dialogs:
    g_MemoryDialog.Init();

    //////////////////////////////////////////////////////////////////////////
    //	Initialize All AchievementSets
    g_pCoreAchievements = new AchievementSet();
    g_pUnofficialAchievements = new AchievementSet();
    g_pLocalAchievements = new AchievementSet();
    g_pActiveAchievements = g_pCoreAchievements;

    //////////////////////////////////////////////////////////////////////////
    //	Image rendering: Setup overlay
    g_AchievementOverlay.UpdateImages();
}

API BOOL CCONV _RA_InitOffline(HWND hMainHWND, /*enum EmulatorID*/int nEmulatorID, const char* /*sClientVer*/)
{
    InitCommon(hMainHWND, nEmulatorID);
    return TRUE;
}

API BOOL CCONV _RA_InitI(HWND hMainHWND, /*enum EmulatorID*/int nEmulatorID, const char* sClientVer)
{
    InitCommon(hMainHWND, nEmulatorID);

    // Set the client version and User-Agent string
    ra::services::ServiceLocator::GetMutable<ra::data::EmulatorContext>().SetClientVersion(sClientVer);
    RAWeb::SetUserAgentString();

    //////////////////////////////////////////////////////////////////////////
    //	Update news:
    PostArgs args;
    args['c'] = std::to_string(6);
    RAWeb::CreateThreadedHTTPRequest(RequestNews, args);

    // validate version (async call)
    ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>().RunAsync([]
    {
        if (!ra::services::ServiceLocator::GetMutable<ra::data::EmulatorContext>().ValidateClientVersion())
            ra::services::ServiceLocator::GetMutable<ra::data::UserContext>().Logout();
    });

    //	TBD:
    //if( RAUsers::LocalUser().Username().length() > 0 )
    //{
    //	args.clear();
    //	args[ 'u' ] = RAUsers::LocalUser().Username();
    //	args[ 't' ] = RAUsers::LocalUser().Token();
    //	RAWeb::CreateThreadedHTTPRequest( RequestScore, args );
    //}

    return TRUE;
}

API int CCONV _RA_Shutdown()
{
    // notify the background threads as soon as possible so they start to wind down
    ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>().Shutdown(false);

    ra::services::ServiceLocator::Get<ra::services::IConfiguration>().Save();

    ra::services::ServiceLocator::GetMutable<ra::data::SessionTracker>().EndSession();

    ra::services::ServiceLocator::GetMutable<ra::data::GameContext>().LoadGame(0U);

    g_pActiveAchievements = nullptr;
    SAFE_DELETE(g_pCoreAchievements);
    SAFE_DELETE(g_pUnofficialAchievements);
    SAFE_DELETE(g_pLocalAchievements);

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

    if (g_GameLibrary.GetHWND() != nullptr)
    {
        DestroyWindow(g_GameLibrary.GetHWND());
        g_GameLibrary.InstallHWND(nullptr);
    }

    g_GameLibrary.KillThread();

    if (g_MemBookmarkDialog.GetHWND() != nullptr)
    {
        DestroyWindow(g_MemBookmarkDialog.GetHWND());
        g_MemBookmarkDialog.InstallHWND(nullptr);
    }

    ra::services::Initialization::Shutdown();

    CoUninitialize();

    RA_LOG_INFO("Shutdown complete");

    return 0;
}

API bool CCONV _RA_ConfirmLoadNewRom(bool bQuittingApp)
{
    //	Returns true if we can go ahead and load the new rom.
    int nResult = IDYES;

    const char* sCurrentAction = bQuittingApp ? "quit now" : "load a new ROM";
    const char* sNextAction = bQuittingApp ? "Are you sure?" : "Continue load?";

    if (g_pCoreAchievements->HasUnsavedChanges())
    {
        char buffer[1024];
        sprintf_s(buffer, 1024,
            "You have unsaved changes in the Core Achievements set.\n"
            "If you %s you will lose these changes.\n"
            "%s", sCurrentAction, sNextAction);

        nResult = MessageBox(g_RAMainWnd, NativeStr(buffer).c_str(), TEXT("Warning"), MB_ICONWARNING | MB_YESNO);
    }
    if (g_pUnofficialAchievements->HasUnsavedChanges())
    {
        char buffer[1024];
        sprintf_s(buffer, 1024,
            "You have unsaved changes in the Unofficial Achievements set.\n"
            "If you %s you will lose these changes.\n"
            "%s", sCurrentAction, sNextAction);

        nResult = MessageBox(g_RAMainWnd, NativeStr(buffer).c_str(), TEXT("Warning"), MB_ICONWARNING | MB_YESNO);
    }
    if (g_pLocalAchievements->HasUnsavedChanges())
    {
        char buffer[1024];
        sprintf_s(buffer, 1024,
            "You have unsaved changes in the Local Achievements set.\n"
            "If you %s you will lose these changes.\n"
            "%s", sCurrentAction, sNextAction);

        nResult = MessageBox(g_RAMainWnd, NativeStr(buffer).c_str(), TEXT("Warning"), MB_ICONWARNING | MB_YESNO);
    }

    return(nResult == IDYES);
}

API void CCONV _RA_SetConsoleID(unsigned int nConsoleID)
{
    g_ConsoleID = static_cast<ConsoleID>(nConsoleID);
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

static unsigned int IdentifyRom(const BYTE* pROM, unsigned int nROMSize, std::string& sCurrentROMMD5)
{
    unsigned int nGameId = 0U;

    if (pROM == nullptr || nROMSize == 0)
    {
        sCurrentROMMD5 = RAGenerateMD5(nullptr, 0);
    }
    else
    {
        sCurrentROMMD5 = RAGenerateMD5(pROM, nROMSize);

        // TBD: local DB of MD5 to ra::GameIDs here

        // Fetch the gameID from the DB
        ra::api::ResolveHash::Request request;
        request.Hash = sCurrentROMMD5;

        const auto response = request.Call();
        if (response.Succeeded())
        {
            nGameId = response.GameId;
            if (nGameId == 0) // Unknown
            {
                RA_LOG("Could not identify game with MD5 %s\n", sCurrentROMMD5.c_str());

                char buffer[64];
                ZeroMemory(buffer, 64);
                RA_GetEstimatedGameTitle(buffer);
                buffer[sizeof(buffer) - 1] = '\0'; // ensure buffer is null terminated
                std::string sEstimatedGameTitle(buffer);

                Dlg_GameTitle::DoModalDialog(g_hThisDLLInst, g_RAMainWnd, sCurrentROMMD5, sEstimatedGameTitle, nGameId);
            }
            else
            {
                RA_LOG("Successfully looked up game with ID %u\n", nGameId);
            }
        }
        else
        {
            std::wstring sErrorMessage = ra::Widen(response.ErrorMessage);
            if (sErrorMessage.empty())
                sErrorMessage = ra::StringPrintf(L"Error from %s", _RA_HostName());
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Could not identify game.", sErrorMessage);
        }
    }

    return nGameId;
}

static void ActivateGame(unsigned int nGameId)
{
    if (!ra::services::ServiceLocator::Get<ra::data::UserContext>().IsLoggedIn())
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Cannot load achievements",
            L"You must be logged in to load achievements. Please reload the game after logging in.");
        return;
    }

    g_bRAMTamperedWith = false;

    ra::services::ServiceLocator::GetMutable<ra::services::ILeaderboardManager>().Clear();

    if (nGameId != 0)
    {
        RA_LOG("Loading game %u", nGameId);

        ra::services::ServiceLocator::GetMutable<ra::data::GameContext>().LoadGame(nGameId);
        ra::services::ServiceLocator::GetMutable<ra::data::SessionTracker>().BeginSession(nGameId);

        auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
        if (!pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
        {
            const bool bLeaderboardsEnabled = pConfiguration.IsFeatureEnabled(ra::services::Feature::Leaderboards);

            ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\info.wav");
            ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(
                L"Playing in Softcore Mode",
                bLeaderboardsEnabled ? L"Leaderboard submissions will be canceled." : L"");
        }
    }
    else
    {
        RA_LOG("Unloading current game");

        ra::services::ServiceLocator::GetMutable<ra::data::SessionTracker>().EndSession();
        ra::services::ServiceLocator::GetMutable<ra::data::GameContext>().LoadGame(0U);
    }

    g_AchievementsDialog.OnLoad_NewRom(nGameId);
    g_AchievementEditorDialog.OnLoad_NewRom();
    g_MemoryDialog.OnLoad_NewRom();
    g_AchievementOverlay.OnLoad_NewRom();
    g_MemBookmarkDialog.OnLoad_NewRom();

    ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().RichPresenceMonitor.UpdateDisplayString();
}

struct PendingRom
{
    std::string sMD5;
    unsigned int nGameId{};
};

API unsigned int CCONV _RA_IdentifyRom(const BYTE* pROM, unsigned int nROMSize)
{
    auto pPendingRom = std::make_unique<PendingRom>();
    const auto nGameId = IdentifyRom(pROM, nROMSize, pPendingRom->sMD5);

    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::GameContext>();
    if (nGameId != 0 && nGameId == pGameContext.GameId())
    {
        // same as currently loaded rom. assume user is switching disks and _RA_ActivateGame won't be called.
        // update the hash now
        pGameContext.SetGameHash(pPendingRom->sMD5);
    }
    else
    {
        // different game. assume user is loading a new game and _RA_ActivateGame will be called.
        // store the hash so when _RA_ActivateGame is called, we can load it then.
        pPendingRom->nGameId = nGameId;
        ra::services::ServiceLocator::Provide<PendingRom>(std::move(pPendingRom));
    }

    return nGameId;
}

static void ActivateHash(const std::string& sHash, unsigned int nGameId)
{
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::GameContext>();
    pGameContext.SetGameHash(sHash);

    // if the hash didn't resolve, we still want to ping with "Playing GAMENAME"
    if (nGameId == 0)
    {
        char buffer[64]{};
        RA_GetEstimatedGameTitle(buffer);
        buffer[sizeof(buffer) - 1] = '\0'; // ensure buffer is null terminated
        std::wstring sEstimatedGameTitle = ra::ToWString(buffer);
        if (sEstimatedGameTitle.empty())
            sEstimatedGameTitle = L"Unknown";
        pGameContext.SetGameTitle(sEstimatedGameTitle);

        ra::services::ServiceLocator::GetMutable<ra::data::SessionTracker>().BeginSession(nGameId);
    }
}

API void CCONV _RA_ActivateGame(unsigned int nGameId)
{
    ActivateGame(nGameId);

    // if the hash was captured, use it
    if (ra::services::ServiceLocator::Exists<PendingRom>())
    {
        const auto& pPendingRom = ra::services::ServiceLocator::Get<PendingRom>();
        if (nGameId == pPendingRom.nGameId)
            ActivateHash(pPendingRom.sMD5, nGameId);
    }
}

API int CCONV _RA_OnLoadNewRom(const BYTE* pROM, unsigned int nROMSize)
{
    std::string sCurrentROMMD5;
    const auto nGameId = IdentifyRom(pROM, nROMSize, sCurrentROMMD5);

    ActivateGame(nGameId);

    // if a ROM was provided, store the hash, even if it didn't match anything
    if (pROM && nROMSize)
        ActivateHash(sCurrentROMMD5, nGameId);

    return 0;
}

API void CCONV _RA_OnReset()
{
    // Temporarily disable achievements while the system is resetting. They will automatically re-enable when
    // DoAchievementsFrame is called if the trigger is not active. Prevents most unexpected triggering caused
    // by resetting the emulator.
    ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>().ResetActiveAchievements();

    ra::services::ServiceLocator::GetMutable<ra::services::ILeaderboardManager>().Reset();
}

API void CCONV _RA_InstallMemoryBank(int nBankID, void* pReader, void* pWriter, int nBankSize)
{
    g_MemManager.AddMemoryBank(ra::to_unsigned(nBankID), (_RAMByteReadFn*)pReader, (_RAMByteWriteFn*)pWriter, ra::to_unsigned(nBankSize));
    g_MemoryDialog.AddBank(nBankID);
}

API void CCONV _RA_ClearMemoryBanks()
{
    g_MemManager.ClearMemoryBanks();
    g_MemoryDialog.ClearBanks();
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

API int CCONV _RA_HandleHTTPResults()
{
    WaitForSingleObject(RAWeb::Mutex(), INFINITE);

    RequestObject* pObj = RAWeb::PopNextHttpResult();
    while (pObj != nullptr)
    {
        rapidjson::Document doc;
        if (pObj->GetResponse().size() > 0 && pObj->ParseResponseToJSON(doc))
        {
            switch (pObj->GetRequestType())
            {
                case RequestBadgeIter:
                    g_AchievementEditorDialog.GetBadgeNames().OnNewBadgeNames(doc);
                    break;

                case RequestFriendList:
                    RAUsers::LocalUser().OnFriendListResponse(doc);
                    break;

                case RequestScore:
                {
                    ASSERT(doc["Success"].GetBool());
                    if (doc["Success"].GetBool() && doc.HasMember("User") && doc.HasMember("Score"))
                    {
                        const std::string& sUser = doc["User"].GetString();
                        const unsigned int nScore = doc["Score"].GetUint();
                        RA_LOG("%s's score: %u", sUser.c_str(), nScore);

                        auto& pUserContext = ra::services::ServiceLocator::GetMutable<ra::data::UserContext>();
                        if (sUser.compare(pUserContext.GetUsername()) == 0)
                        {
                            pUserContext.SetScore(nScore);
                        }
                        else
                        {
                            // Find friend? Update this information?
                            RAUsers::GetUser(sUser).SetScore(nScore);
                        }
                    }
                    else
                    {
                        ASSERT(!"RequestScore bad response!?");
                        RA_LOG("RequestScore bad response!?");
                    }
                }
                break;

                case RequestNews:
                    _WriteBufferToFile(g_sHomeDir + RA_NEWS_FILENAME, doc);
                    g_AchievementOverlay.InstallNewsArticlesFromFile();
                    break;

                case RequestAchievementInfo:
                    g_AchExamine.OnReceiveData(doc);
                    break;

                case RequestCodeNotes:
                    CodeNotes::OnCodeNotesResponse(doc);
                    break;

                case RequestSubmitLeaderboardEntry:
                    ra::services::impl::LeaderboardManager::OnSubmitEntry(doc);
                    break;

                case RequestLeaderboardInfo:
                    g_LBExamine.OnReceiveData(doc);
                    break;
            }
        }

        SAFE_DELETE(pObj);
        pObj = RAWeb::PopNextHttpResult();
    }

    ReleaseMutex(RAWeb::Mutex());
    return 0;
}

//	Following this function, an app should call AppendMenu to associate this submenu.
API HMENU CCONV _RA_CreatePopupMenu()
{
    HMENU hRA = CreatePopupMenu();
    HMENU hRA_LB = CreatePopupMenu();
    if (ra::services::ServiceLocator::Get<ra::data::UserContext>().IsLoggedIn())
    {
        AppendMenu(hRA, MF_STRING, IDM_RA_FILES_LOGOUT, TEXT("Log&out"));
        AppendMenu(hRA, MF_SEPARATOR, 0U, nullptr);
        AppendMenu(hRA, MF_STRING, IDM_RA_OPENUSERPAGE, TEXT("Open my &User Page"));

        constexpr auto nGameFlags = ra::to_unsigned(MF_STRING);
        //if( g_pActiveAchievements->ra::GameID() == 0 )	//	Disabled til I can get this right: Snes9x doesn't call this?
        //	nGameFlags |= (MF_GRAYED|MF_DISABLED);

        auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();

        // TODO: Replace UINT_PTR{} with the _z literal after PR #23 gets accepted
        AppendMenu(hRA, nGameFlags, IDM_RA_OPENGAMEPAGE, TEXT("Open this &Game's Page"));
        AppendMenu(hRA, MF_SEPARATOR, 0U, nullptr);
        AppendMenu(hRA, pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore) ? MF_CHECKED : MF_UNCHECKED, IDM_RA_HARDCORE_MODE, TEXT("&Hardcore Mode"));
        AppendMenu(hRA, MF_SEPARATOR, 0U, nullptr);

        GSL_SUPPRESS_TYPE1 AppendMenu(hRA, MF_POPUP, reinterpret_cast<UINT_PTR>(hRA_LB), TEXT("Leaderboards"));
        AppendMenu(hRA_LB, pConfiguration.IsFeatureEnabled(ra::services::Feature::Leaderboards) ? MF_CHECKED : MF_UNCHECKED, IDM_RA_TOGGLELEADERBOARDS, TEXT("Enable &Leaderboards"));
        AppendMenu(hRA_LB, MF_SEPARATOR, 0U, nullptr);
        AppendMenu(hRA_LB, pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardNotifications) ? MF_CHECKED : MF_UNCHECKED, IDM_RA_TOGGLE_LB_NOTIFICATIONS, TEXT("Display Challenge Notification"));
        AppendMenu(hRA_LB, pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardCounters) ? MF_CHECKED : MF_UNCHECKED, IDM_RA_TOGGLE_LB_COUNTER, TEXT("Display Time/Score Counter"));
        AppendMenu(hRA_LB, pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardScoreboards) ? MF_CHECKED : MF_UNCHECKED, IDM_RA_TOGGLE_LB_SCOREBOARD, TEXT("Display Rank Scoreboard"));

        AppendMenu(hRA, MF_SEPARATOR, 0U, nullptr);
        AppendMenu(hRA, MF_STRING, IDM_RA_FILES_ACHIEVEMENTS, TEXT("Achievement &Sets"));
        AppendMenu(hRA, MF_STRING, IDM_RA_FILES_ACHIEVEMENTEDITOR, TEXT("Achievement &Editor"));
        AppendMenu(hRA, MF_STRING, IDM_RA_FILES_MEMORYFINDER, TEXT("&Memory Inspector"));
        AppendMenu(hRA, MF_STRING, IDM_RA_PARSERICHPRESENCE, TEXT("Rich &Presence Monitor"));
        AppendMenu(hRA, MF_SEPARATOR, 0U, nullptr);
        AppendMenu(hRA, MF_STRING, IDM_RA_REPORTBROKENACHIEVEMENTS, TEXT("&Report Broken Achievements"));
        AppendMenu(hRA, MF_STRING, IDM_RA_GETROMCHECKSUM, TEXT("Get ROM &Checksum"));
        //AppendMenu(hRA, MF_STRING, IDM_RA_SCANFORGAMES, TEXT("Scan &for games"));
    }
    else
    {
        AppendMenu(hRA, MF_STRING, IDM_RA_FILES_LOGIN, TEXT("&Login to RA"));
    }

    return hRA;
}

void _FetchGameHashLibraryFromWeb()
{
    PostArgs args;
    args['c'] = std::to_string(g_ConsoleID);
    args['u'] = RAUsers::LocalUser().Username();
    args['t'] = RAUsers::LocalUser().Token();
    std::string Response;
    if (RAWeb::DoBlockingRequest(RequestHashLibrary, args, Response))
        _WriteBufferToFile(g_sHomeDir + RA_GAME_HASH_FILENAME, Response);
}

void _FetchGameTitlesFromWeb()
{
    PostArgs args;
    args['c'] = std::to_string(g_ConsoleID);
    args['u'] = RAUsers::LocalUser().Username();
    args['t'] = RAUsers::LocalUser().Token();
    std::string Response;
    if (RAWeb::DoBlockingRequest(RequestGamesList, args, Response))
        _WriteBufferToFile(g_sHomeDir + RA_GAME_LIST_FILENAME, Response);
}

void _FetchMyProgressFromWeb()
{
    PostArgs args;
    args['c'] = std::to_string(g_ConsoleID);
    args['u'] = RAUsers::LocalUser().Username();
    args['t'] = RAUsers::LocalUser().Token();
    std::string Response;
    if (RAWeb::DoBlockingRequest(RequestAllProgress, args, Response))
        _WriteBufferToFile(g_sHomeDir + RA_MY_PROGRESS_FILENAME, Response);
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

            ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().ClearPopups();
        }
        break;

        case IDM_RA_REPORTBROKENACHIEVEMENTS:
            Dlg_AchievementsReporter::DoModalDialog(g_hThisDLLInst, g_RAMainWnd);
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
                std::ostringstream oss;
                oss << "http://" << _RA_HostName() << "/User/" << pUserContext.GetUsername();
                ShellExecute(nullptr,
                    TEXT("open"),
                    NativeStr(oss.str()).c_str(),
                    nullptr,
                    nullptr,
                    SW_SHOWNORMAL);
            }
            break;
        }

        case IDM_RA_OPENGAMEPAGE:
        {
            const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
            if (pGameContext.GameId() != 0)
            {
                std::ostringstream oss;
                oss << "http://" << _RA_HostName() << "/Game/" << pGameContext.GameId();
                ShellExecute(nullptr,
                    TEXT("open"),
                    NativeStr(oss.str()).c_str(),
                    nullptr,
                    nullptr,
                    SW_SHOWNORMAL);
            }
            else
            {
                MessageBox(nullptr, TEXT("No ROM loaded!"), TEXT("Error!"), MB_ICONWARNING);
            }
        }
        break;

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
                    _FetchGameTitlesFromWeb();			//	##BLOCKING##
                    _FetchMyProgressFromWeb();			//	##BLOCKING##

                    g_GameLibrary.InstallHWND(CreateDialog(g_hThisDLLInst, MAKEINTRESOURCE(IDD_RA_GAMELIBRARY), g_RAMainWnd, &Dlg_GameLibrary::s_GameLibraryProc));
                }

                if (g_GameLibrary.GetHWND() != nullptr)
                    ShowWindow(g_GameLibrary.GetHWND(), SW_SHOW);
            }
            break;

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
                ra::services::ServiceLocator::GetMutable<ra::services::ILeaderboardManager>().Reset();
            }
            else
            {
                const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
                std::wstring sMessage = L"Leaderboards are now enabled.";
                if (pGameContext.GameId() != 0)
                    sMessage += L"\nYou may need to reload the game to activate them.";
                ra::ui::viewmodels::MessageBoxViewModel::ShowMessage(sMessage);
            }

            _RA_RebuildMenu();
        }
        break;

        case IDM_RA_TOGGLE_LB_NOTIFICATIONS:
        {
            auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
            pConfiguration.SetFeatureEnabled(ra::services::Feature::LeaderboardNotifications,
                !pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardNotifications));

            _RA_RebuildMenu();
        }
        break;

        case IDM_RA_TOGGLE_LB_COUNTER:
        {
            auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
            pConfiguration.SetFeatureEnabled(ra::services::Feature::LeaderboardCounters,
                !pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardCounters));

            _RA_RebuildMenu();
        }
        break;

        case IDM_RA_TOGGLE_LB_SCOREBOARD:
        {
            auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
            pConfiguration.SetFeatureEnabled(ra::services::Feature::LeaderboardScoreboards,
                !pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardScoreboards));

            _RA_RebuildMenu();
        }
        break;

        default:
            //	Unknown!
            break;
    }
}

API void CCONV _RA_SetPaused(bool bIsPaused)
{
    //	TBD: store this state?? (Rendering?)
    if (bIsPaused)
        g_AchievementOverlay.Activate();
    else
        g_AchievementOverlay.Deactivate();
}

API void CCONV _RA_OnSaveState(const char* sFilename)
{
    if (ra::services::ServiceLocator::Get<ra::data::UserContext>().IsLoggedIn())
    {
        if (!g_bRAMTamperedWith)
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
        ra::services::ServiceLocator::GetMutable<ra::services::ILeaderboardManager>().Reset();
        g_MemoryDialog.Invalidate();

        for (size_t i = 0; i < g_pActiveAchievements->NumAchievements(); ++i)
            g_pActiveAchievements->GetAchievement(i).SetDirtyFlag(Achievement::DirtyFlags::Conditions);
    }
}

API void CCONV _RA_DoAchievementsFrame()
{
    if (ra::services::ServiceLocator::Get<ra::data::UserContext>().IsLoggedIn() && g_pActiveAchievements != nullptr)
    {
        g_pActiveAchievements->Test();
        ra::services::ServiceLocator::GetMutable<ra::services::ILeaderboardManager>().Test();

        g_MemoryDialog.Invalidate();
    }
}

void CCONV _RA_InstallSharedFunctions(bool(*fpIsActive)(void), void(*fpCauseUnpause)(void), void(*fpRebuildMenu)(void), void(*fpEstimateTitle)(char*), void(*fpResetEmulation)(void), void(*fpLoadROM)(const char*))
{
    void(*fpCausePause)(void) = nullptr;
    _RA_InstallSharedFunctionsExt(fpIsActive, fpCauseUnpause, fpCausePause, fpRebuildMenu, fpEstimateTitle, fpResetEmulation, fpLoadROM);
}

void CCONV _RA_InstallSharedFunctionsExt(bool(*fpIsActive)(void), void(*fpCauseUnpause)(void), void(*fpCausePause)(void), void(*fpRebuildMenu)(void), void(*fpEstimateTitle)(char*), void(*fpResetEmulation)(void), void(*fpLoadROM)(const char*))
{
    //	NB. Must be called from within DLL
    _RA_GameIsActive = fpIsActive;
    _RA_CauseUnpause = fpCauseUnpause;
    _RA_CausePause = fpCausePause;
    _RA_RebuildMenu = fpRebuildMenu;
    _RA_GetEstimatedGameTitle = fpEstimateTitle;
    _RA_ResetEmulation = fpResetEmulation;
    _RA_LoadROM = fpLoadROM;
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

std::string _TimeStampToString(time_t nTime)
{
    char buffer[64];
    ctime_s(buffer, 64, &nTime);
    return std::string(buffer);
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

BOOL CanCausePause() noexcept
{
    return (_RA_CausePause != nullptr);
}
