#if defined RA_VBA
#include "stdafx.h"
#endif

#include <Windows.h>
#include <stdio.h>
#include <direct.h>
#include <shlobj.h>
#include <string>
#include <map>
#include <time.h>
#include <sstream>

#include "RA_Interface.h"
#include "RA_Defs.h"
#include "RA_Core.h"
#include "RA_Resource.h"
#include "RA_Achievement.h"
#include "RA_Leaderboard.h"
#include "RA_MemManager.h"
#include "RA_CodeNotes.h"

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



std::string g_sKnownRAVersion;
std::string g_sHomeDir;
std::string g_sROMDirLocation;
std::string g_sCurrentROMMD5;	//	Internal

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
bool g_bLeaderboardsActive = true;
unsigned int g_nNumHTTPThreads = 15;

const char* (*g_fnKeysVersion)(void) = NULL;
void (*g_fnDoValidation)( char sBufferOut[50], const char* sUser, const char* sToken, const unsigned int nID ) = NULL;

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
	if( DirectoryExists( RA_DIR_USERPIC ) == FALSE )
		_mkdir( RA_DIR_USERPIC );
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

	char buffer[2048];
	GetCurrentDirectory( 2048, buffer );
	g_sHomeDir = buffer;
	strcat_s( buffer, "\\" );

	RA_LOG( __FUNCTION__ " - storing \"%s\" as home dir\n", g_sHomeDir.c_str() );

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
				DataStream Response;
				//if( RAWeb::DoBlockingHttpGet( "RA_Keys.dll", Response ) )
				if( RAWeb::DoBlockingHttpPost( "RA_Keys.dll", "", Response ) )
					_WriteBufferToFile( "RA_Keys.dll", Response.data(), Response.size() );
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

	RAWeb::RA_InitializeHTTPThreads();

	//////////////////////////////////////////////////////////////////////////
	//	Memory Manager
	g_MemManager.Init();
	g_MemManager.Reset( CMP_SZ_8BIT );

	//	Dialogs:
	g_MemoryDialog.Init();

	//////////////////////////////////////////////////////////////////////////
	//	Initialize All AchievementSets
	CoreAchievements = new AchievementSet( AchievementSetCore );
	CoreAchievements->Clear();

	UnofficialAchievements = new AchievementSet( AchievementSetUnofficial );
	UnofficialAchievements->Clear();

	LocalAchievements = new AchievementSet( AchievementSetLocal );
	LocalAchievements->Clear();

	g_pActiveAchievements = CoreAchievements;

	//////////////////////////////////////////////////////////////////////////
	//	Image rendering: Setup image factory and overlay
	InitializeUserImageFactory( g_hThisDLLInst );
	g_AchievementOverlay.Initialize( g_hThisDLLInst );

	//////////////////////////////////////////////////////////////////////////
	//	Setup min required directories:
	SetCurrentDirectory( g_sHomeDir.c_str() );
	
	//////////////////////////////////////////////////////////////////////////
	//	Update news:
	PostArgs args;
	args['c'] = std::to_string( 6 );
	RAWeb::CreateThreadedHTTPRequest( RequestNews, args );

	//////////////////////////////////////////////////////////////////////////
	//	Attempt to fetch latest client version:
	args.clear();
	args['c'] = std::to_string( g_ConsoleID );
	RAWeb::CreateThreadedHTTPRequest( RequestLatestClientPage, args );	//	g_sGetLatestClientPage
	
	//	TBD:
	args.clear();
	args['u'] = RAUsers::LocalUser.Username();
	args['t'] = RAUsers::LocalUser.Token();
	RAWeb::CreateThreadedHTTPRequest( RequestScore, args );
	
	return TRUE;
}

API int CCONV _RA_Shutdown()
{
	_RA_SavePreferences();
	
	SAFE_DELETE( CoreAchievements );
	SAFE_DELETE( UnofficialAchievements );
	SAFE_DELETE( LocalAchievements );

	RAWeb::RA_KillHTTPThreads();

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
	
	CoUninitialize();

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

API int CCONV _RA_HardcoreModeIsActive()
{
	return g_hardcoreModeActive;
}

API int CCONV _RA_HTTPGetRequestExists( const char* sPageName )
{
	//return RAWeb::HTTPRequestExists( sPageName );	//	Deprecated
	return 0;
}

API int CCONV _RA_OnLoadNewRom( BYTE* pROM, size_t nROMSize, BYTE* pRAM, size_t nRAMSize, BYTE* pRAMExtra, size_t nRAMExtraSize )
{
	g_sCurrentROMMD5.clear();
	if( pROM != NULL && nROMSize > 0 )
		g_sCurrentROMMD5 = RA::GenerateMD5( pROM, nROMSize );

	g_MemManager.InstallRAM( pRAM, nRAMSize, pRAMExtra, nRAMExtraSize );

	//	Go ahead and load: RA_ConfirmLoadNewRom has allowed it.

	//	TBD: local DB of MD5 to GameIDs here
	GameID nGameID = 0;
	if( g_sCurrentROMMD5.length() > 0 )
	{
		//	Fetch the gameID from the DB here:
		PostArgs args;
		args['u'] = RAUsers::LocalUser.Username();
		args['m'] = g_sCurrentROMMD5;

		Document doc;
		if( RAWeb::DoBlockingRequest( RequestGameID, args, doc ) )
		{
			nGameID = static_cast<GameID>( doc["GameID"].GetUint() );
			if( nGameID == 0 )	//	Unknown
			{
				RA_LOG( "Could not recognise game with MD5 %s\n", g_sCurrentROMMD5.c_str() );
				char sEstimatedGameTitle[64];
				ZeroMemory( sEstimatedGameTitle, 64 );
				RA_GetEstimatedGameTitle( sEstimatedGameTitle );
				nGameID = Dlg_GameTitle::DoModalDialog( g_hThisDLLInst, g_RAMainWnd, g_sCurrentROMMD5, sEstimatedGameTitle );
			}
			else
			{
				RA_LOG( "Successfully looked up game with ID %d\n", nGameID );
			}
		}
		else
		{
			//	Some other fatal error... panic?
			assert( !"Unknown error from requestgameid.php" );

			char buffer[8192];
			sprintf_s( buffer, 8192, "Error from " RA_HOST "!\n" );
			MessageBox( g_RAMainWnd, buffer, "Error returned!", MB_OK );
		}
	}

	//g_PopupWindows.Clear(); //TBD

	g_bRAMTamperedWith = false;
	g_LeaderboardManager.Clear();
	g_PopupWindows.LeaderboardPopups().Reset();

	if( nGameID != 0 )
	{ 
		if( RAUsers::LocalUser.IsLoggedIn() )
		{
			//	Delete Core and Unofficial Achievements so it is redownloaded every time:
			AchievementSet::DeletePatchFile( AchievementSetCore, nGameID );
			AchievementSet::DeletePatchFile( AchievementSetUnofficial, nGameID );

			AchievementSet::FetchFromWebBlocking( nGameID );	//##BLOCKING##
			AchievementSet::LoadFromFile( nGameID );
			//CoreAchievements->Load( nGameID );
			//UnofficialAchievements->Load( nGameID );
			//LocalAchievements->Load( nGameID );

			RAUsers::LocalUser.PostActivity( ActivityType_StartPlaying );
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


//void FetchBinaryFromWeb( const char* sFilename )
//{
//	const unsigned int nBufferSize = (3*1024*1024);	//	3mb enough?
//
//	char* buffer = new char[nBufferSize];	
//	if( buffer != NULL )
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
//		buffer = NULL;
//	}
//}

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
	WaitForSingleObject( RAWeb::g_hHTTPMutex, INFINITE );

	RequestObject* pObj = RAWeb::LastHttpResults.PopNextItem();
	while( pObj	!= NULL )
	{
		if( pObj->GetResponse().size() > 0 )
		{
			Document doc;
			BOOL bJSONParsedOK = pObj->ParseResponseToJSON( doc );

			switch( pObj->GetRequestType() )
			{
			case RequestLogin:
				RAUsers::LocalUser.HandleSilentLoginResponse( doc );
				break;

			case RequestBadge:
				{
					SetCurrentDirectory( g_sHomeDir.c_str() );

					const std::string& sBadgeURI = pObj->GetData();
					_WriteBufferToFile( RA_DIR_BADGE + sBadgeURI + ".png", pObj->GetResponse() );

					for( size_t i = 0; i < g_pActiveAchievements->NumAchievements(); ++i )
					{
						Achievement& ach = g_pActiveAchievements->GetAchievement( i );
						if( ach.BadgeImageURI().compare( 0, 5, sBadgeURI, 0, 5 ) == 0 )
						{
							//	Re-set this badge image
							//	NB. This is already a non-modifying op
							ach.SetBadgeImage( sBadgeURI );
						}
					}

					g_AchievementEditorDialog.UpdateSelectedBadgeImage();	//	Is this necessary if it's no longer selected?
				}
				break;

			case RequestBadgeIter:
				g_AchievementEditorDialog.GetBadgeNames().CB_OnNewBadgeNames( pObj );
				break;

			case RequestUserPic:
				RAUsers::OnUserPicDownloaded( *pObj );
				break;

			case RequestScore:
				{
					ASSERT( doc["Success"].GetBool() );

					const std::string& sUser = doc["User"].GetString();
					unsigned int nScore = doc["Score"].GetUint();
					RA_LOG( "%s's score: %d", sUser.c_str(), nScore );

					if( sUser.compare( RAUsers::LocalUser.Username() ) == 0 )
					{
						RAUsers::LocalUser.SetScore( nScore );
					}
					else
					{
						//	Find friend? Update this information?
						RAUsers::GetUser( sUser )->SetScore( nScore );
					}
				}
				break;

			case RequestLatestClientPage:
				{
					if( doc.HasMember( "LatestVersion" ) )
					{
						const std::string& sReply = doc["LatestVersion"].GetString();
						if( sReply.substr( 0, 2 ).compare( "0." ) == 0 )
						{
							long nValServer = std::strtol( sReply.c_str()+2, NULL, 10 );
							long nValKnown = std::strtol( g_sKnownRAVersion.c_str()+2, NULL, 10 );
							long nValCurrent = std::strtol( g_sClientVersion+2, NULL, 10 );

							if( nValKnown < nValServer && nValCurrent < nValServer )
							{
								//	Update available:
								_RA_OfferNewRAUpdate( sReply.c_str() );

								//	Update the last version I've heard of:
								g_sKnownRAVersion = sReply;
							}
							else
							{
								RA_LOG( "Latest Client already up to date: server 0.%d, current 0.%d\n", nValServer, nValCurrent );
							}
						}
					}
				}
				break;

			case RequestSubmitAwardAchievement:
				{
					//	Response to an achievement being awarded:
					const Achievement* pAch = CoreAchievements->Find( static_cast<AchievementID>( doc["AchievementID"].GetUint() ) );
					if( pAch == NULL )
						pAch = UnofficialAchievements->Find( static_cast<AchievementID>( doc["AchievementID"].GetUint() ) );
					if( pAch != NULL )
					{
						if( !doc.HasMember("Error") )
						{
							g_PopupWindows.AchievementPopups().AddMessage( 
								MessagePopup( " Achievement Unlocked ",
											  " " + pAch->Title() + " (" + std::to_string( pAch->Points() ) + ") ",
											  PopupMessageType::PopupAchievementUnlocked,
											  pAch->BadgeImage() ) );
							g_AchievementsDialog.OnGet_Achievement( *pAch );
					
							RAUsers::LocalUser.SetScore( doc["Score"].GetUint() );
						}
						else
						{
							g_PopupWindows.AchievementPopups().AddMessage( 
								MessagePopup( " Achievement Unlocked (Error) ", 
											  " " + pAch->Title() + " (" + std::to_string( pAch->Points() ) + ") ",
											  PopupMessageType::PopupAchievementError, 
											  pAch->BadgeImage() ) );
							g_AchievementsDialog.OnGet_Achievement( *pAch );

							g_PopupWindows.AchievementPopups().AddMessage( 
								MessagePopup( 
									"Error submitting achievement:", 
									doc["Error"].GetString() ) ); //?

							//MessageBox( HWnd, buffer, "Error!", MB_OK|MB_ICONWARNING );
						}
					}
				}
				break;

			case RequestNews:
				SetCurrentDirectory( g_sHomeDir.c_str() );
				_WriteBufferToFile( RA_NEWS_FILENAME, doc );
				g_AchievementOverlay.InstallNewsArticlesFromFile();
				break;

			case RequestAchievementInfo:
				if( pObj->ParseResponseToJSON( doc ) )
					g_AchExamine.OnReceiveData( doc );
				break;

			case RequestCodeNotes:
				CodeNotes::OnCodeNotesResponse( doc );
				break;

			case RequestSubmitLeaderboardEntry:
				RA_LeaderboardManager::OnSubmitEntry( doc );
				break;
				
			case RequestLeaderboardInfo:
				if( pObj->ParseResponseToJSON( doc ) )
					g_LBExamine.OnReceiveData( doc );
				break;
			}
		
			//if( strcmp( pObj->m_sRequestPageName, "requestlogin.php" ) == 0 )
			//{
			//	if( pObj->m_bResponse == TRUE )
			//	{
			//		if( strncmp( pObj->m_sResponse, "FAILED", 6 ) == 0 )
			//		{
			//			char buffer[4096];
			//			sprintf_s( buffer, 4096, 
			//				"%s failed:\n\n"
			//				"Response from server:\n"
			//				"%s",
			//				( pObj->m_nUserRef == 1 ) ? "Auto-login" : "Login",
			//				pObj->m_sResponse );

			//			MessageBox( NULL, buffer, "Error in login", MB_OK );
			//			RAUsers::LocalUser.Clear();
			//		}
			//		else if( strncmp( pObj->m_sResponse, "OK:", 3 ) == 0 )
			//		{
			//			//	will return a valid token : points : messages

			//			//	fish out the username again
			//			char* pUserPtr = pObj->m_sRequestPost+2;
			//			char* sUser = strtok_s( pUserPtr, "&", &pUserPtr );

			//			//	find the token, points, and num messages
			//			char* pBuf = pObj->m_sResponse+3;
			//			char* pToken = strtok_s( pBuf, ":", &pBuf );
			//			unsigned int nPoints = strtol( pBuf, &pBuf, 10 );
			//			pBuf++;
			//			unsigned int nMessages = strtol( pBuf, &pBuf, 10 );
			//			//pBuf++;
			//					
			//			RAUsers::ProcessSuccessfulLogin( sUser, pToken, RAUsers::LocalUser.m_bStoreToken, nPoints, nMessages );
			//		}
			//		else
			//		{
			//			MessageBox( NULL, "Login failed: please check user/password!", "Error", MB_OK );
			//			RAUsers::LocalUser.Clear();
			//		}
			//	}
			//	else
			//	{
			//		MessageBox( NULL, "Login failed: please check user/password!", "Error", MB_OK );
			//		RAUsers::LocalUser.Clear();
			//	}

			//	RA_RebuildMenu();
			//	_RA_UpdateAppTitle();
			//}

		}

		SAFE_DELETE( pObj );
		pObj = RAWeb::LastHttpResults.PopNextItem();
	}

	ReleaseMutex( RAWeb::g_hHTTPMutex );

	return 0;
}

//	Following this function, an app should call AppendMenu to associate this submenu.
API HMENU CCONV _RA_CreatePopupMenu()
{
	HMENU hRA = CreatePopupMenu();
	if( RAUsers::LocalUser.IsLoggedIn() )
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
		AppendMenu( hRA, MF_STRING, IDM_RA_SCANFORGAMES, TEXT("Scan &for games") );
		AppendMenu( hRA, MF_STRING, IDM_RA_TOGGLELEADERBOARDS, TEXT("Toggle &Leaderboards") );
	}
	else
	{
		AppendMenu( hRA, MF_STRING, IDM_RA_FILES_LOGIN, TEXT("&Login to RA") );
	}

	AppendMenu( hRA, MF_SEPARATOR, NULL, NULL );
	AppendMenu( hRA, MF_STRING, IDM_RA_FILES_CHECKFORUPDATE, TEXT("&Check for Emulator Update") );
	
	return hRA;
}

API int CCONV _RA_UpdateAppTitle( const char* sMessage )
{
	std::stringstream sstr;
	sstr << std::string( g_sClientName ) << " - " << std::string( g_sClientVersion );

	if( sMessage != NULL )
		sstr << " - " << sMessage;

	if( RAUsers::LocalUser.IsLoggedIn() )
		sstr << " - " << RAUsers::LocalUser.Username();

	SetWindowText( g_RAMainWnd, sstr.str().c_str() );

	return 0;
}

API int CCONV _RA_CheckForUpdate()
{
	//	Blocking:
	char sReply[8192];
	ZeroMemory( sReply, 8192 );
	char* sReplyCh = &sReply[0];
	unsigned long nCharsRead = 0;

	PostArgs args;
	args['c'] = std::to_string( g_ConsoleID );

	DataStream Response;
	if( RAWeb::DoBlockingRequest( RequestLatestClientPage, args, Response ) )
	{
		std::string sReply = DataStreamAsString( Response );
		if( sReply.length() > 2 && sReply[0] == '0' && sReply[1] == '.' )
		{
			//	Ignore g_sKnownRAVersion: check against g_sRAVersion
			long nCurrent = strtol( g_sClientVersion+2, NULL, 10 );
			long nUpdate = strtol( sReplyCh+2, NULL, 10 );

			if( nCurrent < nUpdate )
			{
				_RA_OfferNewRAUpdate( sReply.c_str() );
			}
			else
			{
				//	Up to date
				char buffer[1024];
				sprintf_s( buffer, 1024, "You have the latest version of %s: %s", g_sClientEXEName, sReplyCh );
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

API void CCONV _RA_LoadPreferences()
{
	RA_LOG( __FUNCTION__ " - loading preferences...\n" );
	
	SetCurrentDirectory( g_sHomeDir.c_str() );
	FILE* pf = NULL;
	fopen_s( &pf, std::string( std::string( RA_PREFERENCES_FILENAME_PREFIX ) + g_sClientName + ".cfg" ).c_str(), "rb" );
	if( pf == NULL )
	{
		//	Test for first-time use:
		//RA_LOG( __FUNCTION__ " - no preferences found: showing first-time message!\n" );
		//
		//char sWelcomeMessage[4096];
		
		//sprintf_s( sWelcomeMessage, 4096, 
		//	"Welcome! It looks like this is your first time using RetroAchievements.\n\n"
		//	"Quick Start: Press ESCAPE or 'Back' on your Xbox 360 controller to view the achievement overlay.\n\n" );
			
		//switch( g_EmulatorID )
		//{
		//case RA_Gens:
		//	strcat_s( sWelcomeMessage, 4096,
		//		"Default Keyboard Controls: Use cursor keys, A-S-D are A, B, C, and Return for Start.\n\n" );
		//	break;
		//case RA_VisualboyAdvance:
		//	strcat_s( sWelcomeMessage, 4096,
		//		"Default Keyboard Controls: Use cursor keys, Z-X are A and B, A-S are L and R, use Return for Start and Backspace for Select.\n\n" );
		//	break;
		//case RA_Snes9x:
		//	strcat_s( sWelcomeMessage, 4096,
		//		"Default Keyboard Controls: Use cursor keys, D-C-S-X are A, B, X, Y, Z-V are L and R, use Return for Start and Space for Select.\n\n" );
		//	break;
		//case RA_FCEUX:
		//	strcat_s( sWelcomeMessage, 4096,
		//		"Default Keyboard Controls: Use cursor keys, D-F are B and A, use Return for Start and S for Select.\n\n" );
		//	break;
		//case RA_PCE:
		//	strcat_s( sWelcomeMessage, 4096,
		//		"Default Keyboard Controls: Use cursor keys, A-S-D for A, B, C, and Return for Start\n\n" );
		//	break;
		//}
			
		//strcat_s( sWelcomeMessage, 4096, "These defaults can be changed under [Option]->[Joypads].\n\n"
		//	"If you have any questions, comments or feedback, please visit forum.RetroAchievements.org for more information.\n\n" );
			
		//MessageBox( g_RAMainWnd, 
		//	sWelcomeMessage,
		//	"Welcome to RetroAchievements!", MB_OK );
			
		//	TBD: setup some decent default variables:
		_RA_SavePreferences();
	}
	else
	{
		Document doc;
		doc.ParseStream( FileStream( pf ) );
		
		if( doc.HasParseError() )
		{
			//MessageBox( NULL, std::to_string( doc.GetParseError() ).c_str(), "ERROR!", MB_OK );
			_RA_SavePreferences();
		}
		else
		{
			if( doc.HasMember( "Username" ) )
				RAUsers::LocalUser.SetUsername( doc["Username"].GetString() );
			if( doc.HasMember( "Token" ) )
				RAUsers::LocalUser.SetToken( doc["Token"].GetString() );
			if( doc.HasMember( "Hardcore Active" ) )
				g_hardcoreModeActive = doc["Hardcore Active"].GetBool();
			if( doc.HasMember( "Num Background Threads" ) )
				g_nNumHTTPThreads = doc["Num Background Threads"].GetUint();
			if( doc.HasMember( "ROM Directory" ) )
				g_sROMDirLocation = doc["ROM Directory"].GetString();
		}

		fclose( pf );
	}
	
	//TBD:
	//g_GameLibrary.LoadAll();
}

API void CCONV _RA_SavePreferences()
{
	RA_LOG( __FUNCTION__ " - saving preferences...\n" );
	
	SetCurrentDirectory( g_sHomeDir.c_str() );
	FILE* pf = NULL;
	fopen_s( &pf, std::string( std::string( RA_PREFERENCES_FILENAME_PREFIX ) + g_sClientName + ".cfg" ).c_str(), "wb" );
	if( pf != NULL )
	{
		FileStream fs( pf );
		Writer<FileStream> writer( fs );

		Document doc;
		doc.SetObject();

		Document::AllocatorType& a = doc.GetAllocator();
		doc.AddMember( "Username", StringRef( RAUsers::LocalUser.Username().c_str() ), a );
		doc.AddMember( "Token", StringRef( RAUsers::LocalUser.Token().c_str() ), a );
		doc.AddMember( "Hardcore Active", g_hardcoreModeActive, a );
		doc.AddMember( "Num Background Threads", g_nNumHTTPThreads, a );
		doc.AddMember( "ROM Directory", StringRef( g_sROMDirLocation.c_str() ), a );
		
		doc.Accept( writer );	//	Save

		fclose( pf );
	}

	//TBD:
	//g_GameLibrary.SaveAll();
}

void _FetchGameHashLibraryFromWeb()
{
	PostArgs args;
	args['c'] = std::to_string( g_ConsoleID );
	args['u'] = RAUsers::LocalUser.Username();
	DataStream Response; 
	if( RAWeb::DoBlockingRequest( RequestHashLibrary, args, Response ) )
		_WriteBufferToFile( RA_GAME_HASH_FILENAME, Response );
}

void _FetchGameTitlesFromWeb()
{
	PostArgs args;
	args['c'] = std::to_string( g_ConsoleID );
	args['u'] = RAUsers::LocalUser.Username();
	DataStream Response; 
	if( RAWeb::DoBlockingRequest( RequestGamesList, args, Response ) )
		_WriteBufferToFile( RA_GAME_LIST_FILENAME, Response );
}

void _FetchMyProgressFromWeb()
{
	PostArgs args;
	args['c'] = std::to_string( g_ConsoleID );
	args['u'] = RAUsers::LocalUser.Username();
	DataStream Response; 
	if( RAWeb::DoBlockingRequest( RequestAllProgress, args, Response ) )
		_WriteBufferToFile( RA_MY_PROGRESS_FILENAME, Response );
}

void EnsureDialogVisible( HWND hDlg )
{
	//	Does this nudge the dlg back onto the screen?

	const int nScreenWidth = GetSystemMetrics( SM_CXSCREEN );
	const int nScreenHeight = GetSystemMetrics( SM_CYMAXIMIZED ) - ( GetSystemMetrics( SM_CYSCREEN ) - GetSystemMetrics( SM_CYMAXIMIZED ) );

	RECT rc;
	GetWindowRect( g_AchievementsDialog.GetHWND(), &rc );
	
	const int nDlgWidth = rc.right - rc.left;
	const int nDlgHeight = rc.bottom - rc.top;

	if( rc.left < 0 || rc.top < 0 )
		SetWindowPos( hDlg, NULL, rc.left < 0 ? 0 : rc.left, rc.top < 0 ? 0 : rc.top, 0, 0, SWP_NOSIZE );
	else if( ( rc.right > nScreenWidth ) || ( rc.bottom > nScreenHeight ) )
		SetWindowPos( hDlg, NULL, rc.right > nScreenWidth ? nScreenWidth - nDlgWidth : rc.left, rc.bottom > nScreenHeight ? nScreenHeight - nDlgHeight : rc.top, 0, 0, SWP_NOSIZE );
}

API void CCONV _RA_InvokeDialog( LPARAM nID )
{
	switch( nID )
	{
		case IDM_RA_FILES_ACHIEVEMENTS:
			{
				if( g_AchievementsDialog.GetHWND() == NULL )
					g_AchievementsDialog.InstallHWND( CreateDialog( g_hThisDLLInst, MAKEINTRESOURCE(IDD_RA_ACHIEVEMENTS), g_RAMainWnd, g_AchievementsDialog.s_AchievementsProc ) );
				if( g_AchievementsDialog.GetHWND() != NULL )
					ShowWindow( g_AchievementsDialog.GetHWND(), SW_SHOW );
			
				EnsureDialogVisible( g_AchievementsDialog.GetHWND() );
			}
		break;

		case IDM_RA_FILES_ACHIEVEMENTEDITOR:
			{
				if( g_AchievementEditorDialog.GetHWND() == NULL )
					g_AchievementEditorDialog.InstallHWND( CreateDialog( g_hThisDLLInst, MAKEINTRESOURCE(IDD_RA_ACHIEVEMENTEDITOR), g_RAMainWnd, g_AchievementEditorDialog.s_AchievementEditorProc ) );
				if( g_AchievementEditorDialog.GetHWND() != NULL )
					ShowWindow( g_AchievementEditorDialog.GetHWND(), SW_SHOW );
			
				EnsureDialogVisible( g_AchievementsDialog.GetHWND() );
			}
		break;

		case IDM_RA_FILES_MEMORYFINDER:
			{
				if( g_MemoryDialog.GetHWND() == NULL )
					g_MemoryDialog.InstallHWND( CreateDialog( g_hThisDLLInst, MAKEINTRESOURCE(IDD_RA_MEMORY), g_RAMainWnd, g_MemoryDialog.s_MemoryProc ) );
				if( g_MemoryDialog.GetHWND() != NULL )
					ShowWindow( g_MemoryDialog.GetHWND(), SW_SHOW );
			
				EnsureDialogVisible( g_AchievementsDialog.GetHWND() );
			}
		break;
		
		case IDM_RA_FILES_LOGIN:
			{
				RA_Dlg_Login::DoModalLogin();
				_RA_SavePreferences();
				//RA_RebuildMenu();
			}

			break;

		case IDM_RA_FILES_LOGOUT:

			RAUsers::LocalUser.Clear();
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
			
			if( CoreAchievements->GetGameID() != 0 )
			{
				//	Delete Core and Unofficial Achievements so it is redownloaded every time:
				AchievementSet::DeletePatchFile( AchievementSetCore, CoreAchievements->GetGameID() );
				AchievementSet::DeletePatchFile( AchievementSetUnofficial, CoreAchievements->GetGameID() );
				if( !AchievementSet::LoadFromFile( CoreAchievements->GetGameID() ) )
				{
					//	Fetch then load again from file
					AchievementSet::FetchFromWebBlocking( CoreAchievements->GetGameID() );
					AchievementSet::LoadFromFile( CoreAchievements->GetGameID() );
				}
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
				if( g_pActiveAchievements->NumAchievements() == 0 )
				{
					MessageBox( NULL, "No ROM loaded!", "Error", MB_OK );
				}
				else
				{
					char buffer[2048];
					sprintf_s( buffer, "Current ROM MD5: %s", g_sCurrentROMMD5 );
				
					MessageBox( NULL, buffer, "Get ROM Checksum", MB_OK );
				}
			}
			break;

		case IDM_RA_OPENUSERPAGE:
			{
				if( RAUsers::LocalUser.IsLoggedIn() )
				{
					ShellExecute( NULL,
						"open",
						( RA_HOST_URL + std::string( "/User/" ) + RAUsers::LocalUser.Username() ).c_str(),
						NULL,
						NULL,
						SW_SHOWNORMAL );
				}
			}
			break;

		case IDM_RA_OPENGAMEPAGE:
			{
				if( g_pActiveAchievements->GetGameID() != 0 )
				{
					ShellExecute( NULL,
						"open",
						( RA_HOST_URL + std::string( "/Game/" ) + std::to_string( g_pActiveAchievements->GetGameID() ) ).c_str(),
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
						char buffer[4096];
						if( SHGetPathFromIDList( pidl, buffer ) )
						{
							g_sROMDirLocation = buffer;
							RA_LOG( "Selected Folder: %s\n", g_sROMDirLocation );
						}

						CoTaskMemFree( pidl );
					}
				}

				if( g_sROMDirLocation[0] != '\0' )
				{
					if( g_GameLibrary.GetHWND() == NULL )
					{
						_FetchGameHashLibraryFromWeb();
						_FetchGameTitlesFromWeb();
						_FetchMyProgressFromWeb();
							
						g_GameLibrary.InstallHWND( CreateDialog( g_hThisDLLInst, MAKEINTRESOURCE(IDD_RA_GAMELIBRARY), g_RAMainWnd, &Dlg_GameLibrary::s_GameLibraryProc ) );
					}

					if( g_GameLibrary.GetHWND() != NULL )
					{
						ShowWindow( g_GameLibrary.GetHWND(), SW_SHOW );
					}

					//Dlg_GameLibrary::DoModalDialog( g_hThisDLLInst, g_RAMainWnd );
				}

			}
			break;

		case IDM_RA_PARSERICHPRESENCE:
			{
				if( g_pActiveAchievements->GetGameID() != 0 )
				{
					char sRichPresenceFile[1024];
					sprintf_s( sRichPresenceFile, 1024, "%s%d-Rich.txt", RA_DIR_DATA, g_pActiveAchievements->GetGameID() );

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

		case IDM_RA_TOGGLELEADERBOARDS:
			if( g_bLeaderboardsActive )
			{
				g_bLeaderboardsActive = false;
				MessageBox( NULL, "Leaderboards are now disabled", "Warning", MB_OK );
			}
			else
			{
				g_bLeaderboardsActive = true;
				MessageBox( NULL, "Leaderboards are now enabled", "Warning", MB_OK );
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
	return RAUsers::LocalUser.Username().c_str();
}

API void CCONV _RA_AttemptLogin()
{
	RAUsers::LocalUser.AttemptLogin();
}

API void CCONV _RA_OnSaveState( const char* sFilename )
{
	//	Save State is being allowed by app (user was warned!)
	if( RAUsers::LocalUser.IsLoggedIn() )
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
	//	Save State is being allowed by app (user was warned!)
	if( RAUsers::LocalUser.IsLoggedIn() )
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
	if( RAUsers::LocalUser.IsLoggedIn() )
	{
		g_pActiveAchievements->Test();
		g_LeaderboardManager.Test();
		g_MemoryDialog.Invalidate();
	}
}

void CCONV _RA_InstallSharedFunctions( bool(*fpIsActive)(void), void(*fpCauseUnpause)(void), void(*fpRebuildMenu)(void), void(*fpEstimateTitle)(char*), void(*fpResetEmulation)(void), void(*fpLoadROM)(char*) )
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

int _WriteBufferToFile( const std::string& sFileName, const Document& doc )
{
	SetCurrentDirectory( g_sHomeDir.c_str() );
	FILE* pf = NULL;
	if( fopen_s( &pf, sFileName.c_str(), "wb" ) == 0 )
	{
		FileStream fs( pf );
		Writer<FileStream> writer( fs );
		doc.Accept( writer );

		//fwrite( raw.data(), 1, raw.size(), pf );
		fclose( pf );
	}

	return 0;
}

int _WriteBufferToFile( const std::string& sFileName, const DataStream& raw )
{
	SetCurrentDirectory( g_sHomeDir.c_str() );
	FILE* pf;
	if( fopen_s( &pf, sFileName.c_str(), "wb" ) == 0 )
	{
		fwrite( raw.data(), 1, raw.size(), pf );
		fclose( pf );
	}

	return 0;
}

int _WriteBufferToFile( const std::string& sFileName, const std::string& sData )
{
	SetCurrentDirectory( g_sHomeDir.c_str() );
	FILE* pf;
	if( fopen_s( &pf, sFileName.c_str(), "wb" ) == 0 )
	{
		fwrite( sData.data(), 1, sData.length(), pf );
		fclose( pf );
	}

	return 0;
}

int _WriteBufferToFile( const char* sFile, const BYTE* sBuffer, int nBytes )
{
	SetCurrentDirectory( g_sHomeDir.c_str() );
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
	SetCurrentDirectory( g_sHomeDir.c_str() );
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

std::string _TimeStampToString( time_t nTime )
{
	char buffer[64];
	ctime_s( buffer, 64, &nTime );
	return std::string( buffer );
}

BOOL _FileExists( const std::string& sFileName )
{
	FILE* pf = NULL;
	fopen_s( &pf, sFileName.c_str(), "rb" );
	if( pf != NULL )
	{
		fclose( pf );
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}