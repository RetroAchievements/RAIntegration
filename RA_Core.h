#ifndef __RA_CORE__
#define __RA_CORE__

#include "RA_Defs.h"
#include "RA_Interface.h"
#include <stdio.h>

#if defined RA_EXPORTS
#define API __declspec(dllexport)
#else
#define API
#endif

#ifndef CCONV
#define CCONV __cdecl
#endif


extern "C"
{

//	Fetch the version number of this integration version.
API const char* CCONV _RA_IntegrationVersion();

//	Initialize all data related to RA Engine. Call as early as possible.
API int CCONV _RA_InitI( HWND hMainHWND, /*enum EmulatorID*/int nConsoleID, const char* sClientVersion );

//	Call for a tidy exit at end of app.
API int CCONV _RA_Shutdown();

//	Allocates and configures a popup menu, to be called, embedded and managed by the app.
API HMENU CCONV _RA_CreatePopupMenu();

//	Check all achievement sets for changes, and displays a dlg box to warn lost changes.
API int CCONV _RA_ConfirmLoadNewRom( BOOL bQuittingApp );

//	On or immediately after a new ROM is loaded, including if the ROM is reset.
API int CCONV _RA_OnLoadNewRom( BYTE* pROM, unsigned int nROMSize, BYTE* pRAM, unsigned int nRAMSize, BYTE* pRAMExtra, unsigned int nRAMExtraSize );


//	Immediately after loading a new state.
API void CCONV _RA_OnLoadState( const char* sFileName );

//	Immediately after saving a new state.
API void CCONV _RA_OnSaveState( const char* sFileName );


//	Perform one test for all achievements in the current set. Call this once per frame/cycle.
API void CCONV _RA_DoAchievementsFrame();


//	Use in special cases where the emulator contains more than one console ID.
API int CCONV _RA_SetConsoleID( unsigned int nConsoleID );

//	Display a dialog helping the user get the latest RA client version
API BOOL CCONV _RA_OfferNewRAUpdate( const char* sNewVer );

//	Deal with any HTTP results that come along. Call per-cycle from main thread.
API int CCONV _RA_HandleHTTPResults();

//	Execute a blocking check to see if this client is out of date.
API int CCONV _RA_CheckForUpdate();

//	Update the title of the app
API int CCONV _RA_UpdateAppTitle( const char* sMessage = NULL );

//	Load preferences from ra_prefs.cfg
API void CCONV _RA_LoadPreferences();

//	Save preferences to ra_prefs.cfg
API void CCONV _RA_SavePreferences();

//	Display or unhide an RA dialog.
API void CCONV _RA_InvokeDialog( LPARAM nID );

//	Call this when the pause state changes, to update RA with the new state.
API void CCONV _RA_SetPaused( bool bIsPaused );

//	Returns the currently active user
API const char* CCONV _RA_Username();

//	Attempt to login, or present login dialog.
API void CCONV _RA_AttemptLogin();

//	Return whether or not the hardcore mode is active.
API int CCONV _RA_HardcoreModeIsActive();

//	Return whether a HTTPGetRequest already exists
API int CCONV _RA_HTTPGetRequestExists( const char* sPageName );

//	Install user-side functions that can be called from the DLL
API void CCONV _RA_InstallSharedFunctions( bool(*fpIsActive)(void), void(*fpCauseUnpause)(void), void(*fpRebuildMenu)(void), void(*fpEstimateTitle)(char*), void(*fpResetEmulation)(void), void(*fpLoadROM)(char*) );


}


//	Non-exposed:

extern char g_sKnownRAVersion[50];
extern char g_sHomeDir[2048];
extern char g_sROMDirLocation[2048];
extern HINSTANCE g_hRAKeysDLL;
extern HMODULE g_hThisDLLInst;
extern HWND g_RAMainWnd;
extern EmulatorID g_EmulatorID;
extern unsigned int g_ConsoleID;
extern const char* g_sGetLatestClientPage;
extern const char* g_sClientVersion;
extern const char* g_sClientName;
extern bool g_bRAMTamperedWith;
extern bool g_hardcoreModeActive;
extern unsigned int g_nNumHTTPThreads;

extern const char* (*g_fnKeysVersion)(void);
extern void (*g_fnDoValidation)(char sBufferOut[50], const char* sUser, const char* sToken, const unsigned int nID);

//	Install validation tools.
extern bool _RA_InstallKeys();

//	Read a file to a malloc'd buffer. Returns NULL on error. Owner MUST free() buffer if not NULL.
extern char* _MallocAndBulkReadFileToBuffer( const char* sFilename, long& nFileSizeOut );

//	Read file until reaching the end of the file, or the specified char.
extern BOOL _ReadTil( char nChar, char buffer[], unsigned int nSize, DWORD* pCharsRead, FILE* pFile );

//	Read a string til the end of the string, or nChar. bTerminate==TRUE replaces that char with \0.
extern char* _ReadStringTil( char nChar, char*& pOffsetInOut, BOOL bTerminate );

//	Write out the buffer to a file
extern int _WriteBufferToFile( const char* sFile, const char* sBuffer, int nBytes );

//	Fetch various interim txt/data files
extern void _FetchGameHashLibraryFromWeb();
extern void _FetchGameTitlesFromWeb();
extern void _FetchMyProgressFromWeb();

#endif //__RA_CORE__