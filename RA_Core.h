#pragma once

#include "RA_Defs.h"
#include "RA_Interface.h"
//#include <stdio.h>

#if defined RA_EXPORTS
#define API __declspec(dllexport)
#else
#define API
#endif

#ifdef __cplusplus
extern "C" {
#endif

//	Fetch the version number of this integration version.
API const char* CCONV _RA_IntegrationVersion();

//	Initialize all data related to RA Engine. Call as early as possible.
API int CCONV _RA_InitI( HWND hMainHWND, /*enum EmulatorID*/int nConsoleID, const char* sClientVersion );

//	Call for a tidy exit at end of app.
API int CCONV _RA_Shutdown();

//	Allocates and configures a popup menu, to be called, embedded and managed by the app.
API HMENU CCONV _RA_CreatePopupMenu();

//	Check all achievement sets for changes, and displays a dlg box to warn lost changes.
API bool CCONV _RA_ConfirmLoadNewRom( bool bQuittingApp );

//	On or immediately after a new ROM is loaded, including if the ROM is reset.
API int CCONV _RA_OnLoadNewRom( const BYTE* pROM, unsigned int nROMSize );

//	On or immediately after a new ROM is loaded, for each memory bank found
//	NB:
//pReader is typedef unsigned char (_RAMByteReadFn)( size_t nOffset );
//pWriter is typedef void (_RAMByteWriteFn)( unsigned int nOffs, unsigned int nVal );
API void CCONV _RA_InstallMemoryBank( int nBankID, void* pReader, void* pWriter, int nBankSize );

//	Call before installing any memory banks
API void CCONV _RA_ClearMemoryBanks();

//	Immediately after loading a new state.
API void CCONV _RA_OnLoadState( const char* sFileName );

//	Immediately after saving a new state.
API void CCONV _RA_OnSaveState( const char* sFileName );


//	Perform one test for all achievements in the current set. Call this once per frame/cycle.
API void CCONV _RA_DoAchievementsFrame();


//	Use in special cases where the emulator contains more than one console ID.
API void CCONV _RA_SetConsoleID( unsigned int nConsoleID );

//	Display a dialog helping the user get the latest RA client version
API BOOL CCONV _RA_OfferNewRAUpdate( const char* sNewVer );

//	Deal with any HTTP results that come along. Call per-cycle from main thread.
API int CCONV _RA_HandleHTTPResults();

//	Execute a blocking check to see if this client is out of date.
API void CCONV _RA_CheckForUpdate();

//	Update the title of the app
API void CCONV _RA_UpdateAppTitle( const char* sMessage = nullptr );

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
API void CCONV _RA_AttemptLogin( bool bBlocking );

//	Return whether or not the hardcore mode is active.
API int CCONV _RA_HardcoreModeIsActive();

//	Return whether a HTTPGetRequest already exists
API int CCONV _RA_HTTPGetRequestExists( const char* sPageName );

//	Install user-side functions that can be called from the DLL
API void CCONV _RA_InstallSharedFunctions(bool(*fpIsActive)(void), void(*fpCauseUnpause)(void), void(*fpRebuildMenu)(void), void(*fpEstimateTitle)(char*), void(*fpResetEmulation)(void), void(*fpLoadROM)(const char*));
API void CCONV _RA_InstallSharedFunctionsExt( bool(*fpIsActive)(void), void(*fpCauseUnpause)(void), void(*fpCausePause)(void), void(*fpRebuildMenu)(void), void(*fpEstimateTitle)(char*), void(*fpResetEmulation)(void), void(*fpLoadROM)(const char*) );

#ifdef __cplusplus
}
#endif


//	Non-exposed:
extern std::string g_sKnownRAVersion;
extern std::string g_sHomeDir;
extern std::string g_sROMDirLocation;
extern std::string g_sCurrentROMMD5;

extern HINSTANCE g_hRAKeysDLL;
extern HMODULE g_hThisDLLInst;
extern HWND g_RAMainWnd;
extern EmulatorID g_EmulatorID;
extern ConsoleID g_ConsoleID;
extern const char* g_sGetLatestClientPage;
extern const char* g_sClientVersion;
extern const char* g_sClientName;
extern bool g_bRAMTamperedWith;
extern bool g_bHardcoreModeActive;
extern bool g_bLeaderboardsActive;
extern unsigned int g_nNumHTTPThreads;

//	Read a file to a malloc'd buffer. Returns NULL on error. Owner MUST free() buffer if not NULL.
extern char* _MallocAndBulkReadFileToBuffer( const char* sFilename, long& nFileSizeOut );

//	Read file until reaching the end of the file, or the specified char.
extern BOOL _ReadTil( const char nChar, char buffer[], unsigned int nSize, DWORD* pCharsRead, FILE* pFile );

//	Read a string til the end of the string, or nChar. bTerminate==TRUE replaces that char with \0.
extern char* _ReadStringTil( char nChar, char*& pOffsetInOut, BOOL bTerminate );

//	Write out the buffer to a file
extern void _WriteBufferToFile( const std::string& sFileName, const DataStream& rawData );
extern void _WriteBufferToFile( const std::string& sFileName, const Document& doc );
extern void _WriteBufferToFile( const std::string& sFileName, const std::string& sString );
extern void _WriteBufferToFile( const char* sFile, const BYTE* sBuffer, int nBytes );

//	Fetch various interim txt/data files
extern void _FetchGameHashLibraryFromWeb();
extern void _FetchGameTitlesFromWeb();
extern void _FetchMyProgressFromWeb();

extern BOOL _FileExists( const std::string& sFileName );

extern std::string _TimeStampToString( time_t nTime );

extern std::string GetFolderFromDialog();

extern BOOL RemoveFileIfExists(const std::string& sFilePath);