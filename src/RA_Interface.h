#ifndef RA_INTERFACE_H
#define RA_INTERFACE_H
#pragma once

//	NB. Shared between RA_Integration and emulator
using BOOL = int;

#ifndef CCONV
#define CCONV __cdecl
#endif

struct ControllerInput
{
    BOOL m_bUpPressed;
    BOOL m_bDownPressed;
    BOOL m_bLeftPressed;
    BOOL m_bRightPressed;
    BOOL m_bConfirmPressed; // Usually C or A
    BOOL m_bCancelPressed;  // Usually B
    BOOL m_bQuitPressed;    // Usually Start
};

enum EmulatorID
{
    RA_Gens = 0,
    RA_Project64 = 1,
    RA_Snes9x = 2,
    RA_VisualboyAdvance = 3,
    RA_Nester = 4,
    RA_FCEUX = 5,
    RA_PCE = 6,
    RA_Libretro = 7,
    RA_Meka = 8,
    RA_QUASI88 = 9,
    RA_AppleWin = 10,

    NumEmulatorIDs,
    UnknownEmulator = NumEmulatorIDs
};

//	Should match DB!
enum ConsoleID
{
    UnknownConsoleID = 0,
    MegaDrive = 1, //	DB
    N64 = 2,
    SNES = 3,
    GB = 4,
    GBA = 5,
    GBC = 6,
    NES = 7,
    PCEngine = 8,
    SegaCD = 9,
    Sega32X = 10,
    MasterSystem = 11,
    PlayStation = 12,
    Lynx = 13,
    NeoGeoPocket = 14,
    GameGear = 15,
    GameCube = 16,
    Jaguar = 17,
    DS = 18,
    WII = 19,
    WIIU = 20,
    PlayStation2 = 21,
    Xbox = 22,
    Events = 23, // not an actual console
    XboxOne = 24,
    Atari2600 = 25,
    MSDOS = 26,
    Arcade = 27,
    VirtualBoy = 28,
    MSX = 29,
    C64 = 30,
    ZX81 = 31,
    // unused32 = 32,
    SG1000 = 33,
    VIC20 = 34,
    Amiga = 35,
    AmigaST = 36,
    AmstradCPC = 37,
    AppleII = 38,
    Saturn = 39,
    Dreamcast = 40,
    PSP = 41,
    CDi = 42,
    ThreeDO = 43,
    Colecovision = 44,
    Intellivision = 45,
    Vectrex = 46,
    PC8800 = 47,
    PC9800 = 48,
    PCFX = 49,
    Atari5200 = 50,
    Atari7800 = 51,
    X68K = 52,
    WonderSwan = 53,

    NumConsoleIDs
};

extern bool (*_RA_GameIsActive)();
extern void (*_RA_CauseUnpause)();
extern void (*_RA_CausePause)();
extern void (*_RA_RebuildMenu)();
extern void (*_RA_GetEstimatedGameTitle)(char* sNameOut);
extern void (*_RA_ResetEmulation)();
extern void (*_RA_LoadROM)(const char* sFullPath);

// Shared funcs, should be implemented by emulator.
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
extern bool RA_GameIsActive();
extern void RA_CauseUnpause();
extern void RA_CausePause();
extern void RA_RebuildMenu();
extern void RA_GetEstimatedGameTitle(char* sNameOut);
extern void RA_ResetEmulation();
extern void RA_LoadROM(const char* sFullPath);
#ifdef __cplusplus
}
#endif // __cplusplus

#ifndef RA_EXPORTS

#include <wtypes.h>

//
//	Note: any changes in these files will require a full binary release of the emulator!
//	 This file will be fully included in the emulator build as standard.
//

// resource values for menu items - needed by MFC ON_COMMAND_RANGE macros
// they're not all currently used, allowing additional items without forcing recompilation of the emulators
#define IDM_RA_MENUSTART 1700
#define IDM_RA_MENUEND 1739

//	Captures the RA_DLL and installs/allocs all required information.
//	Populates all function pointers so they can be used by the app.
extern void RA_Init(HWND hMainHWND, /*enum ConsoleType*/ int console, const char* sClientVersion);

//	Call with shared function pointers from app.
extern void RA_InstallSharedFunctions(bool (*fpIsActive)(void), void (*fpCauseUnpause)(void),
                                      void (*fpCausePause)(void), void (*fpRebuildMenu)(void),
                                      void (*fpEstimateTitle)(char*), void (*fpResetEmulator)(void),
                                      void (*fpLoadROM)(const char*));

//	Shuts down, tidies up and deallocs the RA DLL from the app's perspective.
extern void RA_Shutdown();

//	Perform one test for all achievements in the current set. Call this once per frame/cycle.
extern void RA_DoAchievementsFrame();

//	Updates and renders all on-screen overlays.
extern void RA_UpdateRenderOverlay(HDC hDC, ControllerInput* pInput, float fDeltaTime, RECT* prcSize, bool Full_Screen,
                                   bool Paused);

//  Determines if the overlay is completely covering the screen.
extern bool RA_IsOverlayFullyVisible();

//	Attempts to login, or show login dialog.
extern void RA_AttemptLogin(bool bBlocking);

//  Gets the unique identifier of the game associated to the provided ROM data
extern unsigned int RA_IdentifyRom(BYTE* pROMData, unsigned int nROMSize);

//  Downloads and activates the achievements for the specified game.
extern void RA_ActivateGame(unsigned int nGameId);

//	Downloads and activates the achievements for the game associated to the provided ROM data
extern void RA_OnLoadNewRom(BYTE* pROMData, unsigned int nROMSize);

//	Clear all memory banks before installing any new ones!
extern void RA_ClearMemoryBanks();

//	Call once for each memory bank found, immediately after a new rom or load
// pReader is typedef unsigned char (_RAMByteReadFn)( size_t nOffset );
// pWriter is typedef void (_RAMByteWriteFn)( unsigned int nOffs, unsigned int nVal );
extern void RA_InstallMemoryBank(int nBankID, void* pReader, void* pWriter, int nBankSize);

//	Call this before loading a new ROM or quitting, to ensure no developer changes are lost.
extern bool RA_ConfirmLoadNewRom(bool bIsQuitting);

//	Should be called every time the app's title should change.
extern void RA_UpdateAppTitle(const char* sCustomMsg);

//	Call when you wish to recreate the popup menu.
extern HMENU RA_CreatePopupMenu();

//	Should be called as regularly as possible from the emulator's main thread.
extern void RA_HandleHTTPResults();

//	Should be called to update RA whenever the emulator's 'paused' state changes.
extern void RA_SetPaused(bool bIsPaused);

//	With multiple platform emulators, call this immediately before loading a new ROM.
extern void RA_SetConsoleID(unsigned int nConsoleID);

//  Should be called before performing an activity that is not allowed in hardcore mode to
//  give the user a chance to disable hardcore mode and continue with the activity.
//  Returns TRUE if hardcore was disabled, or FALSE to cancel the activity.
extern bool RA_WarnDisableHardcore(const char* sActivity);

//	Should be called immediately after loading or saving a new state.
extern void RA_OnLoadState(const char* sFilename);
extern void RA_OnSaveState(const char* sFilename);

//  Should be called immediately after resetting the system.
extern void RA_OnReset();

//	Call this on response to a menu selection for any RA ID:
extern void RA_InvokeDialog(LPARAM nID);

//	Returns TRUE if HC mode is ongoing
extern int RA_HardcoreModeIsActive();

#endif // RA_EXPORTS

#endif // !RA_INTERFACE_H
