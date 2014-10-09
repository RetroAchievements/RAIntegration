#if defined RA_VBA
#include "stdafx.h"
#endif

#include <Windows.h>
#include <stdio.h>
#include <direct.h>
#include <shlobj.h>
#include <string>
#include <map>

#include "RA_Interface.h"
#include "RA_Defs.h"
#include "RA_Core.h"
#include "RA_Resource.h"
#include "RA_Achievement.h"
#include "RA_Leaderboard.h"
#include "RA_MemManager.h"

#include "RA_Dlg_Login.h"
#include "RA_Dlg_Memory.h"
#include "RA_Dlg_GameTitle.h"
#include "RA_Dlg_Achievement.h"
#include "RA_Dlg_AchEditor.h"
#include "RA_Dlg_AchievementsReporter.h"
#include "RA_Dlg_GameLibrary.h"

#include "RA_AchievementOverlay.h"
#include "RA_httpthread.h"
#include "RA_ImageFactory.h"
#include "RA_PopupWindows.h"
#include "RA_md5factory.h"
#include "RA_User.h"
#include "RA_RichPresence.h"


char g_sKnownRAVersion[50];
char g_sHomeDir[2048];
char g_sROMDirLocation[2048];
char g_sCurrentROMMD5[33];
HMODULE g_hThisDLLInst;
HINSTANCE g_hRAKeysDLL;
HWND g_RAMainWnd;
EmulatorID g_EmulatorID;					//	Uniquely identifies the emulator
unsigned int g_ConsoleID = -1;				//	Currently active Console ID
const char* g_sGetLatestClientPage = NULL;	
const char* g_sClientVersion = NULL;		
const char* g_sClientName = NULL;			
const char* g_sClientDownloadURL = NULL;	
const char* g_sClientEXEName = NULL;
bool g_bRAMTamperedWith = false;
bool g_hardcoreModeActive = false;
unsigned int g_nNumHTTPThreads = 15;

const char* (*g_fnKeysVersion)(void) = NULL;
void (*g_fnDoValidation)(char sBufferOut[50], const char* sUser, const char* sToken, const unsigned int nID) = NULL;

//	local func def.
bool _InstallKeys();

BOOL APIENTRY DllMain( HMODULE hModule, DWORD dwReason, LPVOID lpReserved )
{
	if (dwReason == DLL_PROCESS_ATTACH) 
		g_hThisDLLInst = hModule;
	return TRUE;
}

API const char* CCONV _RA_IntegrationVersion()
{
	return RA_INTEGRATION_VERSION;
}

API BOOL CCONV _RA_InitI( HWND hMainHWND, /*enum EmulatorID*/int nEmulatorID, const char* sClientVer )
{
	//	Ensure all required directories are created:
	if( DirectoryExists( RA_DIR_BASE ) == FALSE )
		_mkdir( RA_DIR_BASE );
	if( DirectoryExists( RA_DIR_BADGE ) == FALSE )
		_mkdir( RA_DIR_BADGE );
	if( DirectoryExists( RA_DIR_DATA ) == FALSE )
		_mkdir( RA_DIR_DATA );
	if( DirectoryExists( RA_DIR_OVERLAY ) == FALSE )	//	It should already, really...
		_mkdir( RA_DIR_OVERLAY );

	g_EmulatorID = (EmulatorID)nEmulatorID;
	g_RAMainWnd = hMainHWND;
	//g_hThisDLLInst

	RA_LOG( __FUNCTION__ " Init called! ID: %d, ClientVer: %s\n", nEmulatorID, sClientVer );

	switch( g_EmulatorID )
	{
	case RA_Gens:
		g_ConsoleID				= 1;
		g_sGetLatestClientPage	= "LatestRAGensVersion.html";
		g_sClientVersion		= sClientVer;
		g_sClientName			= "RAGens_REWiND";
		g_sClientDownloadURL	= "RAGens.zip";
		g_sClientEXEName		= "RAGens.exe";
		break;
	case RA_Project64:
		g_ConsoleID				= 2;
		g_sGetLatestClientPage	= "LatestRAP64Version.html";
		g_sClientVersion		= sClientVer;
		g_sClientName			= "RAP64";
		g_sClientDownloadURL	= "RAP64.zip";
		g_sClientEXEName		= "RAP64.exe";
		break;
	case RA_Snes9x:
		g_ConsoleID				= 3;
		g_sGetLatestClientPage	= "LatestRASnesVersion.html";
		g_sClientVersion		= sClientVer;
		g_sClientName			= "RASnes9X";
		g_sClientDownloadURL	= "RASnes9X.zip";
		g_sClientEXEName		= "RASnes9X.exe";
		break;
	case RA_VisualboyAdvance:
		g_ConsoleID				= 4;
		g_sGetLatestClientPage	= "LatestRAVBAVersion.html";
		g_sClientVersion		= sClientVer;
		g_sClientName			= "RAVisualBoyAdvance";
		g_sClientDownloadURL	= "RAVBA.zip";
		g_sClientEXEName		= "RAVisualBoyAdvance.exe";
		break;
	case RA_Nester:
	case RA_FCEUX:
		g_ConsoleID				= 7;
		g_sGetLatestClientPage	= "LatestRANESVersion.html";
		g_sClientVersion		= sClientVer;
		g_sClientName			= "RANes";
		g_sClientDownloadURL	= "RANes.zip";
		g_sClientEXEName		= "RANes.exe";
		break;
	case RA_PCE:
		g_ConsoleID				= 8;
		g_sGetLatestClientPage	= "LatestRAPCEVersion.html";
		g_sClientVersion		= sClientVer;
		g_sClientName			= "RAPCE";
		g_sClientDownloadURL	= "RAPCE.zip";
		g_sClientEXEName		= "RAPCE.exe";
		break;
	default:
		break;
	}

	if( g_sClientName != NULL )
	{
		RA_LOG( "(found as: %s)\n", g_sClientName );
	}


	GetCurrentDirectory( 2048, g_sHomeDir );
	strcat_s( g_sHomeDir, "\\" );
	RA_LOG( "%s - storing \"%s\" as home dir\n", __FUNCTION__, g_sHomeDir );

	g_sROMDirLocation[0] = '\0';

	_RA_LoadPreferences();

	bool bInstalled = FALSE;

	int nMBReply = IDNO;
	do
	{
		if( _InstallKeys() == FALSE )
		{
			nMBReply = MessageBox( NULL, 
				"Cannot load RA_Keys.dll!\n"
				"Download fresh RetroAchievement keys file from RetroAchievements.org?", 
				"Warning", MB_YESNO );
			
			if( nMBReply == IDYES )
			{
				char buffer[65536];
				DWORD nBytesRead = 0;
				if( DoBlockingHttpGet( "RA_Keys.dll", buffer, 65536, &nBytesRead ) )
					_WriteBufferToFile( "RA_Keys.dll", buffer, nBytesRead );
			}
		}
		else
		{
			bInstalled = TRUE;
			break;
		}

	} while( nMBReply == IDYES );

	//	DO NOT CONTINUE if they have opted NOT to get the latest keys!
	if( !bInstalled )
	{
		RA_LOG( __FUNCTION__ " Not opted to get RA_Keys.dll... shutting down!\n" );

		_RA_Shutdown();
		return FALSE;
	}

	RA_InitializeHTTPThreads();

	//////////////////////////////////////////////////////////////////////////
	//	Memory Manager
	g_MemManager.Init();
	g_MemManager.Reset( CMP_SZ_8BIT );

	//	Dialogs:
	g_MemoryDialog.Init();

	//////////////////////////////////////////////////////////////////////////
	//	Initialize All AchievementSets
	CoreAchievements = new AchievementSet( AT_CORE );
	CoreAchievements->Clear();

	UnofficialAchievements = new AchievementSet( AT_UNOFFICIAL );
	UnofficialAchievements->Clear();

	LocalAchievements = new AchievementSet( AT_USER );
	LocalAchievements->Clear();

	g_pActiveAchievements = CoreAchievements;

	//////////////////////////////////////////////////////////////////////////
	//	Image rendering: Setup image factory and overlay
	InitializeUserImageFactory( g_hThisDLLInst );
	g_AchievementOverlay.Initialize( g_hThisDLLInst );

	//////////////////////////////////////////////////////////////////////////
	//	Setup min required directories:
	SetCurrentDirectory( g_sHomeDir );
	
	//////////////////////////////////////////////////////////////////////////
	//	Update news:
	char sPostVars[256];
	sprintf_s( sPostVars, 256, "n=%d&a=1", AchievementOverlay::m_nMaxNews );
	CreateHTTPRequestThread( "requestnews.php", sPostVars, HTTPRequest_Post, 0, NULL );

	//////////////////////////////////////////////////////////////////////////
	//	Attempt to fetch latest client version:
	CreateHTTPRequestThread( g_sGetLatestClientPage, "", HTTPRequest_Get, 0, NULL );

	return TRUE;
}

API int CCONV _RA_Shutdown()
{
	_RA_SavePreferences();

	if( CoreAchievements != NULL )
	{
		delete( CoreAchievements );
		CoreAchievements = NULL;
	}

	if( UnofficialAchievements != NULL )
	{
		delete( UnofficialAchievements );
		UnofficialAchievements = NULL;
	}

	if( LocalAchievements != NULL )
	{
		delete( LocalAchievements );
		LocalAchievements = NULL;
	}

	RA_KillHTTPThreads();

	if( g_AchievementsDialog.GetHWND() != NULL )
	{
		DestroyWindow( g_AchievementsDialog.GetHWND() );
		g_AchievementsDialog.InstallHWND( NULL );
	}

	if( g_AchievementEditorDialog.GetHWND() != NULL )
	{
		DestroyWindow( g_AchievementEditorDialog.GetHWND() );
		g_AchievementEditorDialog.InstallHWND( NULL );
	}

	if( g_MemoryDialog.GetHWND() != NULL )
	{
		DestroyWindow( g_MemoryDialog.GetHWND() );
		g_MemoryDialog.InstallHWND( NULL );
	}
	
	if( g_GameLibrary.GetHWND() != NULL )
	{
		DestroyWindow( g_GameLibrary.GetHWND() );
		g_GameLibrary.InstallHWND( NULL );
	}

	g_GameLibrary.KillThread();
	
	return 0;
}

API BOOL CCONV _RA_ConfirmLoadNewRom( BOOL bQuittingApp )
{
	//	Returns true if we can go ahead and load the new rom.
	char buffer[1024];

	int nResult = IDYES;

	const char* sCurrentAction = bQuittingApp ? "quit now" : "load a new ROM";
	const char* sNextAction = bQuittingApp ? "Are you sure?" : "Continue load?";

	if( CoreAchievements->HasUnsavedChanges() )
	{
		sprintf_s( buffer, 1024, 
			"You have unsaved changes in the Core Achievements set.\n"
			"If you %s you will lose these changes.\n"
			"%s", sCurrentAction, sNextAction );

		nResult = MessageBox( g_RAMainWnd, buffer, "Warning", MB_ICONWARNING|MB_YESNO );
	}
	if( UnofficialAchievements->HasUnsavedChanges() )
	{
		sprintf_s( buffer, 1024, 
			"You have unsaved changes in the Unofficial Achievements set.\n"
			"If you %s you will lose these changes.\n"
			"%s", sCurrentAction, sNextAction );

		nResult = MessageBox( g_RAMainWnd, buffer, "Warning", MB_ICONWARNING|MB_YESNO );
	}
	if( LocalAchievements->HasUnsavedChanges() )
	{
		sprintf_s( buffer, 1024, 
			"You have unsaved changes in the Local Achievements set.\n"
			"If you %s you will lose these changes.\n"
			"%s", sCurrentAction, sNextAction );

		nResult = MessageBox( g_RAMainWnd, buffer, "Warning", MB_ICONWARNING|MB_YESNO );
	}

	return ( nResult == IDYES );
}

API int CCONV _RA_SetConsoleID( unsigned int nConsoleID )
{
	g_ConsoleID = nConsoleID;
	return 0;
}

API int _RA_HardcoreModeIsActive()
{
	return g_hardcoreModeActive;
}

API int _RA_HTTPGetRequestExists( const char* sPageName )
{
	return HTTPRequestExists( sPageName );
}

API int CCONV _RA_OnLoadNewRom( BYTE* pROM, unsigned int nROMSize, BYTE* pRAM, unsigned int nRAMSize, BYTE* pRAMExtra, unsigned int nRAMExtraSize )
{
	ZeroMemory( g_sCurrentROMMD5, 33 );
	if( pROM != NULL && nROMSize > 0 )
	{
		md5_GenerateMD5Raw( pROM, nROMSize, g_sCurrentROMMD5 );
	}

	g_MemManager.InstallRAM( pRAM, nRAMSize, pRAMExtra, nRAMExtraSize );

	//	Go ahead and load: RA_ConfirmLoadNewRom has allowed it.

	//	TBD: local DB of MD5 to GameIDs here

	unsigned int nGameID = 0;
	if( strcmp( g_sCurrentROMMD5, "" ) != 0 )
	{
		//	Fetch the gameID from the DB here:
		char sPostString[1024];
		sprintf_s( sPostString, 1024, "u=%s&m=%s", g_LocalUser.m_sUsername, g_sCurrentROMMD5 );

		char sBufferOut[1024];
		ZeroMemory( sBufferOut, 1024 );

		DWORD nBytesRead = 0;
		if( DoBlockingHttpPost( "requestgameid.php", sPostString, sBufferOut, 1024, &nBytesRead ) )
		{
			if( strncmp( sBufferOut, "OK:", 3 ) == 0 )
			{
				nGameID = strtol( sBufferOut+3, NULL, 10 );
			}
			else if( strncmp( sBufferOut, "UNRECOGNISED", strlen( "UNRECOGNISED" ) ) == 0 )
			{
				//	Unknown game: harass user to select/submit details, then reload!

				char sEstimatedGameTitle[64];
				ZeroMemory( sEstimatedGameTitle, 64 );
				RA_GetEstimatedGameTitle( sEstimatedGameTitle );
				nGameID = Dlg_GameTitle::DoModalDialog( g_hThisDLLInst, g_RAMainWnd, g_sCurrentROMMD5, sEstimatedGameTitle );
			}
			else
			{
				//	Some other fatal error... panic?
				assert( !"Unknown error from requestgameid.php" );

				char msgboxText[8192];
				sprintf_s( msgboxText, 8192, "Error from " RA_HOST "!\n%s", sBufferOut );

				MessageBox( g_RAMainWnd, msgboxText, "Error returned!", MB_OK );
			}
		}
	}

	//g_PopupWindows.Clear(); //TBD

	g_bRAMTamperedWith = false;
	g_LeaderboardManager.Clear();
	g_PopupWindows.LeaderboardPopups().Reset();

	if( nGameID != 0 )
	{ 
		if( g_LocalUser.m_bIsLoggedIn )
		{
			//	Delete Core and Unofficial Achievements so it is redownloaded every time:
			AchievementSet::DeletePatchFile( AT_CORE, nGameID );
			AchievementSet::DeletePatchFile( AT_UNOFFICIAL, nGameID );

			CoreAchievements->Load( nGameID );
			UnofficialAchievements->Load( nGameID );
			LocalAchievements->Load( nGameID );

			g_LocalUser.PostActivity( ActivityType_StartPlaying );
		}
	}
	else
	{
		CoreAchievements->Clear();
		UnofficialAchievements->Clear();
		LocalAchievements->Clear();
	}

	g_AchievementsDialog.OnLoad_NewRom( nGameID );	//	TBD
	g_AchievementEditorDialog.OnLoad_NewRom();
	g_MemoryDialog.OnLoad_NewRom();

	g_AchievementOverlay.OnLoad_NewRom();

	return 0;
}


void FetchBinaryFromWeb( const char* sFilename )
{
	const unsigned int nBufferSize = (3*1024*1024);	//	3mb enough?

	char* buffer = new char[nBufferSize];	
	if( buffer != NULL )
	{
		char sAddr[1024];
		sprintf_s( sAddr, 1024, "/files/%s", sFilename );
		char sOutput[1024];
		sprintf_s( sOutput, 1024, "%s%s.new", g_sHomeDir, sFilename );

		DWORD nBytesRead = 0;
		if( DoBlockingHttpGet( sAddr, buffer, nBufferSize, &nBytesRead ) )
			_WriteBufferToFile( sOutput, buffer, nBytesRead );

		delete[] ( buffer );
		buffer = NULL;
	}
}

API BOOL CCONV _RA_OfferNewRAUpdate( const char* sNewVer )
{
	char buffer[1024];
	sprintf_s( buffer, 1024, "Update available!\n"
		"A new version of %s is available for download at " RA_HOST ".\n\n"
		"Would you like to update?\n\n"
		"Current version:%s\n"
		"New version:%s\n",
		g_sClientName,
		g_sClientVersion,
		sNewVer );

	//	Update last known version:
	//strcpy_s( g_sKnownRAVersion, 50, sNewVer );

	if( MessageBox( g_RAMainWnd, buffer, "Update available!", MB_YESNO ) == IDYES )
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

		//ShellExecute( NULL,
		//	"open",
		//	"BatchUpdater.bat",
		//	NULL,
		//	NULL,
		//	SW_SHOWNORMAL ); 

		ShellExecute( NULL,
			"open",
			"http://www.retroachievements.org/download.php",
			NULL,
			NULL,
			SW_SHOWNORMAL );

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


API int CCONV _RA_HandleHTTPResults()
{
	WaitForSingleObject( g_hHTTPMutex, INFINITE );

	RequestObject* pObj = LastHttpResults.PopNextItem();
	while( pObj	!= NULL )
	{
		if( pObj->m_pfCallbackOnReceive != NULL )
		{
			//	Do a mainthread cb
			//pObj->m_pfCallbackOnReceive( pObj );
		}
		else
		{
			switch( pObj->m_nReqType )
			{
			case HTTPRequest_Get:
				//	Get some data. What do you want with it?
				if( strncmp( pObj->m_sRequestPageName, "Badge", 5 ) == 0 )
				{
					SetCurrentDirectory( g_sHomeDir );

					char sTargetFile[1024];
					sprintf_s( sTargetFile, 1024, RA_DIR_BADGE "%s", pObj->m_sRequestPageName+6 );

					_WriteBufferToFile( sTargetFile, pObj->m_sResponse, pObj->m_nBytesRead );

					//	Fetch minimal name
					char buffer[16];
					sprintf_s( buffer, 16, pObj->m_sRequestPageName+6 );
					buffer[5] = '\0';

					for( unsigned int iter = 0; iter < g_pActiveAchievements->m_nNumAchievements; ++iter )
					{
						Achievement* pAch = &g_pActiveAchievements->m_Achievements[iter];
						if( strcmp( pAch->BadgeImageFilename(), buffer ) == 0 )
						{
							//	Re-set this badge image
							//	NB. This is already a non-modifying op
							pAch->SetBadgeImage( buffer );
						}
					}

					g_AchievementEditorDialog.UpdateSelectedBadgeImage();
				}
				else if( strcmp( pObj->m_sRequestPageName, g_sGetLatestClientPage ) == 0 )
				{
					if( strlen( pObj->m_sResponse ) > 2 && pObj->m_sResponse[0] == '0' && pObj->m_sResponse[1] == '.' )
					{
						long nValServer = strtol( pObj->m_sResponse+2, NULL, 10 );
						long nValKnown = strtol( g_sKnownRAVersion+2, NULL, 10 );
						long nValCurrent = strtol( g_sClientVersion+2, NULL, 10 );

						if( nValKnown < nValServer && nValCurrent < nValServer )
						{
							//	Update available:
							_RA_OfferNewRAUpdate( pObj->m_sResponse );

							//	Update the last version I've heard of:
							strcpy_s( g_sKnownRAVersion, 50, pObj->m_sResponse );
						}
					}
					else
					{
						//	Ignore: nonsense response from server
					}
				}


				break;
			case HTTPRequest_Post:
				//	Post some data. Is the reply important?
				if( strcmp( pObj->m_sRequestPageName, "requestachievement.php" ) == 0 )
				{
					if( pObj->m_bResponse == TRUE )
					{
						char sTitle[64];
						char sSubtitle[1024];
						Achievement* pAch = &g_pActiveAchievements->m_Achievements[ pObj->m_nUserRef ];

						if( strncmp( pObj->m_sResponse, "OK:", 3 ) == 0 )
						{
							sprintf_s( sTitle, 64, " Achievement Unlocked " );
							sprintf_s( sSubtitle, 1024, " %s (%d) ", pAch->Title(), pAch->Points() );
							g_PopupWindows.AchievementPopups().AddMessage( sTitle, sSubtitle, MSG_ACHIEVEMENT_UNLOCKED, pAch->BadgeImage() );
							g_AchievementsDialog.OnGet_Achievement( pObj->m_nUserRef );

							//	First param passed back will be score

							//	first four chars after OK: are FBOK, FBDC, FBNA, or FBER
							g_LocalUser.m_nLatestScore = strtol( &( pObj->m_sResponse[3+4] ), NULL, 10 );
						}
						else
						{
							sprintf_s( sTitle, 64, " Achievement Unlocked (Error) " );
							sprintf_s( sSubtitle, 1024, " %s (%d) ", pAch->Title(), pAch->Points() );
							g_PopupWindows.AchievementPopups().AddMessage( sTitle, sSubtitle, MSG_ACHIEVEMENT_ERROR, pAch->BadgeImage() );
							g_AchievementsDialog.OnGet_Achievement( pObj->m_nUserRef );

							g_PopupWindows.AchievementPopups().AddMessage( "Error with submission, please check!", pObj->m_sResponse, MSG_INFO ); //?

							//MessageBox( HWnd, buffer, "Error!", MB_OK|MB_ICONWARNING );
						}
					}
				}
				else if( strcmp( pObj->m_sRequestPageName, "requestunlocks.php" ) == 0 )
				{
					//	Done blocking instead
				}
				else if( strcmp( pObj->m_sRequestPageName, "requestlogin.php" ) == 0 )
				{
					if( pObj->m_bResponse == TRUE )
					{
						if( strncmp( pObj->m_sResponse, "FAILED", 6 ) == 0 )
						{
							char buffer[4096];
							sprintf_s( buffer, 4096, 
								"%s failed:\n\n"
								"Response from server:\n"
								"%s",
								( pObj->m_nUserRef == 1 ) ? "Auto-login" : "Login",
								pObj->m_sResponse );

							MessageBox( NULL, buffer, "Error in login", MB_OK );
							g_LocalUser.Clear();
						}
						else if( strncmp( pObj->m_sResponse, "OK:", 3 ) == 0 )
						{
							//	will return a valid token : points : messages

							//	fish out the username again
							char* pUserPtr = pObj->m_sRequestPost+2;
							char* sUser = strtok_s( pUserPtr, "&", &pUserPtr );

							//	find the token, points, and num messages
							char* pBuf = pObj->m_sResponse+3;
							char* pToken = strtok_s( pBuf, ":", &pBuf );
							unsigned int nPoints = strtol( pBuf, &pBuf, 10 );
							pBuf++;
							unsigned int nMessages = strtol( pBuf, &pBuf, 10 );
							//pBuf++;

							g_LocalUser.Login( sUser, pToken, g_LocalUser.m_bStoreToken, nPoints, nMessages );
						}
						else
						{
							MessageBox( NULL, "Login failed: please check user/password!", "Error", MB_OK );
							g_LocalUser.Clear();
						}
					}
					else
					{
						MessageBox( NULL, "Login failed: please check user/password!", "Error", MB_OK );
						g_LocalUser.Clear();
					}

					RA_RebuildMenu();
					_RA_UpdateAppTitle();
				}
				else if( strcmp( pObj->m_sRequestPageName, "requestnews.php" ) == 0 )
				{
					if( pObj->m_bResponse )
					{
						SetCurrentDirectory( g_sHomeDir );
						_WriteBufferToFile( RA_DIR_DATA "ra_news.txt", pObj->m_sResponse, pObj->m_nBytesRead );
						g_AchievementOverlay.InstallNewsArticlesFromFile();
					}
				}
				else if( strcmp( pObj->m_sRequestPageName, "requestrichpresence.php" ) == 0 )
				{
					if( pObj->m_bResponse && ( pObj->m_nBytesRead > 0 ) )
					{
						char sRichPresenceFile[1024];
						sprintf_s( sRichPresenceFile, 1024, "%s%d-Rich.txt", RA_DIR_DATA, pObj->m_nUserRef );

						//	Read to file:
						SetCurrentDirectory( g_sHomeDir );
						_WriteBufferToFile( sRichPresenceFile, pObj->m_sResponse, pObj->m_nBytesRead );
						
						//	Then install it
						g_RichPresenceInterpretter.ParseRichPresenceFile( sRichPresenceFile );
					}
				}
				break;
			}
		}

		free( pObj );
		pObj = LastHttpResults.PopNextItem();
	}

	ReleaseMutex( g_hHTTPMutex );

	return 0;
}

//	Following this function, an app should call AppendMenu to associate this submenu.
API HMENU CCONV _RA_CreatePopupMenu()
{
	HMENU hRA = CreatePopupMenu();
	if( g_LocalUser.m_bIsLoggedIn )
	{
		AppendMenu( hRA, MF_STRING, IDM_RA_FILES_LOGOUT, TEXT("Log&out") );
		AppendMenu( hRA, MF_SEPARATOR, NULL, NULL );
		AppendMenu( hRA, MF_STRING, IDM_RA_OPENUSERPAGE, TEXT("Open my &User Page") );

		UINT nGameFlags = MF_STRING;
		//if( g_pActiveAchievements->GameID() == 0 )	//	Disabled til I can get this right: Snes9x doesn't call this?
		//	nGameFlags |= (MF_GRAYED|MF_DISABLED);

		AppendMenu( hRA, nGameFlags, IDM_RA_OPENGAMEPAGE, TEXT("Open this &Game's Page") );
		AppendMenu( hRA, MF_SEPARATOR, NULL, NULL );
		AppendMenu( hRA, g_hardcoreModeActive ? MF_CHECKED : MF_UNCHECKED, IDM_RA_HARDCORE_MODE, TEXT("&Hardcore Mode") );
		AppendMenu( hRA, MF_SEPARATOR, NULL, NULL );
		AppendMenu( hRA, MF_STRING, IDM_RA_FILES_ACHIEVEMENTS, TEXT("Achievement &Sets") );
		AppendMenu( hRA, MF_STRING, IDM_RA_FILES_ACHIEVEMENTEDITOR, TEXT("Achievement &Editor") );
		AppendMenu( hRA, MF_STRING, IDM_RA_FILES_MEMORYFINDER, TEXT("&Memory Inspector") );
		AppendMenu( hRA, MF_STRING, IDM_RA_PARSERICHPRESENCE, TEXT("&Parse Rich Presence script") );
		AppendMenu( hRA, MF_SEPARATOR, NULL, NULL );
		AppendMenu( hRA, MF_STRING, IDM_RA_REPORTBROKENACHIEVEMENTS, TEXT("&Report Broken Achievements") );
		AppendMenu( hRA, MF_STRING, IDM_RA_GETROMCHECKSUM, TEXT("Get ROM &Checksum") );
		AppendMenu( hRA, MF_STRING, IDM_RA_SCANFORGAMES, TEXT("&Scan for games") );
	}
	else
	{
		AppendMenu( hRA, MF_STRING, IDM_RA_FILES_LOGIN, TEXT("&Login to RA") );
	}

	AppendMenu(hRA, MF_SEPARATOR, NULL, NULL);
	AppendMenu(hRA, MF_STRING, IDM_RA_FILES_CHECKFORUPDATE, TEXT("&Check for Emulator Update") );
	
	return hRA;
}

API int CCONV _RA_UpdateAppTitle( const char* sMessage )
{
	char sNewTitle[1024];
	sprintf_s( sNewTitle, 1024, "%s %s", g_sClientName, g_sClientVersion );

	if( sMessage != NULL )
	{
		strcat_s( sNewTitle, 1024, " " );
		strcat_s( sNewTitle, 1024, sMessage );
		strcat_s( sNewTitle, 1024, " " );
	}

	if( g_LocalUser.m_bIsLoggedIn )
	{
		strcat_s( sNewTitle, 1024, " (" );
		strcat_s( sNewTitle, 1024, g_LocalUser.m_sUsername );
		strcat_s( sNewTitle, 1024, ")" );
	}

	SetWindowText( g_RAMainWnd, sNewTitle );

	return 0;
}

API int CCONV _RA_CheckForUpdate()
{
	//	Blocking:
	char sReply[8192];
	ZeroMemory( sReply, 8192 );
	char* sReplyCh = &sReply[0];
	unsigned long nCharsRead = 0;

	if( DoBlockingHttpPost( g_sGetLatestClientPage, "", sReplyCh, 8192, &nCharsRead ) )
	{
		//	Error in download
		if( nCharsRead > 2 && sReplyCh[0] == '0' && sReplyCh[1] == '.' )
		{
			//	Ignore g_sKnownRAVersion: check against g_sRAVersion
			long nCurrent = strtol( g_sClientVersion+2, NULL, 10 );
			long nUpdate = strtol( sReplyCh+2, NULL, 10 );

			char buffer[1024];

			if( nCurrent < nUpdate )
			{
				_RA_OfferNewRAUpdate( sReply );
			}
			else
			{
				sprintf_s( buffer, 1024, "You have the latest version of %s: %s", g_sClientEXEName, sReplyCh );

				//	Up to date
				MessageBox( g_RAMainWnd, buffer, "Up to date", MB_OK );
			}
		}
		else
		{
			//	Error in download
			MessageBox( g_RAMainWnd, 
				"Error in download from " RA_HOST "...\n"
				"Please check your connection settings or RA forums!",
				"Error!", MB_OK );
		}
	}
	else
	{
		//	Could not connect
		MessageBox( g_RAMainWnd, 
			"Could not connect to " RA_HOST "...\n"
			"Please check your connection settings or RA forums!",
			"Error!", MB_OK );
	}

	return 0;
}


//	Load preferences from ra_prefs.cfg
API void CCONV _RA_LoadPreferences()
{
	RA_LOG( __FUNCTION__ " - loading preferences...\n" );

	char buffer[2048];
	sprintf_s( buffer, 2048, "%s\\ra_prefs_%s.cfg", g_sHomeDir, g_sClientName );

	FILE* pConfigFile = NULL;
	fopen_s( &pConfigFile, buffer, "r" );

	//	Test for first-time use:
	if( pConfigFile == NULL )
	{
		RA_LOG( __FUNCTION__ " - no preferences found: showing first-time message!\n" );
		
		char sWelcomeMessage[4096];

		sprintf_s( sWelcomeMessage, 4096, 
			"Welcome! It looks like this is your first time using RetroAchievements.\n\n"
			"Quick Start: Press ESCAPE or 'Back' on your Xbox 360 controller to view the achievement overlay.\n\n" );

		switch( g_EmulatorID )
		{
		case RA_Gens:
			strcat_s( sWelcomeMessage, 4096,
				"Default Keyboard Controls: Use cursor keys, A-S-D are A, B, C, and Return for Start.\n\n" );
			break;
		case RA_VisualboyAdvance:
			strcat_s( sWelcomeMessage, 4096,
				"Default Keyboard Controls: Use cursor keys, Z-X are A and B, A-S are L and R, use Return for Start and Backspace for Select.\n\n" );
			break;
		case RA_Snes9x:
			strcat_s( sWelcomeMessage, 4096,
				"Default Keyboard Controls: Use cursor keys, D-C-S-X are A, B, X, Y, Z-V are L and R, use Return for Start and Space for Select.\n\n" );
			break;
		case RA_FCEUX:
			strcat_s( sWelcomeMessage, 4096,
				"Default Keyboard Controls: Use cursor keys, D-F are B and A, use Return for Start and S for Select.\n\n" );
			break;
		case RA_PCE:
			strcat_s( sWelcomeMessage, 4096,
				"Default Keyboard Controls: Use cursor keys, A-S-D for A, B, C, and Return for Start\n\n" );
			break;
		}

		strcat_s( sWelcomeMessage, 4096, "These defaults can be changed under [Option]->[Joypads].\n\n"
			"If you have any questions, comments or feedback, please visit forum.RetroAchievements.org for more information.\n\n" );

		MessageBox( g_RAMainWnd, 
			sWelcomeMessage,
			"Welcome to RetroAchievements!", MB_OK );

		//	TBD: setup some decent default variables:
		_RA_SavePreferences();
	}
	else
	{
		//	Load and read variables:
		DWORD nCharsRead = 0;
		_ReadTil( '\n', buffer, 2048, &nCharsRead, pConfigFile );	//	'--Username:'
		_ReadTil( '\n', buffer, 2048, &nCharsRead, pConfigFile );	//	

		if( nCharsRead > 0 )
		{
			buffer[nCharsRead-1] = '\0';	//	Turn that endline into an end-string
			g_LocalUser.SetUsername( buffer );
		}

		_ReadTil( '\n', buffer, 2048, &nCharsRead, pConfigFile );	//	'--Token:'
		_ReadTil( '\n', buffer, 2048, &nCharsRead, pConfigFile );	//	

		if( nCharsRead > 0 )
		{
			buffer[nCharsRead-1] = '\0';	//	Turn that endline into an end-string
			strcpy_s( g_LocalUser.m_sToken, 64, buffer );
		}

		_ReadTil( '\n', buffer, 2048, &nCharsRead, pConfigFile );	//	'--HardcoreModeActive:'
		_ReadTil( '\n', buffer, 2048, &nCharsRead, pConfigFile );	//	

		if( nCharsRead > 0 )
		{
			buffer[nCharsRead-1] = '\0';	//	Turn that endline into an end-string
			g_hardcoreModeActive = ( *buffer == '1' );
		}

		_ReadTil( '\n', buffer, 2048, &nCharsRead, pConfigFile );	//	'--NumberHTTPThreads:'
		_ReadTil( '\n', buffer, 2048, &nCharsRead, pConfigFile );	//	

		if( nCharsRead > 0 )
		{
			buffer[nCharsRead-1] = '\0';	//	Turn that endline into an end-string
			g_nNumHTTPThreads = atoi( buffer );

			if( g_nNumHTTPThreads <= 0 )
				g_nNumHTTPThreads = 1;
			else if( g_nNumHTTPThreads > 100 )
				g_nNumHTTPThreads = 100;
		}

		_ReadTil( '\n', buffer, 2048, &nCharsRead, pConfigFile );	//	'--ROMDir:'
		_ReadTil( '\n', buffer, 2048, &nCharsRead, pConfigFile );	//	

		if( nCharsRead > 0 )
		{
			buffer[nCharsRead-1] = '\0';	//	Turn that endline into an end-string
			strcpy_s( g_sROMDirLocation, 2048, buffer );
		}

		fclose( pConfigFile );
	}

	g_GameLibrary.LoadAll();
}

//	Save preferences to ra_prefs.cfg
API void CCONV _RA_SavePreferences()
{
	char buffer[2048];
	sprintf_s( buffer, 2048, "%s\\ra_prefs_%s.cfg", g_sHomeDir, g_sClientName );

	FILE* pConfigFile = NULL;
	fopen_s( &pConfigFile, buffer, "w" );

	if( pConfigFile != NULL )
	{
		//	Save config here:
		fwrite( "--Username:\n", sizeof(char), strlen( "--Username:\n" ), pConfigFile );

		fwrite( g_LocalUser.Username(), sizeof(char), strlen( g_LocalUser.Username() ), pConfigFile );
		fwrite( "\n", sizeof(char), 1, pConfigFile ); 


		fwrite( "--Token:\n", sizeof(char), strlen( "--Token:\n" ), pConfigFile );

		if( g_LocalUser.m_bStoreToken )
		{
			fwrite( g_LocalUser.m_sToken, sizeof(char), strlen( g_LocalUser.m_sToken ), pConfigFile );
		}
		else
		{
			//fwrite( "", sizeof(char), strlen( "" ), pConfigFile );
		}

		fwrite( "\n", sizeof(char), 1, pConfigFile );

		fwrite( "--HardcoreModeActive:\n", sizeof(char), strlen( "--HardcoreModeActive:\n" ), pConfigFile );
		fwrite( g_hardcoreModeActive ? "1" : "0", sizeof(char), 1, pConfigFile ); 
		fwrite( "\n", sizeof(char), 1, pConfigFile ); 

		fwrite( "--NumberHTTPThreads:\n", sizeof(char), strlen( "--NumberHTTPThreads:\n" ), pConfigFile );
		char sNumThreads[256];
		sprintf_s( sNumThreads, 256, "%d", g_nNumHTTPThreads );
		fwrite( sNumThreads, sizeof(char), strlen( sNumThreads ), pConfigFile ); 
		fwrite( "\n", sizeof(char), 1, pConfigFile ); 

		fwrite( "--ROMDir:\n", sizeof(char), strlen( "--ROMDir:\n" ), pConfigFile );
		fwrite( g_sROMDirLocation, sizeof(char), strlen( g_sROMDirLocation ), pConfigFile ); 
		fwrite( "\n", sizeof(char), 1, pConfigFile ); 

		//	Add more parameters here:
		//fwrite( g_LocalUser.Username(), sizeof(char), strlen( g_LocalUser.Username() ), pConfigFile );
		//fwrite( "\n", sizeof(char), strlen( "\n" ), pConfigFile );

		fclose( pConfigFile );
	}
	
	g_GameLibrary.SaveAll();
}

void _FetchGameHashLibraryFromWeb()
{
	const unsigned int BUFFER_SIZE = 1*1024*1024;
	char* buffer = new char[BUFFER_SIZE];
	if( buffer != NULL )
	{
		DWORD nBytesRead = 0;

		char sTarget[256];
		sprintf_s( sTarget, 256, "requesthashlibrary.php?c=%d", g_ConsoleID );

		if( DoBlockingHttpGet( sTarget, buffer, BUFFER_SIZE, &nBytesRead ) )
			_WriteBufferToFile( RA_DIR_DATA "gamehashlibrary.txt", buffer, nBytesRead );
 
		delete[] ( buffer );
		buffer = NULL;
	}
}

void _FetchGameTitlesFromWeb()
{
	const unsigned int BUFFER_SIZE = 1*1024*1024;
	char* buffer = new char[BUFFER_SIZE];
	if( buffer != NULL )
	{
		DWORD nBytesRead = 0;

		char sTarget[256];
		sprintf_s( sTarget, 256, "requestgametitles.php?c=%d", g_ConsoleID );

		if( DoBlockingHttpGet( sTarget, buffer, BUFFER_SIZE, &nBytesRead ) )
			_WriteBufferToFile( RA_DIR_DATA "gametitles.txt", buffer, nBytesRead );
 
		delete[] ( buffer );
		buffer = NULL;
	}
}

void _FetchMyProgressFromWeb()
{
	const unsigned int BUFFER_SIZE = 1*1024*1024;
	char* buffer = new char[BUFFER_SIZE];
	if( buffer != NULL )
	{
		DWORD nBytesRead = 0;

		char sTarget[256];
		sprintf_s( sTarget, 256, "requestallmyprogress.php?u=%s&c=%d", g_LocalUser.Username(), g_ConsoleID );

		if( DoBlockingHttpGet( sTarget, buffer,BUFFER_SIZE, &nBytesRead ) )
			_WriteBufferToFile( RA_DIR_DATA "myprogress.txt", buffer, nBytesRead );
 
		delete[] ( buffer );
		buffer = NULL;
	}
}

API void CCONV _RA_InvokeDialog( LPARAM nID )
{
	switch( nID )
	{
		case IDM_RA_FILES_ACHIEVEMENTS:

			if( g_AchievementsDialog.GetHWND() == NULL )
				g_AchievementsDialog.InstallHWND( CreateDialog( g_hThisDLLInst, MAKEINTRESOURCE(IDD_RA_ACHIEVEMENTS), g_RAMainWnd, g_AchievementsDialog.s_AchievementsProc ) );
			if( g_AchievementsDialog.GetHWND() != NULL )
				ShowWindow( g_AchievementsDialog.GetHWND(), SW_SHOW );
		break;

		case IDM_RA_FILES_ACHIEVEMENTEDITOR:

			if( g_AchievementEditorDialog.GetHWND() == NULL )
				g_AchievementEditorDialog.InstallHWND( CreateDialog( g_hThisDLLInst, MAKEINTRESOURCE(IDD_RA_ACHIEVEMENTEDITOR), g_RAMainWnd, g_AchievementEditorDialog.s_AchievementEditorProc ) );
			if( g_AchievementEditorDialog.GetHWND() != NULL )
				ShowWindow( g_AchievementEditorDialog.GetHWND(), SW_SHOW );
		break;

		case IDM_RA_FILES_MEMORYFINDER:

			if( g_MemoryDialog.GetHWND() == NULL )
				g_MemoryDialog.InstallHWND( CreateDialog( g_hThisDLLInst, MAKEINTRESOURCE(IDD_RA_MEMORY), g_RAMainWnd, g_MemoryDialog.s_MemoryProc ) );
			if( g_MemoryDialog.GetHWND() != NULL )
				ShowWindow( g_MemoryDialog.GetHWND(), SW_SHOW );
		break;
		
		case IDM_RA_FILES_LOGIN:
			{
				DialogBox( g_hThisDLLInst, MAKEINTRESOURCE(IDD_RA_LOGIN), g_RAMainWnd, LoginProc );
				_RA_SavePreferences();
				//RA_RebuildMenu();
			}

			break;

		case IDM_RA_FILES_LOGOUT:

			g_LocalUser.Clear();
			g_PopupWindows.Clear();
			
			_RA_SavePreferences();

			//UpdateAppTitle();
			_RA_UpdateAppTitle();

			MessageBox( g_RAMainWnd, "You are now logged out.", "Info", MB_OK );

			//Reset_Genesis();
			_RA_RebuildMenu();

			break;

		case IDM_RA_FILES_CHECKFORUPDATE:

			_RA_CheckForUpdate();

			break;

		case IDM_RA_HARDCORE_MODE:

			g_hardcoreModeActive = !g_hardcoreModeActive;

			_RA_ResetEmulation();
			
			if( CoreAchievements->GameID() != 0 )
			{
				//	Delete Core and Unofficial Achievements so it is redownloaded every time:
				AchievementSet::DeletePatchFile( AT_CORE, CoreAchievements->GameID() );
				AchievementSet::DeletePatchFile( AT_UNOFFICIAL, CoreAchievements->GameID() );

				CoreAchievements->Load( CoreAchievements->GameID() );
				UnofficialAchievements->Load( CoreAchievements->GameID() );
				LocalAchievements->Load( CoreAchievements->GameID() );
			}

			_RA_RebuildMenu();

			break;

		case IDM_RA_REPORTBROKENACHIEVEMENTS:
			{
				Dlg_AchievementsReporter::DoModalDialog( g_hThisDLLInst, g_RAMainWnd );
			}
			break;

		case IDM_RA_GETROMCHECKSUM:
			{
				if( g_pActiveAchievements->Count() == 0 )
				{
					MessageBox( NULL, "No ROM loaded!", "Error", MB_OK );
				}
				else
				{	char buffer[2048];
					sprintf_s( buffer, "Current ROM MD5: %s", g_sCurrentROMMD5 );
				
					MessageBox( NULL, buffer, "Get ROM Checksum", MB_OK );
				}
			}
			break;

		case IDM_RA_OPENUSERPAGE:
			{
				if( g_LocalUser.m_bIsLoggedIn )
				{
					char buffer[1024];
					sprintf_s( buffer, 1024, "http://www.retroachievements.org/User/%s", g_LocalUser.m_sUsername );

					ShellExecute( NULL,
						"open",
						buffer,
						NULL,
						NULL,
						SW_SHOWNORMAL );
				}
			}
			break;

		case IDM_RA_OPENGAMEPAGE:
			{
				if( g_pActiveAchievements->GameID() != 0 )
				{
					char buffer[1024];
					sprintf_s( buffer, 1024, "http://www.retroachievements.org/Game/%d", g_pActiveAchievements->GameID() );

					ShellExecute( NULL,
						"open",
						buffer,
						NULL,
						NULL,
						SW_SHOWNORMAL );
				}
				else
				{
					MessageBox( NULL, "No ROM loaded!", "Error!", MB_ICONWARNING );
				}
			}
			break;

		case IDM_RA_SCANFORGAMES:
			{
				if( g_sROMDirLocation[0] == '\0' )
				{
					BROWSEINFO bi = { 0 };
					bi.lpszTitle = "Select your ROM directory";
					LPITEMIDLIST pidl = SHBrowseForFolder( &bi );
					if( pidl != 0 )
					{
						// get the name of the folder
						if( SHGetPathFromIDList( pidl, g_sROMDirLocation ) )
						{
							RA_LOG( "Selected Folder: %s\n", g_sROMDirLocation );
						}

						CoTaskMemFree( pidl );
					}
				}

				if( g_sROMDirLocation[0] != '\0' )
				{
					_FetchGameHashLibraryFromWeb();
					_FetchGameTitlesFromWeb();
					_FetchMyProgressFromWeb();

							
					if( g_GameLibrary.GetHWND() == NULL )
						g_GameLibrary.InstallHWND( CreateDialog( g_hThisDLLInst, MAKEINTRESOURCE(IDD_RA_GAMELIBRARY), g_RAMainWnd, &Dlg_GameLibrary::s_GameLibraryProc ) );
					if( g_GameLibrary.GetHWND() != NULL )
						ShowWindow( g_GameLibrary.GetHWND(), SW_SHOW );

					//Dlg_GameLibrary::DoModalDialog( g_hThisDLLInst, g_RAMainWnd );
				}

			}
			break;

		case IDM_RA_PARSERICHPRESENCE:
			{
				if( g_pActiveAchievements->GameID() != 0 )
				{
					char sRichPresenceFile[1024];
					sprintf_s( sRichPresenceFile, 1024, "%s%d-Rich.txt", RA_DIR_DATA, g_pActiveAchievements->GameID() );

					//	Then install it
					g_RichPresenceInterpretter.ParseRichPresenceFile( sRichPresenceFile );

					//	Then fetch immediately
					std::string sRP = g_RichPresenceInterpretter.GetRichPresenceString();

					MessageBox( NULL, sRP.c_str(), "Rich Presence script result", MB_OK );
				}
				else
				{
					MessageBox( NULL, "No ROM loaded!", "Error!", MB_ICONWARNING );
				}
			}
			break;

		default:
			//	Unknown!
			break;
	}
}

API void CCONV _RA_SetPaused( bool bIsPaused )
{
	//	TBD: store this state?? (Rendering?)

	if( bIsPaused )
	{
		g_AchievementOverlay.Activate();
	}
	else
	{
		g_AchievementOverlay.Deactivate();
	}
}

API const char* CCONV _RA_Username()
{
	return g_LocalUser.Username();
}

API void CCONV _RA_AttemptLogin()
{
	g_LocalUser.AttemptLogin();
}

API void CCONV _RA_OnSaveState( const char* sFilename )
{
	if( g_LocalUser.m_bIsLoggedIn )
	{
		if( g_hardcoreModeActive )
		{
			g_hardcoreModeActive = false;
			RA_RebuildMenu();
			//RA_ResetEmulation();
		}

		if( !g_bRAMTamperedWith )
		{
			CoreAchievements->SaveProgress( sFilename );
		}
	}
}

API void CCONV _RA_OnLoadState( const char* sFilename )
{
	if( g_LocalUser.m_bIsLoggedIn )
	{
		if( g_hardcoreModeActive )
		{
			MessageBox( NULL, "Savestates are not allowed during Hardcore Mode!", "Warning!", MB_OK|MB_ICONEXCLAMATION );
			g_hardcoreModeActive = false;
			RA_RebuildMenu();
			RA_ResetEmulation();
		}

		CoreAchievements->LoadProgress( sFilename );
		g_LeaderboardManager.Reset();
		g_PopupWindows.LeaderboardPopups().Reset();
	}
}

API void CCONV _RA_DoAchievementsFrame()
{
	if( g_LocalUser.m_bIsLoggedIn )
	{
		g_pActiveAchievements->Test();
		g_LeaderboardManager.Test();
		g_MemoryDialog.Invalidate();
	}
}

void _RA_InstallSharedFunctions( bool(*fpIsActive)(void), void(*fpCauseUnpause)(void), void(*fpRebuildMenu)(void), void(*fpEstimateTitle)(char*), void(*fpResetEmulation)(void), void(*fpLoadROM)(char*) )
{
	//	NB. Must be called from within DLL
	_RA_GameIsActive			= fpIsActive;
	_RA_CauseUnpause			= fpCauseUnpause;
	_RA_RebuildMenu				= fpRebuildMenu;
	_RA_GetEstimatedGameTitle	= fpEstimateTitle;
	_RA_ResetEmulation			= fpResetEmulation;
	_RA_LoadROM					= fpLoadROM;
}


//////////////////////////////////////////////////////////////////////////

//	Install Keys for achievement validation
bool _InstallKeys()
{
	const int nMINKEYSVER = 01;

	g_hRAKeysDLL = LoadLibrary( "RA_Keys.dll" );
	if( g_hRAKeysDLL == NULL )
		return false;
 
	//	Achievement
	g_fnDoValidation = (void(*)( char sBufferOut[50], const char* sUser, const char* sToken, const unsigned int nID )) GetProcAddress( g_hRAKeysDLL, "GenerateValidation" );
	if( g_fnDoValidation == NULL )
		return false;

	//	Leaderboards //TBD
	//g_fnDoValidation = (void(*)( char sBufferOut[50], const char* sUser, const char* sToken, const unsigned int nID )) GetProcAddress( g_hRAKeysDLL, "GenerateValidation2" );
	//if( g_fnDoValidation == NULL )
	//	return false;

	g_fnKeysVersion = (const char*(*)(void)) GetProcAddress( g_hRAKeysDLL, "KeysVersion" );
	if( g_fnKeysVersion == NULL )
		return false;

	const char* sVer = g_fnKeysVersion();
	int nKeysVer = sVer ? (int)strtol( &sVer[2], NULL, 10 ) : 0;

	return nKeysVer >= nMINKEYSVER;
}

BOOL _ReadTil( const char nChar, char buffer[], unsigned int nSize, DWORD* pCharsReadOut, FILE* pFile )
{
	char pNextChar = '\0';
	memset( buffer, '\0', nSize );

	//	Read title:
	(*pCharsReadOut) = 0;
	do
	{
		if( fread( &pNextChar, sizeof(char), 1, pFile ) == 0 )
			break;

		buffer[(*pCharsReadOut)++] = pNextChar;
	}
	while( pNextChar != nChar && (*pCharsReadOut) < nSize && !feof(pFile) );

	//return ( !feof( pFile ) );
	return ((*pCharsReadOut) > 0);
}

char* _ReadStringTil( char nChar, char*& pOffsetInOut, BOOL bTerminate )
{
	char* pStartString = pOffsetInOut;

	while( (*pOffsetInOut) != '\0' && (*pOffsetInOut) != nChar )
		pOffsetInOut++;

	if( bTerminate )
		(*pOffsetInOut) = '\0';

	pOffsetInOut++;

	return (pStartString);
}

int _WriteBufferToFile( const char* sFile, const char* sBuffer, int nBytes )
{
	FILE* pf;
	if( fopen_s( &pf, sFile, "wb" ) == 0 )
	{
		fwrite( sBuffer, 1, nBytes, pf );
		fclose( pf );
	}

	return 0;
}

char* _MallocAndBulkReadFileToBuffer( const char* sFilename, long& nFileSizeOut )
{
	FILE* pf = NULL;
	fopen_s( &pf, sFilename, "r" );
	if( pf == NULL )
		return NULL;

	//	Calculate filesize
	fseek( pf, 0L, SEEK_END );
	nFileSizeOut = ftell( pf );
	fseek( pf, 0L, SEEK_SET );

	if( nFileSizeOut <= 0 )
	{
		//	No good content in this file.
		fclose( pf );
		return NULL;
	}

	//	malloc() must be managed!
	//	NB. By adding +1, we allow for a single \0 character :)
	char* pRawFileOut = (char*)malloc( ( nFileSizeOut+1 )*sizeof(char) );
	ZeroMemory( pRawFileOut, nFileSizeOut+1 );

	fread( pRawFileOut, nFileSizeOut, sizeof(char), pf );
	fclose( pf );

	return pRawFileOut;
}

