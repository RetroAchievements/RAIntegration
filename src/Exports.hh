#ifndef RA_EXPORTS_H
#define RA_EXPORTS_H
#pragma once

#ifndef CCONV
#define CCONV __cdecl
#endif

#define API __declspec(dllexport)

#ifdef __cplusplus
extern "C" {
#endif

    // Fetch the version number of this integration version.
    API const char* CCONV _RA_IntegrationVersion();

    // Fetch the name of the host to connect to.
    API const char* CCONV _RA_HostName();

    // Fetch the URL of the host to connect to.
    API const char* CCONV _RA_HostUrl();

    // Initialize all data related to RA Engine. Call as early as possible.
    API int CCONV _RA_InitI(HWND hMainHWND, /*enum EmulatorID*/int nEmulatorID, const char* sClientVersion);

    // Initialize all data related to RA Engine for offline mode. Call as early as possible.
    API int CCONV _RA_InitOffline(HWND hMainHWND, /*enum EmulatorID*/int nEmulatorID, const char* sClientVersion);

    // Initialize all data related to RA Engine. Call as early as possible.
    API int CCONV _RA_InitClient(HWND hMainHWND, const char* sClientName, const char* sClientVersion);

    // Initialize all data related to RA Engine for offline mode. Call as early as possible.
    API int CCONV _RA_InitClientOffline(HWND hMainHWND, const char* sClientName, const char* sClientVersion);

    // Specifies additional information to include in the UserAgent string
    API void CCONV _RA_SetUserAgentDetail(const char* sDetail);

    // Changes the HWND for the main emulator window
    API void CCONV _RA_UpdateHWnd(HWND hMainHWND);

    // Call for a tidy exit at end of app.
    API int CCONV _RA_Shutdown();

    // Allocates and configures a popup menu, to be called, embedded and managed by the app.
    API HMENU CCONV _RA_CreatePopupMenu();

    // Gets items for building a popup menu to be embedded and managed by the app.
    struct RA_MenuItem;
    API int CCONV _RA_GetPopupMenuItems(RA_MenuItem* pItems);

    // Check all achievement sets for changes, and displays a dlg box to warn lost changes.
    API int CCONV _RA_ConfirmLoadNewRom(int bQuittingApp);

    //  Gets the unique identifier of the game associated to the provided ROM data
    API unsigned int CCONV _RA_IdentifyRom(const BYTE* pROMData, unsigned int nROMSize);

    //  Gets the unique identifier of the game associated to the provided file (which may already be loaded into memory)
    API unsigned int CCONV _RA_IdentifyHash(const char* sHash);

    //  Downloads and activates the achievements for the specified game.
    API void CCONV _RA_ActivateGame(unsigned int nGameId);

    //	Downloads and activates the achievements for the game associated to the provided ROM data
    API int CCONV _RA_OnLoadNewRom(const BYTE* pROM, unsigned int nROMSize);

    // On or immediately after a new ROM is loaded, for each memory bank found
    //  pReader is typedef unsigned char (_RAMByteReadFn)( size_t nOffset );
    //  pWriter is typedef void (_RAMByteWriteFn)( unsigned int nOffs, unsigned int nVal );
    API void CCONV _RA_InstallMemoryBank(int nBankID, void* pReader, void* pWriter, int nBankSize);

    // Call before installing any memory banks
    API void CCONV _RA_ClearMemoryBanks();

    // Immediately after loading a new state.
    API void CCONV _RA_OnLoadState(const char* sFileName);

    // Immediately after saving a new state.
    API void CCONV _RA_OnSaveState(const char* sFileName);

    // Captures the current RetroAchievements state data.
    //  returns the number of bytes written to pBuffer. if larger than nBufferSize, the
    //  caller should allocate a larger buffer and call again.
    API int CCONV _RA_CaptureState(char* pBuffer, int nBufferSize);

    // Restores the RetroAchievements state from captured state data.
    API void CCONV _RA_RestoreState(const char* pBuffer);

    // Immediately after resetting the system.
    API void CCONV _RA_OnReset();

    // Perform one test for all achievements in the current set. Call this once per frame/cycle.
    API void CCONV _RA_DoAchievementsFrame();

    // Enables forced repainting for controls when InvalidateWindow doesn't cause them to update
    // properly. Seems to primarily be an issue when integrating with an application using SDL.
    API void CCONV _RA_SetForceRepaint(int bEnable);

    // Temporarily disable forced redraws
    API void CCONV _RA_SuspendRepaint();

    // Resume forced redraws
    API void CCONV _RA_ResumeRepaint();

    // Use in special cases where the emulator contains more than one console ID.
    API void CCONV _RA_SetConsoleID(unsigned int nConsoleID);

    // Update the title of the app
    API void CCONV _RA_UpdateAppTitle(const char* sMessage = nullptr);

    // Display or unhide an RA dialog.
    API void CCONV _RA_InvokeDialog(LPARAM nID);

    // Call this when the pause state changes, to update RA with the new state.
    API void CCONV _RA_SetPaused(int bIsPaused);

    // Attempt to login, or present login dialog.
    API void CCONV _RA_AttemptLogin(int bBlocking);

    // Gets the user name of the currently logged in user - empty string is no user is currently logged in.
    API const char* CCONV _RA_UserName();

    // Return whether or not the hardcore mode is active.
    API int CCONV _RA_HardcoreModeIsActive();

    // Should be called before performing an activity that is not allowed in hardcore mode to
    // give the user a chance to disable hardcore mode and continue with the activity.
    // Returns TRUE if hardcore was disabled, or FALSE to cancel the activity.
    API int CCONV _RA_WarnDisableHardcore(const char* sActivity);

    // Install user-side functions that can be called from the DLL
    API void CCONV _RA_InstallSharedFunctions(bool(*)(void), void(*fpCauseUnpause)(void), void(*fpRebuildMenu)(void), void(*fpEstimateTitle)(char*), void(*fpResetEmulation)(void), void(*fpLoadROM)(const char*));
    API void CCONV _RA_InstallSharedFunctionsExt(bool(*)(void), void(*fpCauseUnpause)(void), void(*fpCausePause)(void), void(*fpRebuildMenu)(void), void(*fpEstimateTitle)(char*), void(*fpResetEmulation)(void), void(*fpLoadROM)(const char*));

    struct ControllerInput;
    API void CCONV _RA_NavigateOverlay(_In_ const ControllerInput* pInput);
    API int CCONV _RA_IsOverlayFullyVisible();

#ifdef __cplusplus
}
#endif

#endif // !RA_EXPORTS_H
