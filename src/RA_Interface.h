#ifndef RA_INTERFACE_H
#define RA_INTERFACE_H
#pragma once

//	NB. Shared between RA_Integration and emulator

typedef int BOOL;

#ifndef CCONV
#define CCONV __cdecl
#endif

struct ControllerInput
{
    BOOL m_bUpPressed;
    BOOL m_bDownPressed;
    BOOL m_bLeftPressed;
    BOOL m_bRightPressed;
    BOOL m_bConfirmPressed;	//	Usually C or A
    BOOL m_bCancelPressed;	//	Usually B
    BOOL m_bQuitPressed;	//	Usually Start
};

enum EmulatorID
{
    RA_Gens = 0,
    RA_Project64,
    RA_Snes9x,
    RA_VisualboyAdvance,
    RA_Nester,
    RA_FCEUX,
    RA_PCE,
    RA_Libretro,
    RA_Meka,

    NumEmulatorIDs,
    UnknownEmulator = NumEmulatorIDs
};

//	Should match DB!
enum ConsoleID
{
    UnknownConsoleID = 0,
    MegaDrive = 1,	//	DB
    N64,
    SNES,
    GB,
    GBA,
    GBC,
    NES,
    PCEngine,
    SegaCD,
    Sega32X,
    MasterSystem,
    PlayStation,
    Lynx,
    NeoGeoPocket,
    GameGear,
    GameCube,
    Jaguar,
    DS,
    WII,
    WIIU,
    PlayStation2,
    Xbox,
    Skynet,
    XboxOne,
    Atari2600,
    MSDOS,
    Arcade,
    VirtualBoy,
    MSX,
    C64,
    ZX81,
    NeoGeo,
    SG1000,
    VIC20,
    Amiga,
    AmigaST,
    AmstradCPC,
    AppleII,
    Saturn,
    Dreamcast,
    PSP,
    CDi,
    ThreeDO,
    Colecovision,
    Intellivision,
    Vectrex,
    PC8800,
    PC9800,
    PCFX,
    Atari5200,
    Atari7800,
    X68K,
    WonderSwan,

    NumConsoleIDs
};


extern bool(*_RA_GameIsActive)();
extern void(*_RA_CauseUnpause)();
extern void(*_RA_CausePause)();
extern void(*_RA_RebuildMenu)();
extern void(*_RA_GetEstimatedGameTitle)(char* sNameOut);
extern void(*_RA_ResetEmulation)();
extern void(*_RA_LoadROM)(const char* sFullPath);

//	Shared funcs, should be implemented by emulator.
extern bool RA_GameIsActive();
extern void RA_CauseUnpause();
extern void RA_CausePause();
extern void RA_RebuildMenu();
extern void RA_GetEstimatedGameTitle(char* sNameOut);
extern void RA_ResetEmulation();
extern void RA_LoadROM(const char* sFullPath);

#ifndef RA_EXPORTS

#include <wtypes.h>

//
//	Note: any changes in these files will require a full binary release of the emulator!
//	 This file will be fully included in the emulator build as standard.
//

// resource values for menu items - needed by MFC ON_COMMAND_RANGE macros
// they're not all currently used, allowing additional items without forcing recompilation of the emulators
#define IDM_RA_MENUSTART                1700
#define IDM_RA_MENUEND                  1739

//	Captures the RA_DLL and installs/allocs all required information.
//	Populates all function pointers so they can be used by the app.
extern void RA_Init(HWND hMainHWND, /*enum ConsoleType*/int console, const char* sClientVersion);

//	Call with shared function pointers from app.
extern void RA_InstallSharedFunctions(bool(*fpIsActive)(void), void(*fpCauseUnpause)(void), void(*fpCausePause)(void), void(*fpRebuildMenu)(void), void(*fpEstimateTitle)(char*), void(*fpResetEmulator)(void), void(*fpLoadROM)(const char*));

//	Shuts down, tidies up and deallocs the RA DLL from the app's perspective.
extern void RA_Shutdown();


//	Perform one test for all achievements in the current set. Call this once per frame/cycle.
extern void RA_DoAchievementsFrame();


//	Updates and renders all on-screen overlays.
extern void RA_UpdateRenderOverlay(HDC hDC, ControllerInput* pInput, float fDeltaTime, RECT* prcSize, bool Full_Screen, bool Paused);

//  Determines if the overlay is completely covering the screen.
extern bool RA_IsOverlayFullyVisible();

//	Attempts to login, or show login dialog.
extern void RA_AttemptLogin(bool bBlocking);

//	Should be called immediately after a new ROM is loaded.
extern void RA_OnLoadNewRom(BYTE* pROMData, unsigned int nROMSize);

//	Clear all memory banks before installing any new ones!
extern void RA_ClearMemoryBanks();

//	Call once for each memory bank found, immediately after a new rom or load
//pReader is typedef unsigned char (_RAMByteReadFn)( size_t nOffset );
//pWriter is typedef void (_RAMByteWriteFn)( unsigned int nOffs, unsigned int nVal );
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

//	Should be called immediately after loading or saving a new state.
extern void RA_OnLoadState(const char* sFilename);
extern void RA_OnSaveState(const char* sFilename);

//  Should be called immediately after resetting the system.
extern void RA_OnReset();

//	Call this on response to a menu selection for any RA ID:
extern void RA_InvokeDialog(LPARAM nID);

//	Returns TRUE if HC mode is ongoing
extern int RA_HardcoreModeIsActive();

#endif //RA_EXPORTS


#endif // !RA_INTERFACE_H
