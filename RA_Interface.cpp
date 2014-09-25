#include "RA_Interface.h"

//#include <Windows.h>
//#include <WinHttp.h>
//#include <stdio.h>
//#include <assert.h>


#ifndef CCONV
#define CCONV __cdecl
#endif


//	Exposed, shared
//	App-level:
bool	(CCONV *_RA_GameIsActive) (void) = NULL;
void	(CCONV *_RA_CauseUnpause) (void) = NULL;
void	(CCONV *_RA_RebuildMenu) (void) = NULL;
void	(CCONV *_RA_ResetEmulation) (void) = NULL;
void	(CCONV *_RA_GetEstimatedGameTitle) (char* sNameOut) = NULL;
void	(CCONV *_RA_LoadROM) (char* sNameOut) = NULL;


bool RA_GameIsActive()
{
	if( _RA_GameIsActive != NULL )
		return _RA_GameIsActive();
	return false;
}

void RA_CauseUnpause()
{
	if( _RA_CauseUnpause != NULL )
		_RA_CauseUnpause();
}

void RA_RebuildMenu()
{
	if( _RA_RebuildMenu != NULL )
		_RA_RebuildMenu();
}

void RA_ResetEmulation()
{
	if( _RA_ResetEmulation != NULL )
		_RA_ResetEmulation();
}

void RA_LoadROM( char* sFullPath )
{
	if( _RA_LoadROM != NULL )
		_RA_LoadROM( sFullPath );
}

void RA_GetEstimatedGameTitle( char* sNameOut )
{
	if( _RA_GetEstimatedGameTitle != NULL )
		_RA_GetEstimatedGameTitle( sNameOut );
}


#ifndef RA_EXPORTS

//Note: this is ALL public facing! :S tbd tidy up this bit

//	Expose to app:
 
//	Generic:
const char* (CCONV *_RA_IntegrationVersion) () = NULL;
int		(CCONV *_RA_InitI) ( HWND hMainWnd, int nConsoleID, const char* sClientVer ) = NULL;
int		(CCONV *_RA_Shutdown) () = NULL;
//	Load/Save
int		(CCONV *_RA_ConfirmLoadNewRom) (int bQuitting) = NULL;
int		(CCONV *_RA_OnLoadNewRom) (const BYTE* pROM, const unsigned int nROMSize, const BYTE* pRAM, const unsigned int nRAMSize, const BYTE* pExtraRAM, const unsigned int nRAMExtraSize) = NULL;
void	(CCONV *_RA_OnLoadState)(const char* sFilename) = NULL;
void	(CCONV *_RA_OnSaveState)(const char* sFilename) = NULL;
//	Achievements:
void	(CCONV *_RA_DoAchievementsFrame)() = NULL;
//	User:
bool	(CCONV *_RA_UserLoggedIn)() = NULL;
const char*	(CCONV *_RA_Username)() = NULL;
void	(CCONV *_RA_AttemptLogin)() = NULL;
//	Graphics:
void	(CCONV *_RA_InitDirectX) (void) = NULL;
void	(CCONV *_RA_OnPaint)(HWND hWnd) = NULL;
//	Tools:
void	(CCONV *_RA_SetPaused)(bool bIsPaused) = NULL;
HMENU	(CCONV *_RA_CreatePopupMenu)() = NULL;
void	(CCONV *_RA_UpdateAppTitle) (const char* pMessage) = NULL;
void	(CCONV *_RA_HandleHTTPResults) (void) = NULL;
void	(CCONV *_RA_InvokeDialog)(LPARAM nID) = NULL;
void	(CCONV *_RA_InstallSharedFunctions)( bool(*)(), void(*)(), void(*)(), void(*)(char*), void(*)(), void(*)(char*) ) = NULL;
int		(CCONV *_RA_SetConsoleID)(unsigned int nConsoleID) = NULL;
int		(CCONV *_RA_HardcoreModeIsActive)(void) = NULL;
int		(CCONV *_RA_HTTPGetRequestExists)(const char* sPageName) = NULL;

//	Don't expose to app
HINSTANCE g_hRADLL = NULL;


int		(CCONV *_RA_UpdateOverlay) (ControllerInput* pInput, float fDeltaTime, bool Full_Screen, bool Paused) = NULL;
int		(CCONV *_RA_UpdatePopups) (ControllerInput* pInput, float fDeltaTime, bool Full_Screen, bool Paused) = NULL;
void	(CCONV *_RA_RenderOverlay) (HDC hDC, RECT* prcSize) = NULL;
void	(CCONV *_RA_RenderPopups) (HDC hDC, RECT* prcSize) = NULL;


//	Helpers:
bool RA_UserLoggedIn()
{
	if( _RA_UserLoggedIn != NULL )
		return _RA_UserLoggedIn();

	return false;
}

const char* RA_Username()
{
	return _RA_Username ? _RA_Username() : "";
}

void RA_AttemptLogin()
{
	if( _RA_AttemptLogin != NULL )
		_RA_AttemptLogin();
}

void RA_UpdateRenderOverlay( HDC hDC, ControllerInput* pInput, float fDeltaTime, RECT* prcSize, bool Full_Screen, bool Paused )
{
	if( _RA_UpdatePopups != NULL )
		_RA_UpdatePopups( pInput, fDeltaTime, Full_Screen, Paused );

	if( _RA_RenderPopups != NULL )
		_RA_RenderPopups( hDC, prcSize );

	//	NB. Render overlay second, on top of popups!
	if( _RA_UpdateOverlay != NULL )
		_RA_UpdateOverlay( pInput, fDeltaTime, Full_Screen, Paused );

	if( _RA_RenderOverlay != NULL )
		_RA_RenderOverlay( hDC, prcSize );
}

void RA_OnLoadNewRom( BYTE* pROMData, unsigned int nROMSize, BYTE* pRAMData, unsigned int nRAMSize, BYTE* pRAMExtraData, unsigned int nRAMExtraSize )
{
	if( _RA_OnLoadNewRom != NULL )
		_RA_OnLoadNewRom(pROMData, nROMSize, pRAMData, nRAMSize, pRAMExtraData, nRAMExtraSize);
}

HMENU RA_CreatePopupMenu()
{
	return ( _RA_CreatePopupMenu != NULL ) ? _RA_CreatePopupMenu() : NULL;
}

void RA_UpdateAppTitle( const char* sCustomMsg )
{
	if( _RA_UpdateAppTitle != NULL )
		_RA_UpdateAppTitle(sCustomMsg);
}

void RA_HandleHTTPResults()
{
	if( _RA_HandleHTTPResults != NULL )
		_RA_HandleHTTPResults();
}

bool RA_ConfirmLoadNewRom( bool bIsQuitting )
{
	return _RA_ConfirmLoadNewRom ? ( _RA_ConfirmLoadNewRom(bIsQuitting) )!=0 : true;
}

void RA_InitDirectX()
{
	if( _RA_InitDirectX != NULL )
		_RA_InitDirectX();
}

void RA_OnPaint( HWND hWnd )
{
	if( _RA_OnPaint != NULL )
		_RA_OnPaint( hWnd );
}

void RA_InvokeDialog( LPARAM nID )
{
	if( _RA_InvokeDialog != NULL )
		_RA_InvokeDialog( nID );
}

void RA_SetPaused( bool bIsPaused )
{
	if( _RA_SetPaused != NULL )
		_RA_SetPaused( bIsPaused );
}

void RA_OnLoadState( const char* sFilename )
{
	if( _RA_OnLoadState != NULL )
		_RA_OnLoadState( sFilename );
}

void RA_OnSaveState( const char* sFilename )
{
	if( _RA_OnSaveState != NULL )
		_RA_OnSaveState( sFilename );
}

void RA_DoAchievementsFrame()
{
	if( _RA_DoAchievementsFrame != NULL )
		_RA_DoAchievementsFrame();
}

int RA_SetConsoleID( unsigned int nConsoleID )
{
	if( _RA_SetConsoleID != NULL )
		return _RA_SetConsoleID( nConsoleID );
	return 0;
}

int RA_HardcoreModeIsActive()
{
	return ( _RA_HardcoreModeIsActive != NULL ) ? _RA_HardcoreModeIsActive() : 0;
}

int RA_HTTPRequestExists( const char* sPageName )
{
	return ( _RA_HTTPGetRequestExists != NULL ) ? _RA_HTTPGetRequestExists( sPageName ) : 0;
}


BOOL DoBlockingHttpGet( const char* sRequestedPage, char* pBufferOut, const unsigned int /*nBufferOutSize*/, DWORD* pBytesRead )
{
	BOOL bResults = FALSE, bSuccess = FALSE;
	HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;

	WCHAR wBuffer[1024];
	size_t nTemp;
	char* sDataDestOffset = &pBufferOut[0];
	DWORD nBytesToRead = 0;
	DWORD nBytesFetched = 0;

	char sClientName[1024];
	sprintf_s( sClientName, 1024, "Retro Achievements Client" );
	WCHAR wClientNameBuffer[1024];
	mbstowcs_s( &nTemp, wClientNameBuffer, 1024, sClientName, strlen(sClientName)+1 );

	// Use WinHttpOpen to obtain a session handle.
	hSession = WinHttpOpen( wClientNameBuffer, 
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME, 
		WINHTTP_NO_PROXY_BYPASS, 0);

	// Specify an HTTP server.
	if( hSession != NULL )
	{
		hConnect = WinHttpConnect( hSession, L"www.retroachievements.org", INTERNET_DEFAULT_HTTP_PORT, 0);

		// Create an HTTP Request handle.
		if( hConnect != NULL )
		{
			mbstowcs_s( &nTemp, wBuffer, 1024, sRequestedPage, strlen(sRequestedPage)+1 );

			hRequest = WinHttpOpenRequest( hConnect, 
				L"GET", 
				wBuffer, 
				NULL, 
				WINHTTP_NO_REFERER, 
				WINHTTP_DEFAULT_ACCEPT_TYPES,
				0);

			// Send a Request.
			if( hRequest != NULL )
			{
				bResults = WinHttpSendRequest( hRequest, 
					L"Content-Type: application/x-www-form-urlencoded",
					0, 
					WINHTTP_NO_REQUEST_DATA,
					0, 
					0,
					0);

				if( WinHttpReceiveResponse( hRequest, NULL ) )
				{
					nBytesToRead = 0;
					(*pBytesRead) = 0;
					WinHttpQueryDataAvailable( hRequest, &nBytesToRead );

					while( nBytesToRead > 0 )
					{
						char sHttpReadData[8192];
						ZeroMemory( sHttpReadData, 8192 );

						assert( nBytesToRead <= 8192 );
						if( nBytesToRead <= 8192 )
						{
							nBytesFetched = 0;
							if( WinHttpReadData( hRequest, &sHttpReadData, nBytesToRead, &nBytesFetched ) )
							{
								assert( nBytesToRead == nBytesFetched );

								//Read: parse buffer
								memcpy( sDataDestOffset, sHttpReadData, nBytesFetched );

								sDataDestOffset += nBytesFetched;
								(*pBytesRead) += nBytesFetched;
							}
						}

						bSuccess = TRUE;

						WinHttpQueryDataAvailable( hRequest, &nBytesToRead );
					}
				}
			}
		}
	}


	// Close open handles.
	if (hRequest) WinHttpCloseHandle(hRequest);
	if (hConnect) WinHttpCloseHandle(hConnect);
	if (hSession) WinHttpCloseHandle(hSession);

	return bSuccess;
}

void WriteBufferToFile( const char* sFile, const char* sBuffer, int nBytes )
{
	TCHAR buffer[256];
	GetCurrentDirectory( 256, buffer );

	//MessageBox( NULL, "Writing new RA_Integration to file!", buffer, MB_OK );

	FILE* pf;
	fopen_s( &pf, sFile, "wb" );
	if( pf )
	{
		fwrite( sBuffer, 1, nBytes, pf );
		fclose( pf );
		
		//MessageBox( NULL, "File written OK!", sFile, MB_OK );
	}
	else
	{
		MessageBoxA( NULL, "Problems writing file!", sFile, MB_OK );
	}
}

void FetchIntegrationFromWeb()
{
	char* buffer = new char[1*1024*1024];
	if( buffer != NULL )
	{
		DWORD nBytesRead = 0;

		if( DoBlockingHttpGet( "RA_Integration.dll", buffer, 1*1024*1024, &nBytesRead ) )
			WriteBufferToFile( "RA_Integration.dll", buffer, nBytesRead );
 
		delete[] ( buffer );
		buffer = NULL;
	}
}

const char* CCONV _RA_InstallIntegration()
{
	
#ifndef NDEBUG
	g_hRADLL = LoadLibraryA( "RA_Integration_d.dll" );
#else
	g_hRADLL = LoadLibraryA( "RA_Integration.dll" );
#endif
	if( g_hRADLL == NULL )
		return  "0.000";

	//	Install function pointers one by one
 
	_RA_IntegrationVersion	= (const char*(CCONV *)(void))					GetProcAddress( g_hRADLL, "_RA_IntegrationVersion" );
	_RA_InitI				= (int(CCONV *)(HWND, int, const char*))		GetProcAddress( g_hRADLL, "_RA_InitI" );;
	_RA_Shutdown			= (int(CCONV *)(void))							GetProcAddress( g_hRADLL, "_RA_Shutdown" );
	_RA_UserLoggedIn		= (bool(CCONV *)(void))							GetProcAddress( g_hRADLL, "_RA_UserLoggedIn" );
	_RA_Username			= (const char*(CCONV *)(void))					GetProcAddress( g_hRADLL, "_RA_Username" );
	_RA_AttemptLogin		= (void(CCONV *)())								GetProcAddress( g_hRADLL, "_RA_AttemptLogin" );
	_RA_UpdateOverlay		= (int(CCONV *)(ControllerInput*, float, bool, bool))	GetProcAddress( g_hRADLL, "_RA_UpdateOverlay" );
	_RA_UpdatePopups		= (int(CCONV *)(ControllerInput*, float, bool, bool))	GetProcAddress( g_hRADLL, "_RA_UpdatePopups" );
	_RA_RenderOverlay		= (void(CCONV *)(HDC, RECT*))					GetProcAddress( g_hRADLL, "_RA_RenderOverlay" );
	_RA_RenderPopups		= (void(CCONV *)(HDC, RECT*))					GetProcAddress( g_hRADLL, "_RA_RenderPopups" );
	_RA_OnLoadNewRom		= (int(CCONV *)(const BYTE*, const unsigned int, const BYTE*, const unsigned int, const BYTE*, const unsigned int))	GetProcAddress( g_hRADLL, "_RA_OnLoadNewRom" );
	_RA_UpdateAppTitle		= (void(CCONV *)(const char*))					GetProcAddress( g_hRADLL, "_RA_UpdateAppTitle" );
	
	_RA_HandleHTTPResults	= (void(CCONV *)())								GetProcAddress( g_hRADLL, "_RA_HandleHTTPResults" );
	_RA_ConfirmLoadNewRom	= (int(CCONV *)(int))							GetProcAddress( g_hRADLL, "_RA_ConfirmLoadNewRom" );
	_RA_CreatePopupMenu		= (HMENU(CCONV *)(void))						GetProcAddress( g_hRADLL, "_RA_CreatePopupMenu" );
	_RA_InitDirectX			= (void(CCONV *)(void))							GetProcAddress( g_hRADLL, "_RA_InitDirectX" );
	_RA_OnPaint				= (void(CCONV *)(HWND))							GetProcAddress( g_hRADLL, "_RA_OnPaint" );
	_RA_InvokeDialog		= (void(CCONV *)(LPARAM))						GetProcAddress( g_hRADLL, "_RA_InvokeDialog" );
	_RA_SetPaused			= (void(CCONV *)(bool))							GetProcAddress( g_hRADLL, "_RA_SetPaused" );
	_RA_OnLoadState			= (void(CCONV *)(const char*))					GetProcAddress( g_hRADLL, "_RA_OnLoadState" );
	_RA_OnSaveState			= (void(CCONV *)(const char*))					GetProcAddress( g_hRADLL, "_RA_OnSaveState" );
	_RA_DoAchievementsFrame = (void(CCONV *)())								GetProcAddress( g_hRADLL, "_RA_DoAchievementsFrame" );
	_RA_SetConsoleID		= (int(CCONV *)(unsigned int))					GetProcAddress( g_hRADLL, "_RA_SetConsoleID" );
	_RA_HardcoreModeIsActive= (int(CCONV *)())								GetProcAddress( g_hRADLL, "_RA_HardcoreModeIsActive" );
	_RA_HTTPGetRequestExists= (int(CCONV *)(const char*))					GetProcAddress( g_hRADLL, "_RA_HTTPGetRequestExists" );

	_RA_InstallSharedFunctions = ( void(CCONV *)( bool(*)(), void(*)(), void(*)(), void(*)(char*), void(*)(), void(*)(char*) ) ) GetProcAddress( g_hRADLL, "_RA_InstallSharedFunctions" );

	return _RA_IntegrationVersion ? _RA_IntegrationVersion() : "0.000";
}

//	Console IDs: see enum EmulatorID in header
void RA_Init( HWND hMainHWND, int nConsoleID, const char* sClientVersion )
{
	DWORD nBytesRead = 0;
	char buffer[1024];
	ZeroMemory( buffer, 1024 );
	if( DoBlockingHttpGet( "LatestIntegration.html", buffer, 1024, &nBytesRead ) == FALSE )
	{
		MessageBoxA( NULL, "Cannot access www.retroachievements.org - working offline.", "Warning", MB_OK|MB_ICONEXCLAMATION );
		return;
	}

	const unsigned int nLatestDLLVer = strtol( buffer+2, NULL, 10 );

	BOOL bInstalled = FALSE;
	int nMBReply = IDNO;
	do
	{
		const char* sVerInstalled = _RA_InstallIntegration();
		const unsigned int nVerInstalled = strtol( sVerInstalled+2, NULL, 10 );
		if( nVerInstalled < nLatestDLLVer )
		{
			RA_Shutdown();	//	Unhook the DLL, it's out of date. We may need to overwrite the DLL, so unhook it.%

			char sErrorMsg[2048];
			sprintf_s( sErrorMsg, 2048, "%s\nLatest Version: 0.%03d\n%s",
				nVerInstalled == 0 ? 
					"Cannot find or load RA_Integration.dll" :
					"A new version of the RetroAchievements Toolset is available!",
				nLatestDLLVer,
				"Automatically update your RetroAchievements Toolset file?" );

			nMBReply = MessageBoxA( NULL, sErrorMsg, "Warning", MB_YESNO );

			if( nMBReply == IDYES )
			{
				FetchIntegrationFromWeb();
			}
		}
		else
		{
			bInstalled = TRUE;
			break;
		}

	} while( nMBReply == IDYES );

	if( bInstalled )
		_RA_InitI( hMainHWND, nConsoleID, sClientVersion );
	else
		RA_Shutdown();

}

void RA_InstallSharedFunctions( bool(*fpIsActive)(void), void(*fpCauseUnpause)(void), void(*fpRebuildMenu)(void), void(*fpEstimateTitle)(char*), void(*fpResetEmulation)(void), void(*fpLoadROM)(char*) )
{
	_RA_GameIsActive			= fpIsActive;
	_RA_CauseUnpause			= fpCauseUnpause;
	_RA_RebuildMenu				= fpRebuildMenu;
	_RA_GetEstimatedGameTitle	= fpEstimateTitle;
	_RA_ResetEmulation			= fpResetEmulation;
	_RA_LoadROM					= fpLoadROM;

	//	Also install *within* DLL! FFS
	if( _RA_InstallSharedFunctions != NULL )
		_RA_InstallSharedFunctions( fpIsActive, fpCauseUnpause, fpRebuildMenu, fpEstimateTitle, fpResetEmulation, fpLoadROM );
}

void RA_Shutdown()
{
	//	Call shutdown on toolchain
	if( _RA_Shutdown != NULL )
		_RA_Shutdown();

	//	Clear func ptrs
	_RA_IntegrationVersion	= NULL;
	_RA_InitI				= NULL;
	_RA_Shutdown			= NULL;
	_RA_UserLoggedIn		= NULL;
	_RA_Username			= NULL;
	_RA_UpdateOverlay		= NULL;
	_RA_UpdatePopups		= NULL;
	_RA_RenderOverlay		= NULL;
	_RA_RenderPopups		= NULL;
	_RA_OnLoadNewRom		= NULL;
	_RA_UpdateAppTitle		= NULL;
	_RA_HandleHTTPResults	= NULL;
	_RA_ConfirmLoadNewRom	= NULL;
	_RA_CreatePopupMenu		= NULL;
	_RA_InitDirectX			= NULL;
	_RA_OnPaint				= NULL;
	_RA_InvokeDialog		= NULL;
	_RA_SetPaused			= NULL;
	_RA_OnLoadState			= NULL;
	_RA_OnSaveState			= NULL;
	_RA_DoAchievementsFrame = NULL;
	_RA_InstallSharedFunctions= NULL;

	_RA_GameIsActive		= NULL;
	_RA_CauseUnpause		= NULL;
	_RA_RebuildMenu			= NULL;
	_RA_GetEstimatedGameTitle= NULL;
	_RA_ResetEmulation		= NULL;
	_RA_LoadROM				= NULL;
	_RA_SetConsoleID		= NULL;
	_RA_HardcoreModeIsActive= NULL;
	_RA_HTTPGetRequestExists= NULL;
	_RA_AttemptLogin		= NULL;

	//	Uninstall DLL
	FreeLibrary( g_hRADLL );
}


#endif //RA_EXPORTS