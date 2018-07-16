#include "RA_Core.h"

#include "RA_Achievement.h"
#include "RA_AchievementSet.h"
#include "RA_AchievementOverlay.h"
#include "RA_BuildVer.h"
#include "RA_CodeNotes.h"
#include "RA_Defs.h"
#include "RA_GameData.h"
#include "RA_httpthread.h"
#include "RA_ImageFactory.h"
#include "RA_Interface.h"
#include "RA_md5factory.h"
#include "RA_MemManager.h"
#include "RA_PopupWindows.h"
#include "RA_Resource.h"
#include "RA_RichPresence.h"
#include "RA_User.h"

#include "RA_Dlg_AchEditor.h"
#include "RA_Dlg_Achievement.h"
#include "RA_Dlg_AchievementsReporter.h"
#include "RA_Dlg_GameLibrary.h"
#include "RA_Dlg_GameTitle.h"
#include "RA_Dlg_Login.h"
#include "RA_Dlg_Memory.h"
#include "RA_Dlg_RichPresence.h"
#include "RA_Dlg_RomChecksum.h"
#include "RA_Dlg_MemBookmark.h"

#include "services\IConfiguration.hh"
#include "services\ILeaderboardManager.hh"
#include "services\Initialization.hh"
#include "services\ServiceLocator.hh"

#include "services\impl\LeaderboardManager.hh" // for SubmitEntry callback

#include <locale>
#include <memory>
#include <direct.h>
#include <fstream>
#include <io.h>		//	_access()
#include <atlbase.h> // CComPtr
#ifdef WIN32_LEAN_AND_MEAN
#include <ShellAPI.h>
#endif // WIN32_LEAN_AND_MEAN

std::string g_sKnownRAVersion;
std::string g_sHomeDir;
std::string g_sROMDirLocation;
std::string g_sCurrentROMMD5;	//	Internal

HMODULE g_hThisDLLInst = nullptr;
HINSTANCE g_hRAKeysDLL = nullptr;
HWND g_RAMainWnd = nullptr;
EmulatorID g_EmulatorID = EmulatorID::UnknownEmulator;	//	Uniquely identifies the emulator
ConsoleID g_ConsoleID = ConsoleID::UnknownConsoleID;	//	Currently active Console ID
const char* g_sGetLatestClientPage = nullptr;
const char* g_sClientVersion = nullptr;
const char* g_sClientName = nullptr;
const char* g_sClientDownloadURL = nullptr;
const char* g_sClientEXEName = nullptr;
bool g_bRAMTamperedWith = false;

namespace {
const unsigned int PROCESS_WAIT_TIME = 100;
unsigned int g_nProcessTimer = 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
        g_hThisDLLInst = hModule;
    return TRUE;
}

API const char* CCONV _RA_IntegrationVersion()
{
    return RA_INTEGRATION_VERSION;
}

API const char* CCONV _RA_HostName()
{
    static std::string sHostName;
    if (sHostName.empty())
    {
        std::ifstream fHost("host.txt", std::ifstream::in);
        if (fHost.good())
            fHost >> sHostName;

        if (sHostName.empty())
            sHostName = "retroachievements.org";
    }

    return sHostName.c_str();
}

static void InitCommon(HWND hMainHWND, /*enum EmulatorID*/int nEmulatorID, const char* sClientVer)
{
    //	Ensure all required directories are created:
    if (DirectoryExists(RA_DIR_BASE) == FALSE)
        _mkdir(RA_DIR_BASE);
    if (DirectoryExists(RA_DIR_BADGE) == FALSE)
        _mkdir(RA_DIR_BADGE);
    if (DirectoryExists(RA_DIR_DATA) == FALSE)
        _mkdir(RA_DIR_DATA);
    if (DirectoryExists(RA_DIR_USERPIC) == FALSE)
        _mkdir(RA_DIR_USERPIC);
    if (DirectoryExists(RA_DIR_OVERLAY) == FALSE)	//	It should already, really...
        _mkdir(RA_DIR_OVERLAY);
    if (DirectoryExists(RA_DIR_BOOKMARKS) == FALSE)
        _mkdir(RA_DIR_BOOKMARKS);


    g_EmulatorID = static_cast<EmulatorID>(nEmulatorID);
    g_RAMainWnd = hMainHWND;
    //g_hThisDLLInst

    RA_LOG(__FUNCTION__ " Init called! ID: %d, ClientVer: %s\n", nEmulatorID, sClientVer);

    switch (g_EmulatorID)
    {
        case RA_Gens:
            g_ConsoleID = MegaDrive;
            g_sGetLatestClientPage = "LatestRAGensVersion.html";
            g_sClientVersion = sClientVer;
            g_sClientName = "RAGens_REWiND";
            g_sClientDownloadURL = "RAGens.zip";
            g_sClientEXEName = "RAGens.exe";
            break;
        case RA_Project64:
            g_ConsoleID = N64;
            g_sGetLatestClientPage = "LatestRAP64Version.html";
            g_sClientVersion = sClientVer;
            g_sClientName = "RAP64";
            g_sClientDownloadURL = "RAP64.zip";
            g_sClientEXEName = "RAP64.exe";
            break;
        case RA_Snes9x:
            g_ConsoleID = SNES;
            g_sGetLatestClientPage = "LatestRASnesVersion.html";
            g_sClientVersion = sClientVer;
            g_sClientName = "RASnes9X";
            g_sClientDownloadURL = "RASnes9X.zip";
            g_sClientEXEName = "RASnes9X.exe";
            break;
        case RA_VisualboyAdvance:
            g_ConsoleID = GB;
            g_sGetLatestClientPage = "LatestRAVBAVersion.html";
            g_sClientVersion = sClientVer;
            g_sClientName = "RAVisualBoyAdvance";
            g_sClientDownloadURL = "RAVBA.zip";
            g_sClientEXEName = "RAVisualBoyAdvance.exe";
            break;
        case RA_Nester:
        case RA_FCEUX:
            g_ConsoleID = NES;
            g_sGetLatestClientPage = "LatestRANESVersion.html";
            g_sClientVersion = sClientVer;
            g_sClientName = "RANes";
            g_sClientDownloadURL = "RANes.zip";
            g_sClientEXEName = "RANes.exe";
            break;
        case RA_PCE:
            g_ConsoleID = PCEngine;
            g_sGetLatestClientPage = "LatestRAPCEVersion.html";
            g_sClientVersion = sClientVer;
            g_sClientName = "RAPCE";
            g_sClientDownloadURL = "RAPCE.zip";
            g_sClientEXEName = "RAPCE.exe";
            break;
        case RA_Libretro:
            g_ConsoleID = Atari2600;
            g_sGetLatestClientPage = "LatestRALibretroVersion.html";
            g_sClientVersion = sClientVer;
            g_sClientName = "RALibretro";
            g_sClientDownloadURL = "RALibretro.zip";
            g_sClientEXEName = "RALibretro.exe";
            break;
        case RA_Meka:
            g_ConsoleID = MasterSystem;
            g_sGetLatestClientPage = "LatestRAMekaVersion.html";
            g_sClientVersion = sClientVer;
            g_sClientName = "RAMeka";
            g_sClientDownloadURL = "RAMeka.zip";
            g_sClientEXEName = "RAMeka.exe";
            break;
        default:
            g_ConsoleID = UnknownConsoleID;
            g_sClientVersion = sClientVer;
            g_sClientName = "";
            break;
    }

    if (g_sClientName != nullptr)
    {
        RA_LOG("(found as: %s)\n", g_sClientName);
    }

    TCHAR buffer[2048];
    GetCurrentDirectory(2048, buffer);
    g_sHomeDir = ra::Narrow(buffer);
    g_sHomeDir.append("\\");

    RA_LOG(__FUNCTION__ " - storing \"%s\" as home dir\n", g_sHomeDir.c_str());

    g_sROMDirLocation[0] = '\0';

    ra::services::Initialization::RegisterServices(g_sHomeDir, g_sClientName);

    auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    RAUsers::LocalUser().SetUsername(pConfiguration.GetUsername());
    RAUsers::LocalUser().SetToken(pConfiguration.GetApiToken());

    RAWeb::SetUserAgentString();
    RAWeb::RA_InitializeHTTPThreads();

    //////////////////////////////////////////////////////////////////////////
    //	Dialogs:
    g_MemoryDialog.Init();

    //////////////////////////////////////////////////////////////////////////
    //	Initialize All AchievementSets
    g_pCoreAchievements = new AchievementSet(Core);
    g_pUnofficialAchievements = new AchievementSet(Unofficial);
    g_pLocalAchievements = new AchievementSet(Local);
    g_pActiveAchievements = g_pCoreAchievements;

    //////////////////////////////////////////////////////////////////////////
    //	Image rendering: Setup image factory and overlay
    InitializeUserImageFactory(g_hThisDLLInst);
    g_AchievementOverlay.Initialize(g_hThisDLLInst);

    //////////////////////////////////////////////////////////////////////////
    //	Setup min required directories:
    SetCurrentDirectory(NativeStr(g_sHomeDir).c_str());
}

API BOOL CCONV _RA_InitOffline(HWND hMainHWND, /*enum EmulatorID*/int nEmulatorID, const char* sClientVer)
{
    InitCommon(hMainHWND, nEmulatorID, sClientVer);
    return TRUE;
}

API BOOL CCONV _RA_InitI(HWND hMainHWND, /*enum EmulatorID*/int nEmulatorID, const char* sClientVer)
{
    InitCommon(hMainHWND, nEmulatorID, sClientVer);

    //////////////////////////////////////////////////////////////////////////
    //	Update news:
    PostArgs args;
    args['c'] = std::to_string(6);
    RAWeb::CreateThreadedHTTPRequest(RequestNews, args);

    //////////////////////////////////////////////////////////////////////////
    //	Attempt to fetch latest client version:
    args.clear();
    args['c'] = std::to_string(g_ConsoleID);
    RAWeb::CreateThreadedHTTPRequest(RequestLatestClientPage, args);	//	g_sGetLatestClientPage

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
    ra::services::ServiceLocator::Get<ra::services::IConfiguration>().Save();

    SAFE_DELETE(g_pCoreAchievements);
    SAFE_DELETE(g_pUnofficialAchievements);
    SAFE_DELETE(g_pLocalAchievements);

    RAWeb::RA_KillHTTPThreads();

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

    if (g_RichPresenceDialog.GetHWND() != nullptr)
    {
        DestroyWindow(g_RichPresenceDialog.GetHWND());
        g_RichPresenceDialog.InstallHWND(nullptr);
    }

    if (g_MemBookmarkDialog.GetHWND() != nullptr)
    {
        DestroyWindow(g_MemBookmarkDialog.GetHWND());
        g_MemBookmarkDialog.InstallHWND(nullptr);
    }

    CoUninitialize();

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

API int CCONV _RA_HardcoreModeIsActive()
{
    auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    return pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore);
}

API int CCONV _RA_OnLoadNewRom(const BYTE* pROM, unsigned int nROMSize)
{
    static std::string sMD5NULL = RAGenerateMD5(nullptr, 0);

    g_sCurrentROMMD5 = RAGenerateMD5(pROM, nROMSize);
    RA_LOG("Loading new ROM... MD5 is %s\n", (g_sCurrentROMMD5 == sMD5NULL) ? "Null" : g_sCurrentROMMD5.c_str());

    ASSERT(g_MemManager.NumMemoryBanks() > 0);

    //	Go ahead and load: RA_ConfirmLoadNewRom has allowed it.
    //	TBD: local DB of MD5 to ra::GameIDs here
    ra::GameID nGameID = 0;
    if (pROM != nullptr)
    {
        //	Fetch the gameID from the DB here:
        PostArgs args;
        args['u'] = RAUsers::LocalUser().Username();
        args['t'] = RAUsers::LocalUser().Token();
        args['m'] = g_sCurrentROMMD5;

        Document doc;
        if (RAWeb::DoBlockingRequest(RequestGameID, args, doc))
        {
            nGameID = static_cast<ra::GameID>(doc["GameID"].GetUint());
            if (nGameID == 0)	//	Unknown
            {
                RA_LOG("Could not recognise game with MD5 %s\n", g_sCurrentROMMD5.c_str());
                char buffer[64];
                ZeroMemory(buffer, 64);
                RA_GetEstimatedGameTitle(buffer);
                std::string sEstimatedGameTitle(buffer);
                Dlg_GameTitle::DoModalDialog(g_hThisDLLInst, g_RAMainWnd, g_sCurrentROMMD5, sEstimatedGameTitle, nGameID);
            }
            else
            {
                RA_LOG("Successfully looked up game with ID %d\n", nGameID);
            }
        }
        else
        {
            //	Some other fatal error... panic?
            ASSERT(!"Unknown error from requestgameid.php");

            std::ostringstream oss;
            oss << "Game not loaded.\nError from " << _RA_HostName() << "!";
            MessageBox(g_RAMainWnd, NativeStr(oss.str()).c_str(), TEXT("Error returned!"), MB_OK | MB_ICONERROR);
        }
    }

    //g_PopupWindows.Clear(); //TBD

    g_bRAMTamperedWith = false;
    ra::services::ServiceLocator::GetMutable<ra::services::ILeaderboardManager>().Clear();
    g_PopupWindows.LeaderboardPopups().Reset();

    if (nGameID != 0)
    {
        if (RAUsers::LocalUser().IsLoggedIn())
        {
            //	Delete Core and Unofficial Achievements so they are redownloaded every time:
            g_pCoreAchievements->Clear();
            g_pUnofficialAchievements->Clear();
            g_pLocalAchievements->Clear();

            g_pCoreAchievements->DeletePatchFile(nGameID);
            g_pUnofficialAchievements->DeletePatchFile(nGameID);

            AchievementSet::FetchFromWebBlocking(nGameID);

            g_pCoreAchievements->LoadFromFile(nGameID);
            g_pUnofficialAchievements->LoadFromFile(nGameID);
            g_pLocalAchievements->LoadFromFile(nGameID);

            RAUsers::LocalUser().PostActivity(PlayerStartedPlaying);
        }
    }
    else
    {
        g_pCoreAchievements->Clear();
        g_pUnofficialAchievements->Clear();
        g_pLocalAchievements->Clear();
    }

    auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (!pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
    {
        if (pConfiguration.IsFeatureEnabled(ra::services::Feature::Leaderboards))
        {
            g_PopupWindows.AchievementPopups().AddMessage(
                MessagePopup("Playing in Softcore Mode", "Leaderboard submissions will be canceled.", PopupInfo));
        }
        else
        {
            g_PopupWindows.AchievementPopups().AddMessage(
                MessagePopup("Playing in Softcore Mode", "", PopupInfo));
        }
    }

    g_AchievementsDialog.OnLoad_NewRom(nGameID);
    g_AchievementEditorDialog.OnLoad_NewRom();
    g_MemoryDialog.OnLoad_NewRom();
    g_AchievementOverlay.OnLoad_NewRom();
    g_MemBookmarkDialog.OnLoad_NewRom();

    g_nProcessTimer = 0;

    return 0;
}

API void CCONV _RA_OnReset()
{
    g_pActiveAchievements->Reset();
    ra::services::ServiceLocator::GetMutable<ra::services::ILeaderboardManager>().Reset();
    g_PopupWindows.LeaderboardPopups().Reset();

    g_nProcessTimer = 0;
}

API void CCONV _RA_InstallMemoryBank(int nBankID, void* pReader, void* pWriter, int nBankSize)
{
    g_MemManager.AddMemoryBank(static_cast<size_t>(nBankID), (_RAMByteReadFn*)pReader, (_RAMByteWriteFn*)pWriter, static_cast<size_t>(nBankSize));
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

static bool RA_OfferNewRAUpdate(const char* sNewVer)
{
    std::ostringstream oss;
    oss << "Would you like to update?\n\n"
        << "A new version of " << g_sClientName << " is available for download at " << _RA_HostName() << ".\n\n"
        << "Current version: " << g_sClientVersion << "\n"
        << "New version: " << sNewVer;

    if (MessageBox(g_RAMainWnd, NativeStr(oss.str()).c_str(), TEXT("Update available!"), MB_YESNO | MB_ICONINFORMATION) == IDYES)
    {
        //SetCurrentDirectory( g_sHomeDir );
        //FetchBinaryFromWeb( g_sClientEXEName );
        //
        //char sBatchUpdater[2048];
        //sprintf_s( sBatchUpdater, 2048,
        //	"@echo off\r\n"
        //	"taskkill /IM %s\r\n"
        //	"del /f %s\r\n"
        //	"rename %s.new %s%s.exe\r\n"
        //	"start %s%s.exe\r\n"
        //	"del \"%%~f0\"\r\n",
        //	g_sClientEXEName,
        //	g_sClientEXEName,
        //	g_sClientEXEName,
        //	g_sClientName,
        //	sNewVer,
        //	g_sClientName,
        //	sNewVer );

        //_WriteBufferToFile( "BatchUpdater.bat", sBatchUpdater, strlen( sBatchUpdater ) );

        //ShellExecute( nullptr,
        //	"open",
        //	"BatchUpdater.bat",
        //	nullptr,
        //	nullptr,
        //	SW_SHOWNORMAL ); 

        std::ostringstream oss2;
        oss2 << "http://" << _RA_HostName() << "/download.php";
        ShellExecute(nullptr,
            TEXT("open"),
            NativeStr(oss2.str()).c_str(),
            nullptr,
            nullptr,
            SW_SHOWNORMAL);

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

API int CCONV _RA_HandleHTTPResults()
{
    WaitForSingleObject(RAWeb::Mutex(), INFINITE);

    RequestObject* pObj = RAWeb::PopNextHttpResult();
    while (pObj != nullptr)
    {
        if (pObj->GetResponse().size() > 0)
        {
            Document doc;
            BOOL bJSONParsedOK = FALSE;

            if (pObj->GetRequestType() == RequestBadge)
            {
                //	Ignore...
            }
            else
            {
                bJSONParsedOK = pObj->ParseResponseToJSON(doc);
            }

            switch (pObj->GetRequestType())
            {
                case RequestLogin:
                    RAUsers::LocalUser().HandleSilentLoginResponse(doc);
                    break;

                case RequestBadge:
                {
                    SetCurrentDirectory(NativeStr(g_sHomeDir).c_str());
                    const std::string& sBadgeURI = pObj->GetData();
                    _WriteBufferToFile(RA_DIR_BADGE + sBadgeURI + ".png", pObj->GetResponse());

                    /* This block seems unnecessary. --GD
                    for( size_t i = 0; i < g_pActiveAchievements->NumAchievements(); ++i )
                    {
                        Achievement& ach = g_pActiveAchievements->GetAchievement( i );
                        if( ach.BadgeImageURI().compare( 0, 5, sBadgeURI, 0, 5 ) == 0 )
                        {
                            //	Re-set this badge image
                            //	NB. This is already a non-modifying op
                            ach.SetBadgeImage( sBadgeURI );
                        }
                    }*/

                    g_AchievementEditorDialog.UpdateSelectedBadgeImage();	//	Is this necessary if it's no longer selected?
                }
                break;

                case RequestBadgeIter:
                    g_AchievementEditorDialog.GetBadgeNames().OnNewBadgeNames(doc);
                    break;

                case RequestUserPic:
                    RAUsers::OnUserPicDownloaded(*pObj);
                    break;

                case RequestScore:
                {
                    ASSERT(doc["Success"].GetBool());
                    if (doc["Success"].GetBool() && doc.HasMember("User") && doc.HasMember("Score"))
                    {
                        const std::string& sUser = doc["User"].GetString();
                        unsigned int nScore = doc["Score"].GetUint();
                        RA_LOG("%s's score: %d", sUser.c_str(), nScore);

                        if (sUser.compare(RAUsers::LocalUser().Username()) == 0)
                        {
                            RAUsers::LocalUser().SetScore(nScore);
                        }
                        else
                        {
                            //	Find friend? Update this information?
                            RAUsers::GetUser(sUser)->SetScore(nScore);
                        }
                    }
                    else
                    {
                        ASSERT(!"RequestScore bad response!?");
                        RA_LOG("RequestScore bad response!?");
                    }
                }
                break;

                case RequestLatestClientPage:
                {
                    if (doc.HasMember("LatestVersion"))
                    {
                        const std::string& sReply = doc["LatestVersion"].GetString();
                        if (sReply.substr(0, 2).compare("0.") == 0)
                        {
                            long nValServer = std::strtol(sReply.c_str() + 2, nullptr, 10);
                            long nValKnown = std::strtol(g_sKnownRAVersion.c_str() + 2, nullptr, 10);
                            long nValCurrent = std::strtol(g_sClientVersion + 2, nullptr, 10);

                            if (nValKnown < nValServer && nValCurrent < nValServer)
                            {
                                //	Update available:
                                RA_OfferNewRAUpdate(sReply.c_str());

                                //	Update the last version I've heard of:
                                g_sKnownRAVersion = sReply;
                            }
                            else
                            {
                                RA_LOG("Latest Client already up to date: server 0.%d, current 0.%d\n", nValServer, nValCurrent);
                            }
                        }
                    }
                    else
                    {
                        ASSERT(!"RequestLatestClientPage responded, but 'LatestVersion' cannot be found!");
                        RA_LOG("RequestLatestClientPage responded, but 'LatestVersion' cannot be found?");
                    }
                }
                break;

                case RequestSubmitAwardAchievement:
                {
                    //	Response to an achievement being awarded:
                    ra::AchievementID nAchID = static_cast<ra::AchievementID>(doc["AchievementID"].GetUint());
                    const Achievement* pAch = g_pCoreAchievements->Find(nAchID);
                    if (pAch == nullptr)
                        pAch = g_pUnofficialAchievements->Find(nAchID);
                    if (pAch != nullptr)
                    {
                        if (!doc.HasMember("Error"))
                        {
                            g_PopupWindows.AchievementPopups().AddMessage(
                                MessagePopup("Achievement Unlocked",
                                    pAch->Title() + " (" + std::to_string(pAch->Points()) + ")",
                                    PopupMessageType::PopupAchievementUnlocked,
                                    pAch->BadgeImage()));
                            g_AchievementsDialog.OnGet_Achievement(*pAch);

                            RAUsers::LocalUser().SetScore(doc["Score"].GetUint());
                        }
                        else
                        {
                            g_PopupWindows.AchievementPopups().AddMessage(
                                MessagePopup("Achievement Unlocked (Error)",
                                    pAch->Title() + " (" + std::to_string(pAch->Points()) + ")",
                                    PopupMessageType::PopupAchievementError,
                                    pAch->BadgeImage()));
                            g_AchievementsDialog.OnGet_Achievement(*pAch);

                            g_PopupWindows.AchievementPopups().AddMessage(
                                MessagePopup("Error submitting achievement:",
                                    doc["Error"].GetString())); //?

                  //MessageBox( HWnd, buffer, "Error!", MB_OK|MB_ICONWARNING );
                        }
                    }
                    else
                    {
                        ASSERT(!"RequestSubmitAwardAchievement responded, but cannot find achievement ID!");
                        RA_LOG("RequestSubmitAwardAchievement responded, but cannot find achievement with ID %d", nAchID);
                    }
                }
                break;

                case RequestNews:
                    SetCurrentDirectory(NativeStr(g_sHomeDir).c_str());
                    _WriteBufferToFile(RA_NEWS_FILENAME, doc);
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

                case RequestUnlocks:
                    AchievementSet::OnRequestUnlocks(doc);
                    //sprintf_s( sMessage, 512, " You have %d of %d achievements unlocked. ", nNumUnlocked, m_nNumAchievements );
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
    if (RAUsers::LocalUser().IsLoggedIn())
    {
        // TODO: Use the literals once PR 23 is accepted
        AppendMenu(hRA, MF_STRING, IDM_RA_FILES_LOGOUT, TEXT("Log&out"));
        AppendMenu(hRA, MF_SEPARATOR, UINT_PTR{}, nullptr);
        AppendMenu(hRA, MF_STRING, IDM_RA_OPENUSERPAGE, TEXT("Open my &User Page"));

        UINT nGameFlags = MF_STRING;
        //if( g_pActiveAchievements->ra::GameID() == 0 )	//	Disabled til I can get this right: Snes9x doesn't call this?
        //	nGameFlags |= (MF_GRAYED|MF_DISABLED);


        auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();

        // TODO: Replace UINT_PTR{} with the _z literal after PR #23 gets accepted
        AppendMenu(hRA, nGameFlags, IDM_RA_OPENGAMEPAGE, TEXT("Open this &Game's Page"));
        AppendMenu(hRA, MF_SEPARATOR, UINT_PTR{}, nullptr);
        AppendMenu(hRA, pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore) ? MF_CHECKED : MF_UNCHECKED, IDM_RA_HARDCORE_MODE, TEXT("&Hardcore Mode"));
        AppendMenu(hRA, MF_SEPARATOR, UINT_PTR{}, nullptr);

        AppendMenu(hRA, MF_POPUP, reinterpret_cast<UINT_PTR>(hRA_LB), TEXT("Leaderboards"));
        AppendMenu(hRA_LB, pConfiguration.IsFeatureEnabled(ra::services::Feature::Leaderboards) ? MF_CHECKED : MF_UNCHECKED, IDM_RA_TOGGLELEADERBOARDS, TEXT("Enable &Leaderboards"));
        AppendMenu(hRA_LB, MF_SEPARATOR, UINT_PTR{}, nullptr);
        AppendMenu(hRA_LB, pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardNotifications) ? MF_CHECKED : MF_UNCHECKED, IDM_RA_TOGGLE_LB_NOTIFICATIONS, TEXT("Display Challenge Notification"));
        AppendMenu(hRA_LB, pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardCounters) ? MF_CHECKED : MF_UNCHECKED, IDM_RA_TOGGLE_LB_COUNTER, TEXT("Display Time/Score Counter"));
        AppendMenu(hRA_LB, pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardScoreboards) ? MF_CHECKED : MF_UNCHECKED, IDM_RA_TOGGLE_LB_SCOREBOARD, TEXT("Display Rank Scoreboard"));

        AppendMenu(hRA, MF_SEPARATOR, UINT_PTR{}, nullptr);
        AppendMenu(hRA, MF_STRING, IDM_RA_FILES_ACHIEVEMENTS, TEXT("Achievement &Sets"));
        AppendMenu(hRA, MF_STRING, IDM_RA_FILES_ACHIEVEMENTEDITOR, TEXT("Achievement &Editor"));
        AppendMenu(hRA, MF_STRING, IDM_RA_FILES_MEMORYFINDER, TEXT("&Memory Inspector"));
        AppendMenu(hRA, MF_STRING, IDM_RA_PARSERICHPRESENCE, TEXT("Rich &Presence Monitor"));
        AppendMenu(hRA, MF_SEPARATOR, UINT_PTR{}, nullptr);
        AppendMenu(hRA, MF_STRING, IDM_RA_REPORTBROKENACHIEVEMENTS, TEXT("&Report Broken Achievements"));
        AppendMenu(hRA, MF_STRING, IDM_RA_GETROMCHECKSUM, TEXT("Get ROM &Checksum"));
        AppendMenu(hRA, MF_STRING, IDM_RA_SCANFORGAMES, TEXT("Scan &for games"));
    }
    else
    {
        AppendMenu(hRA, MF_STRING, IDM_RA_FILES_LOGIN, TEXT("&Login to RA"));
    }

    AppendMenu(hRA, MF_SEPARATOR, UINT_PTR{}, nullptr);
    AppendMenu(hRA, MF_STRING, IDM_RA_FILES_CHECKFORUPDATE, TEXT("&Check for Emulator Update"));

    return hRA;
}

API void CCONV _RA_UpdateAppTitle(const char* sMessage)
{
    std::ostringstream sstr;
    sstr << std::string(g_sClientName) << " - ";

    // only copy the first two parts of the version string to the title bar: 0.12.7.1 => 0.12
    const char* ptr = g_sClientVersion;
    while (*ptr && *ptr != '.')
        sstr << *ptr++;
    if (*ptr)
    {
        do
        {
            sstr << *ptr++;
        } while (*ptr && *ptr != '.');
    }

    if (sMessage != nullptr && *sMessage)
        sstr << " - " << sMessage;

    if (RAUsers::LocalUser().IsLoggedIn())
        sstr << " - " << RAUsers::LocalUser().Username();

    if (strcmp(_RA_HostName(), "retroachievements.org") != 0)
        sstr << " [" << _RA_HostName() << "]";

    SetWindowText(g_RAMainWnd, NativeStr(sstr.str()).c_str());
}

//	##BLOCKING##
static void RA_CheckForUpdate()
{
    PostArgs args;
    args['c'] = std::to_string(g_ConsoleID);

    std::string Response;
    if (RAWeb::DoBlockingRequest(RequestLatestClientPage, args, Response))
    {
        std::string sReply = std::move(Response);
        if (sReply.length() > 2 && sReply.at(0) == '0' && sReply.at(1) == '.')
        {
            //	Ignore g_sKnownRAVersion: check against g_sRAVersion
            unsigned long nLocalVersion = std::strtoul(g_sClientVersion + 2, nullptr, 10);
            unsigned long nServerVersion = std::strtoul(sReply.c_str() + 2, nullptr, 10);

            if (nLocalVersion < nServerVersion)
            {
                RA_OfferNewRAUpdate(sReply.c_str());
            }
            else
            {
                //	Up to date
                char buffer[1024];
                sprintf_s(buffer, 1024, "You have the latest version of %s: 0.%02d", g_sClientEXEName, nServerVersion);
                MessageBox(g_RAMainWnd, NativeStr(buffer).c_str(), TEXT("Up to date"), MB_OK);
            }
        }
        else
        {
            //	Error in download
            std::ostringstream oss;
            oss << "Unexpected response from " << _RA_HostName() << ".";
            MessageBox(g_RAMainWnd, NativeStr(oss.str()).c_str(), TEXT("Error!"), MB_OK | MB_ICONERROR);
        }
    }
    else
    {
        //	Could not connect
        std::ostringstream oss;
        oss << "Could not connect to " << _RA_HostName() << ".\n" <<
            "Please check your connection settings or RA forums!";
        MessageBox(g_RAMainWnd, NativeStr(oss.str()).c_str(), TEXT("Error!"), MB_OK | MB_ICONERROR);
    }
}

void _FetchGameHashLibraryFromWeb()
{
    PostArgs args;
    args['c'] = std::to_string(g_ConsoleID);
    args['u'] = RAUsers::LocalUser().Username();
    args['t'] = RAUsers::LocalUser().Token();
    std::string Response;
    if (RAWeb::DoBlockingRequest(RequestHashLibrary, args, Response))
        _WriteBufferToFile(RA_GAME_HASH_FILENAME, Response);
}

void _FetchGameTitlesFromWeb()
{
    PostArgs args;
    args['c'] = std::to_string(g_ConsoleID);
    args['u'] = RAUsers::LocalUser().Username();
    args['t'] = RAUsers::LocalUser().Token();
    std::string Response;
    if (RAWeb::DoBlockingRequest(RequestGamesList, args, Response))
        _WriteBufferToFile(RA_GAME_LIST_FILENAME, Response);
}

void _FetchMyProgressFromWeb()
{
    PostArgs args;
    args['c'] = std::to_string(g_ConsoleID);
    args['u'] = RAUsers::LocalUser().Username();
    args['t'] = RAUsers::LocalUser().Token();
    std::string Response;
    if (RAWeb::DoBlockingRequest(RequestAllProgress, args, Response))
        _WriteBufferToFile(RA_MY_PROGRESS_FILENAME, Response);
}

void RestoreWindowPosition(HWND hDlg, const char* sDlgKey, bool bToRight, bool bToBottom)
{
    auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    ra::ui::Position oPosition = pConfiguration.GetWindowPosition(std::string(sDlgKey));
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
            RA_Dlg_Login::DoModalLogin();
            ra::services::ServiceLocator::Get<ra::services::IConfiguration>().Save();
            break;

        case IDM_RA_FILES_LOGOUT:
            RAUsers::LocalUser().Clear();
            g_PopupWindows.Clear();
            ra::services::ServiceLocator::Get<ra::services::IConfiguration>().Save();
            _RA_UpdateAppTitle();

            MessageBox(g_RAMainWnd, TEXT("You are now logged out."), TEXT("Info"), MB_OK);	//	##BLOCKING##
            _RA_RebuildMenu();
            break;

        case IDM_RA_FILES_CHECKFORUPDATE:
            RA_CheckForUpdate();
            break;

        case IDM_RA_HARDCORE_MODE:
        {
            auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
            pConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore,
                !pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));

            _RA_ResetEmulation();
            _RA_OnReset();

            g_PopupWindows.Clear();

            ra::GameID nGameID = g_pCurrentGameData->GetGameID();
            if (nGameID != 0)
            {
                //	Delete Core and Unofficial Achievements so it is redownloaded every time:
                g_pCoreAchievements->DeletePatchFile(nGameID);
                g_pUnofficialAchievements->DeletePatchFile(nGameID);

                g_pCoreAchievements->Clear();
                g_pUnofficialAchievements->Clear();
                g_pLocalAchievements->Clear();

                //	Fetch remotely then load again from file
                AchievementSet::FetchFromWebBlocking(nGameID);

                g_pCoreAchievements->LoadFromFile(nGameID);
                g_pUnofficialAchievements->LoadFromFile(nGameID);
                g_pLocalAchievements->LoadFromFile(nGameID);
            }

            _RA_RebuildMenu();
        }
        break;

        case IDM_RA_REPORTBROKENACHIEVEMENTS:
            Dlg_AchievementsReporter::DoModalDialog(g_hThisDLLInst, g_RAMainWnd);
            break;

        case IDM_RA_GETROMCHECKSUM:
        {
            RA_Dlg_RomChecksum::DoModalDialog();
            //MessageBox( nullptr, ( std::string( "Current ROM MD5: " ) + g_sCurrentROMMD5 ).c_str(), "Get ROM Checksum", MB_OK );
            break;
        }
        //if( g_pActiveAchievements->NumAchievements() == 0 )
        //	MessageBox( nullptr, "No ROM loaded!", "Error", MB_OK );
        //else
        //	MessageBox( nullptr, ( std::string( "Current ROM MD5: " ) + g_sCurrentROMMD5 ).c_str(), "Get ROM Checksum", MB_OK );
        //break;

        case IDM_RA_OPENUSERPAGE:
            if (RAUsers::LocalUser().IsLoggedIn())
            {
                std::ostringstream oss;
                oss << "http://" << _RA_HostName() << "/User/" << RAUsers::LocalUser().Username();
                ShellExecute(nullptr,
                    TEXT("open"),
                    NativeStr(oss.str()).c_str(),
                    nullptr,
                    nullptr,
                    SW_SHOWNORMAL);
            }
            break;

        case IDM_RA_OPENGAMEPAGE:
            if (g_pCurrentGameData->GetGameID() != 0)
            {
                std::ostringstream oss;
                oss << "http://" << _RA_HostName() << "/Game/" << g_pCurrentGameData->GetGameID();
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
            if (g_pCurrentGameData->GetGameID() != 0)
            {
                char sRichPresenceFile[1024];
                sprintf_s(sRichPresenceFile, 1024, "%s%d-Rich.txt", RA_DIR_DATA, g_pCurrentGameData->GetGameID());

                //	Then install it
                g_RichPresenceInterpretter.ParseRichPresenceFile(sRichPresenceFile);

                if (g_RichPresenceDialog.GetHWND() == nullptr)
                    g_RichPresenceDialog.InstallHWND(CreateDialog(g_hThisDLLInst, MAKEINTRESOURCE(IDD_RA_RICHPRESENCE), g_RAMainWnd, &Dlg_RichPresence::s_RichPresenceDialogProc));
                if (g_RichPresenceDialog.GetHWND() != nullptr)
                    ShowWindow(g_RichPresenceDialog.GetHWND(), SW_SHOW);

                g_RichPresenceDialog.StartMonitoring();
            }
            else
            {
                MessageBox(nullptr, TEXT("No ROM loaded!"), TEXT("Error!"), MB_ICONWARNING);
            }
            break;

        case IDM_RA_TOGGLELEADERBOARDS:
        {
            auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
            bool bLeaderboardsActive = !pConfiguration.IsFeatureEnabled(ra::services::Feature::Leaderboards);
            pConfiguration.SetFeatureEnabled(ra::services::Feature::Leaderboards, bLeaderboardsActive);

            std::string msg;
            msg += "Leaderboards are now ";
            msg += (bLeaderboardsActive ? "enabled." : "disabled.");
            msg += "\nNB. You may need to load ROM again to re-enable leaderboards.";

            MessageBox(nullptr, NativeStr(msg).c_str(), TEXT("Success"), MB_OK);

            if (!bLeaderboardsActive)
                g_PopupWindows.LeaderboardPopups().Reset();

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

API void CCONV _RA_AttemptLogin(bool bBlocking)
{
    RAUsers::LocalUser().AttemptLogin(bBlocking);
}

API void CCONV _RA_OnSaveState(const char* sFilename)
{
    //	Save State is being allowed by app (user was warned!)
    if (RAUsers::LocalUser().IsLoggedIn())
    {
        auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
        if (pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
        {
            pConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
            RA_RebuildMenu();
            //RA_ResetEmulation();
        }

        if (!g_bRAMTamperedWith)
        {
            g_pCoreAchievements->SaveProgress(sFilename);
        }
    }
}

API void CCONV _RA_OnLoadState(const char* sFilename)
{
    //	Save State is being allowed by app (user was warned!)
    if (RAUsers::LocalUser().IsLoggedIn())
    {
        auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
        if (pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
        {
            pConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);

            MessageBox(nullptr, TEXT("Savestates are not allowed during Hardcore Mode!"), TEXT("Warning!"), MB_OK | MB_ICONEXCLAMATION);

            RA_RebuildMenu();
            RA_ResetEmulation();
        }

        g_pCoreAchievements->LoadProgress(sFilename);
        ra::services::ServiceLocator::GetMutable<ra::services::ILeaderboardManager>().Reset();
        g_PopupWindows.LeaderboardPopups().Reset();
        g_MemoryDialog.Invalidate();
        g_nProcessTimer = PROCESS_WAIT_TIME;
    }
}

API void CCONV _RA_DoAchievementsFrame()
{
    if (RAUsers::LocalUser().IsLoggedIn())
    {
        if (g_nProcessTimer >= PROCESS_WAIT_TIME)
        {
            g_pActiveAchievements->Test();
            ra::services::ServiceLocator::GetMutable<ra::services::ILeaderboardManager>().Test();
        }
        else
            g_nProcessTimer++;

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

API bool _RA_UserLoggedIn()
{
    return(RAUsers::LocalUser().IsLoggedIn() == TRUE);
}


//////////////////////////////////////////////////////////////////////////

BOOL _ReadTil(const char nChar, char buffer[], unsigned int nSize, DWORD* pCharsReadOut, FILE* pFile)
{
    char pNextChar = '\0';
    memset(buffer, '\0', nSize);

    //	Read title:
    (*pCharsReadOut) = 0;
    do
    {
        if (fread(&pNextChar, sizeof(char), 1, pFile) == 0)
            break;

        buffer[(*pCharsReadOut)++] = pNextChar;
    } while (pNextChar != nChar && (*pCharsReadOut) < nSize && !feof(pFile));

    //return ( !feof( pFile ) );
    return ((*pCharsReadOut) > 0);
}

char* _ReadStringTil(char nChar, char*& pOffsetInOut, BOOL bTerminate)
{
    char* pStartString = pOffsetInOut;

    while ((*pOffsetInOut) != '\0' && (*pOffsetInOut) != nChar)
        pOffsetInOut++;

    if (bTerminate)
        (*pOffsetInOut) = '\0';

    pOffsetInOut++;

    return (pStartString);
}

void _ReadStringTil(std::string& value, char nChar, const char*& pSource)
{
    const char* pStartString = pSource;

    while (*pSource != '\0' && *pSource != nChar)
        pSource++;

    value.assign(pStartString, pSource - pStartString);
    pSource++;
}

void _WriteBufferToFile(const std::string& sFileName, const Document& doc)
{
    SetCurrentDirectory(NativeStr(g_sHomeDir).c_str());
    FILE* pf = nullptr;
    if (fopen_s(&pf, sFileName.c_str(), "wb") == 0)
    {
        FileStream fs(pf);
        Writer<FileStream> writer(fs);
        doc.Accept(writer);
        fclose(pf);
    }
}

void _WriteBufferToFile(const std::string& sFileName, const std::string& raw)
{
    SetCurrentDirectory(NativeStr(g_sHomeDir).c_str());

    using FileH = std::unique_ptr<FILE, decltype(&std::fclose)>;
#pragma warning(push)
#pragma warning(disable : 4996) // Deprecation from Microsoft
    FileH myFile{ std::fopen(sFileName.c_str(), "wb"), std::fclose };
#pragma warning(pop)

    std::fwrite(static_cast<const void*>(raw.c_str()), sizeof(char), raw.length(), myFile.get());
}


void _WriteBufferToFile(const char* sFile, std::streamsize nBytes)
{
    SetCurrentDirectory(NativeStr(g_sHomeDir).c_str());

    using FileH = std::unique_ptr<FILE, decltype(&std::fclose)>;
#pragma warning(push)
#pragma warning(disable : 4996) // Deprecation from Microsoft
    FileH myFile{ std::fopen(sFile, "wb"), std::fclose };
#pragma warning(pop)
    
    auto sBuffer{ std::make_unique<char[]>(static_cast<size_t>(ra::to_unsigned(nBytes))) };
    std::fwrite(static_cast<void* const>(sBuffer.get()), sizeof(char), std::strlen(sBuffer.get()), myFile.get());
}

char* _MallocAndBulkReadFileToBuffer(const char* sFilename, long& nFileSizeOut)
{
    SetCurrentDirectory(NativeStr(g_sHomeDir).c_str());
    FILE* pf = nullptr;
    fopen_s(&pf, sFilename, "r");
    if (pf == nullptr)
        return nullptr;

    //	Calculate filesize
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
    char* pRawFileOut = (char*)malloc((nFileSizeOut + 1) * sizeof(char));
    ZeroMemory(pRawFileOut, nFileSizeOut + 1);

    fread(pRawFileOut, nFileSizeOut, sizeof(char), pf);
    fclose(pf);

    return pRawFileOut;
}

std::string _TimeStampToString(time_t nTime)
{
    char buffer[64];
    ctime_s(buffer, 64, &nTime);
    return std::string(buffer);
}

BOOL _FileExists(const std::string& sFileName)
{
    FILE* pf = nullptr;
    fopen_s(&pf, sFileName.c_str(), "rb");
    if (pf != nullptr)
    {
        fclose(pf);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

std::string GetFolderFromDialog()
{
    std::string sRetVal;
	CComPtr<IFileOpenDialog> pDlg;

    HRESULT hr = HRESULT{};
	if (SUCCEEDED(hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pDlg))))
    {
        pDlg->SetOptions(FOS_PICKFOLDERS);
		if (SUCCEEDED(hr = pDlg->Show(nullptr)))
        {
            CComPtr<IShellItem> pItem;
			if (SUCCEEDED(hr = pDlg->GetResult(&pItem)))
            {
                LPWSTR pStr{ nullptr };
				if (SUCCEEDED(hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pStr)))
                {
                    sRetVal = ra::Narrow(pStr);
                    // https://msdn.microsoft.com/en-us/library/windows/desktop/bb761140(v=vs.85).aspx
                    CoTaskMemFree(static_cast<LPVOID>(pStr));
                    pStr = nullptr;
                }
				pItem.Release();
            }
        }
		pDlg.Release();
    }
    return sRetVal;
}

BOOL RemoveFileIfExists(const std::string& sFilePath)
{
    SetCurrentDirectory(NativeStr(g_sHomeDir).c_str());

    if (_access(sFilePath.c_str(), 06) != -1)	//	06= Read/write permission
    {
        if (remove(sFilePath.c_str()) == -1)
        {
            //	Remove issues?
            ASSERT(!"Could not remove patch file!?");
            return FALSE;
        }
        else
        {
            //	Successfully deleted
            return TRUE;
        }
    }
    else
    {
        //	Doesn't exist (or no permissions?)
        return TRUE;
    }
}

BOOL CanCausePause()
{
    return (_RA_CausePause != nullptr);
}
