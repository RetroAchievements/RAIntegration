#include "RA_Interface.h"

// Exposed, shared
// App-level:
bool (CCONV *_RA_GameIsActive) (void) = nullptr;
void (CCONV *_RA_CauseUnpause) (void) = nullptr;
void (CCONV *_RA_CausePause) (void) = nullptr;
void (CCONV *_RA_RebuildMenu) (void) = nullptr;
void (CCONV *_RA_ResetEmulation) (void) = nullptr;
void (CCONV *_RA_GetEstimatedGameTitle) (char* sNameOut) = nullptr;
void (CCONV *_RA_LoadROM) (const char* sNameOut) = nullptr;

bool RA_GameIsActive()
{
    if (_RA_GameIsActive != nullptr)
        return _RA_GameIsActive();
    return false;
}

void RA_CauseUnpause()
{
    if (_RA_CauseUnpause != nullptr)
        _RA_CauseUnpause();
}

void RA_CausePause()
{
    if (_RA_CausePause != nullptr)
        _RA_CausePause();
}

void RA_RebuildMenu()
{
    if (_RA_RebuildMenu != nullptr)
        _RA_RebuildMenu();
}

void RA_ResetEmulation()
{
    if (_RA_ResetEmulation != nullptr)
        _RA_ResetEmulation();
}

void RA_LoadROM(const char* sFullPath)
{
    if (_RA_LoadROM != nullptr)
        _RA_LoadROM(sFullPath);
}

void RA_GetEstimatedGameTitle(char* sNameOut)
{
    if (_RA_GetEstimatedGameTitle != nullptr)
        _RA_GetEstimatedGameTitle(sNameOut);
}


#ifndef RA_EXPORTS

#include <winhttp.h>
#include <cassert>
#include <string>

//Note: this is ALL public facing! :S tbd tidy up this bit

//	Expose to app:

//	Generic:
const char* (CCONV *_RA_IntegrationVersion)() = nullptr;
const char* (CCONV *_RA_HostName)() = nullptr;
int		(CCONV *_RA_InitI)(HWND hMainWnd, int nConsoleID, const char* sClientVer) = nullptr;
int		(CCONV *_RA_InitOffline)(HWND hMainWnd, int nConsoleID, const char* sClientVer) = nullptr;
int		(CCONV *_RA_Shutdown)() = nullptr;
//	Load/Save
bool    (CCONV *_RA_ConfirmLoadNewRom)(bool bQuitting) = nullptr;
int     (CCONV *_RA_OnLoadNewRom)(const BYTE* pROM, unsigned int nROMSize) = nullptr;
unsigned int (CCONV *_RA_IdentifyRom)(const BYTE* pROM, unsigned int nROMSize) = nullptr;
void    (CCONV *_RA_ActivateGame)(unsigned int nGameId) = nullptr;
void    (CCONV *_RA_InstallMemoryBank)(int nBankID, void* pReader, void* pWriter, int nBankSize) = nullptr;
void    (CCONV *_RA_ClearMemoryBanks)() = nullptr;
void    (CCONV *_RA_OnLoadState)(const char* sFilename) = nullptr;
void    (CCONV *_RA_OnSaveState)(const char* sFilename) = nullptr;
void    (CCONV *_RA_OnReset)() = nullptr;
//	Achievements:
void    (CCONV *_RA_DoAchievementsFrame)() = nullptr;
//	User:
void    (CCONV *_RA_AttemptLogin)(bool bBlocking) = nullptr;
//	Tools:
void    (CCONV *_RA_SetPaused)(bool bIsPaused) = nullptr;
HMENU   (CCONV *_RA_CreatePopupMenu)() = nullptr;
void    (CCONV *_RA_UpdateAppTitle)(const char* pMessage) = nullptr;
void    (CCONV *_RA_HandleHTTPResults)(void) = nullptr;
void    (CCONV *_RA_InvokeDialog)(LPARAM nID) = nullptr;
void    (CCONV *_RA_InstallSharedFunctions)(bool(*)(), void(*)(), void(*)(), void(*)(), void(*)(char*), void(*)(), void(*)(const char*)) = nullptr;
int     (CCONV *_RA_SetConsoleID)(unsigned int nConsoleID) = nullptr;
int     (CCONV *_RA_HardcoreModeIsActive)(void) = nullptr;
bool    (CCONV *_RA_WarnDisableHardcore)(const char* sActivity) = nullptr;
//  Overlay:
int     (CCONV *_RA_UpdateOverlay)(ControllerInput* pInput, float fDeltaTime, bool Full_Screen, bool Paused) = nullptr;
int     (CCONV *_RA_UpdatePopups)(ControllerInput* pInput, float fDeltaTime, bool Full_Screen, bool Paused) = nullptr;
void    (CCONV *_RA_RenderOverlay)(HDC hDC, RECT* prcSize) = nullptr;
void    (CCONV *_RA_RenderPopups)(HDC hDC, RECT* prcSize) = nullptr;
bool    (CCONV *_RA_IsOverlayFullyVisible) () = nullptr;


//	Don't expose to app
HINSTANCE g_hRADLL = nullptr;

void RA_AttemptLogin(bool bBlocking)
{
    if (_RA_AttemptLogin != nullptr)
        _RA_AttemptLogin(bBlocking);
}

void RA_UpdateRenderOverlay(HDC hDC, ControllerInput* pInput, float fDeltaTime, RECT* prcSize, bool Full_Screen, bool Paused)
{
    if (_RA_UpdatePopups != nullptr)
        _RA_UpdatePopups(pInput, fDeltaTime, Full_Screen, Paused);

    if (_RA_RenderPopups != nullptr)
        _RA_RenderPopups(hDC, prcSize);

    //	NB. Render overlay second, on top of popups!
    if (_RA_UpdateOverlay != nullptr)
        _RA_UpdateOverlay(pInput, fDeltaTime, Full_Screen, Paused);

    if (_RA_RenderOverlay != nullptr)
        _RA_RenderOverlay(hDC, prcSize);
}

bool RA_IsOverlayFullyVisible()
{
    if (_RA_IsOverlayFullyVisible != nullptr)
        return _RA_IsOverlayFullyVisible();

    return false;
}

unsigned int RA_IdentifyRom(BYTE* pROMData, unsigned int nROMSize)
{
    if (_RA_IdentifyRom != nullptr)
        return _RA_IdentifyRom(pROMData, nROMSize);

    return 0;
}

void RA_ActivateGame(unsigned int nGameId)
{
    if (_RA_ActivateGame != nullptr)
        _RA_ActivateGame(nGameId);
}

void RA_OnLoadNewRom(BYTE* pROMData, unsigned int nROMSize)
{
    if (_RA_OnLoadNewRom != nullptr)
        _RA_OnLoadNewRom(pROMData, nROMSize);
}

void RA_ClearMemoryBanks()
{
    if (_RA_ClearMemoryBanks != nullptr)
        _RA_ClearMemoryBanks();
}

void RA_InstallMemoryBank(int nBankID, void* pReader, void* pWriter, int nBankSize)
{
    if (_RA_InstallMemoryBank != nullptr)
        _RA_InstallMemoryBank(nBankID, pReader, pWriter, nBankSize);
}

HMENU RA_CreatePopupMenu()
{
    return (_RA_CreatePopupMenu != nullptr) ? _RA_CreatePopupMenu() : nullptr;
}

void RA_UpdateAppTitle(const char* sCustomMsg)
{
    if (_RA_UpdateAppTitle != nullptr)
        _RA_UpdateAppTitle(sCustomMsg);
}

void RA_HandleHTTPResults()
{
    if (_RA_HandleHTTPResults != nullptr)
        _RA_HandleHTTPResults();
}

bool RA_ConfirmLoadNewRom(bool bIsQuitting)
{
    return _RA_ConfirmLoadNewRom ? _RA_ConfirmLoadNewRom(bIsQuitting) : true;
}

void RA_InvokeDialog(LPARAM nID)
{
    if (_RA_InvokeDialog != nullptr)
        _RA_InvokeDialog(nID);
}

void RA_SetPaused(bool bIsPaused)
{
    if (_RA_SetPaused != nullptr)
        _RA_SetPaused(bIsPaused);
}

void RA_OnLoadState(const char* sFilename)
{
    if (_RA_OnLoadState != nullptr)
        _RA_OnLoadState(sFilename);
}

void RA_OnSaveState(const char* sFilename)
{
    if (_RA_OnSaveState != nullptr)
        _RA_OnSaveState(sFilename);
}

void RA_OnReset()
{
    if (_RA_OnReset != nullptr)
        _RA_OnReset();
}

void RA_DoAchievementsFrame()
{
    if (_RA_DoAchievementsFrame != nullptr)
        _RA_DoAchievementsFrame();
}

void RA_SetConsoleID(unsigned int nConsoleID)
{
    if (_RA_SetConsoleID != nullptr)
        _RA_SetConsoleID(nConsoleID);
}

int RA_HardcoreModeIsActive()
{
    return (_RA_HardcoreModeIsActive != nullptr) ? _RA_HardcoreModeIsActive() : 0;
}

bool RA_WarnDisableHardcore(const char* sActivity)
{
    // If Hardcore mode not active, allow the activity.
    if (!RA_HardcoreModeIsActive())
        return true;

    // DLL function will display a yes/no dialog. If the user chooses yes, the DLL will disable hardcore mode, and the activity can proceed.
    if (_RA_WarnDisableHardcore != nullptr)
        return _RA_WarnDisableHardcore(sActivity);

    // We cannot disable hardcore mode, so just warn the user and prevent the activity.
    std::string sMessage;
    sMessage = "You cannot " + std::string(sActivity) + " while Hardcore mode is active.";
    MessageBoxA(nullptr, sMessage.c_str(), "Warning", MB_OK | MB_ICONWARNING);
    return false;
}

static BOOL DoBlockingHttpGet(const char* sHostName, const char* sRequestedPage, char* pBufferOut, unsigned int nBufferOutSize, DWORD* pBytesRead, DWORD* pStatusCode)
{
    BOOL bResults = FALSE, bSuccess = FALSE;
    HINTERNET hSession = nullptr, hConnect = nullptr, hRequest = nullptr;

    WCHAR wBuffer[1024];
    size_t nTemp;
    DWORD nBytesToRead = 0;
    DWORD nBytesFetched = 0;
    (*pBytesRead) = 0;

    // Use WinHttpOpen to obtain a session handle.
    hSession = WinHttpOpen(L"RetroAchievements Client Bootstrap",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);

    // Specify an HTTP server.
    if (hSession == nullptr)
    {
        *pStatusCode = GetLastError();
    }
    else
    {
        mbstowcs_s(&nTemp, wBuffer, sizeof(wBuffer) / sizeof(wBuffer[0]), sHostName, strlen(sHostName) + 1);
        hConnect = WinHttpConnect(hSession, wBuffer, INTERNET_DEFAULT_HTTP_PORT, 0);

        // Create an HTTP Request handle.
        if (hConnect == nullptr)
        {
            *pStatusCode = GetLastError();
        }
        else
        {
            mbstowcs_s(&nTemp, wBuffer, sizeof(wBuffer)/sizeof(wBuffer[0]), sRequestedPage, strlen(sRequestedPage) + 1);

            hRequest = WinHttpOpenRequest(hConnect,
                L"GET",
                wBuffer,
                nullptr,
                WINHTTP_NO_REFERER,
                WINHTTP_DEFAULT_ACCEPT_TYPES,
                0);

            // Send a Request.
            if (hRequest == nullptr)
            {
                *pStatusCode = GetLastError();
            }
            else
            {
                bResults = WinHttpSendRequest(hRequest,
                    L"Content-Type: application/x-www-form-urlencoded",
                    0,
                    WINHTTP_NO_REQUEST_DATA,
                    0,
                    0,
                    0);

                if (!bResults || !WinHttpReceiveResponse(hRequest, nullptr))
                {
                    *pStatusCode = GetLastError();
                }
                else
                {
                    DWORD dwSize = sizeof(DWORD);
                    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, pStatusCode, &dwSize, WINHTTP_NO_HEADER_INDEX);

                    nBytesToRead = 0;
                    WinHttpQueryDataAvailable(hRequest, &nBytesToRead);

                    bSuccess = TRUE;
                    while (nBytesToRead > 0)
                    {
                        if (nBytesToRead > nBufferOutSize)
                        {
                            if (*pStatusCode == 200)
                                *pStatusCode = 998;

                            bSuccess = FALSE;
                            break;
                        }

                        nBytesFetched = 0;
                        if (WinHttpReadData(hRequest, pBufferOut, nBytesToRead, &nBytesFetched))
                        {
                            pBufferOut += nBytesFetched;
                            nBufferOutSize -= nBytesFetched;
                            (*pBytesRead) += nBytesFetched;
                        }
                        else
                        {
                            if (*pStatusCode == 200)
                                *pStatusCode = GetLastError();

                            bSuccess = FALSE;
                            break;
                        }

                        WinHttpQueryDataAvailable(hRequest, &nBytesToRead);
                    }
                }

                WinHttpCloseHandle(hRequest);
            }

            WinHttpCloseHandle(hConnect);
        }

        WinHttpCloseHandle(hSession);
    }

    return bSuccess;
}

static std::wstring GetIntegrationPath()
{
    wchar_t sBuffer[2048];
    DWORD iIndex = GetModuleFileNameW(0, sBuffer, 2048);
    while (iIndex > 0 && sBuffer[iIndex - 1] != '\\' && sBuffer[iIndex - 1] != '/')
        --iIndex;
    wcscpy_s(&sBuffer[iIndex], sizeof(sBuffer)/sizeof(sBuffer[0]) - iIndex, L"RA_Integration.dll");

    return std::wstring(sBuffer);
}

static void WriteBufferToFile(const std::wstring& sFile, const char* sBuffer, int nBytes)
{
    FILE* pf;
    errno_t nErr = _wfopen_s(&pf, sFile.c_str(), L"wb");
    if (pf != nullptr)
    {
        fwrite(sBuffer, 1, nBytes, pf);
        fclose(pf);
    }
    else
    {
        wchar_t sErrBuffer[2048];
        _wcserror_s(sErrBuffer, sizeof(sErrBuffer)/sizeof(sErrBuffer[0]), nErr);

        std::wstring sErrMsg = L"Unable to write " + sFile + L"\n" + sErrBuffer;
        MessageBoxW(nullptr, sErrMsg.c_str(), L"Error", MB_OK | MB_ICONERROR);
    }
}

static void FetchIntegrationFromWeb(const char* sHostName, DWORD* pStatusCode)
{
    const int MAX_SIZE = 2 * 1024 * 1024;
    char* buffer = new char[MAX_SIZE];
    if (buffer == nullptr)
    {
        *pStatusCode = 999;
    }
    else
    {
        DWORD nBytesRead = 0;
        if (DoBlockingHttpGet(sHostName, "bin/RA_Integration.dll", buffer, MAX_SIZE, &nBytesRead, pStatusCode))
            WriteBufferToFile(GetIntegrationPath(), buffer, nBytesRead);

        delete[](buffer);
        buffer = nullptr;
    }
}

//Returns the last Win32 error, in string format. Returns an empty string if there is no error.
static std::string GetLastErrorAsString()
{
    //Get the error message, if any.
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID == 0)
        return "No error message has been recorded";

    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, nullptr);

    std::string message(messageBuffer, size);

    //Free the buffer.
    LocalFree(messageBuffer);

    return message;
}

static const char* CCONV _RA_InstallIntegration()
{
    SetErrorMode(0);

    std::wstring sIntegrationPath = GetIntegrationPath();

    DWORD dwAttrib = GetFileAttributesW(sIntegrationPath.c_str());
    if (dwAttrib == INVALID_FILE_ATTRIBUTES)
        return "0.000";

    g_hRADLL = LoadLibraryW(sIntegrationPath.c_str());
    if (g_hRADLL == nullptr)
    {
        char buffer[1024];
        sprintf_s(buffer, 1024, "LoadLibrary failed: %d\n%s\n", ::GetLastError(), GetLastErrorAsString().c_str());
        MessageBoxA(nullptr, buffer, "Sorry!", MB_OK | MB_ICONWARNING);

        return "0.000";
    }

    //	Install function pointers one by one

    _RA_IntegrationVersion = (const char*(CCONV *)())                                 GetProcAddress(g_hRADLL, "_RA_IntegrationVersion");
    _RA_HostName = (const char*(CCONV *)())                                           GetProcAddress(g_hRADLL, "_RA_HostName");
    _RA_InitI = (int(CCONV *)(HWND, int, const char*))                                GetProcAddress(g_hRADLL, "_RA_InitI");
    _RA_InitOffline = (int(CCONV *)(HWND, int, const char*))                          GetProcAddress(g_hRADLL, "_RA_InitOffline");
    _RA_Shutdown = (int(CCONV *)())                                                   GetProcAddress(g_hRADLL, "_RA_Shutdown");
    _RA_AttemptLogin = (void(CCONV *)(bool))                                          GetProcAddress(g_hRADLL, "_RA_AttemptLogin");
    _RA_UpdateOverlay = (int(CCONV *)(ControllerInput*, float, bool, bool))           GetProcAddress(g_hRADLL, "_RA_UpdateOverlay");
    _RA_UpdatePopups = (int(CCONV *)(ControllerInput*, float, bool, bool))            GetProcAddress(g_hRADLL, "_RA_UpdatePopups");
    _RA_RenderOverlay = (void(CCONV *)(HDC, RECT*))                                   GetProcAddress(g_hRADLL, "_RA_RenderOverlay");
    _RA_IsOverlayFullyVisible = (bool(CCONV *)())                                     GetProcAddress(g_hRADLL, "_RA_IsOverlayFullyVisible");
    _RA_RenderPopups = (void(CCONV *)(HDC, RECT*))                                    GetProcAddress(g_hRADLL, "_RA_RenderPopups");
    _RA_OnLoadNewRom = (int(CCONV *)(const BYTE*, unsigned int))                      GetProcAddress(g_hRADLL, "_RA_OnLoadNewRom");
    _RA_IdentifyRom = (unsigned int(CCONV *)(const BYTE*, unsigned int))              GetProcAddress(g_hRADLL, "_RA_IdentifyRom");
    _RA_ActivateGame = (void(CCONV *)(unsigned int))                                  GetProcAddress(g_hRADLL, "_RA_ActivateGame");
    _RA_InstallMemoryBank = (void(CCONV *)(int, void*, void*, int))                   GetProcAddress(g_hRADLL, "_RA_InstallMemoryBank");
    _RA_ClearMemoryBanks = (void(CCONV *)())                                          GetProcAddress(g_hRADLL, "_RA_ClearMemoryBanks");
    _RA_UpdateAppTitle = (void(CCONV *)(const char*))                                 GetProcAddress(g_hRADLL, "_RA_UpdateAppTitle");
    _RA_HandleHTTPResults = (void(CCONV *)())                                         GetProcAddress(g_hRADLL, "_RA_HandleHTTPResults");
    _RA_ConfirmLoadNewRom = (bool(CCONV *)(bool))                                     GetProcAddress(g_hRADLL, "_RA_ConfirmLoadNewRom");
    _RA_CreatePopupMenu = (HMENU(CCONV *)(void))                                      GetProcAddress(g_hRADLL, "_RA_CreatePopupMenu");
    _RA_InvokeDialog = (void(CCONV *)(LPARAM))                                        GetProcAddress(g_hRADLL, "_RA_InvokeDialog");
    _RA_SetPaused = (void(CCONV *)(bool))                                             GetProcAddress(g_hRADLL, "_RA_SetPaused");
    _RA_OnLoadState = (void(CCONV *)(const char*))                                    GetProcAddress(g_hRADLL, "_RA_OnLoadState");
    _RA_OnSaveState = (void(CCONV *)(const char*))                                    GetProcAddress(g_hRADLL, "_RA_OnSaveState");
    _RA_OnReset = (void(CCONV *)())                                                   GetProcAddress(g_hRADLL, "_RA_OnReset");
    _RA_DoAchievementsFrame = (void(CCONV *)())                                       GetProcAddress(g_hRADLL, "_RA_DoAchievementsFrame");
    _RA_SetConsoleID = (int(CCONV *)(unsigned int))                                   GetProcAddress(g_hRADLL, "_RA_SetConsoleID");
    _RA_HardcoreModeIsActive = (int(CCONV *)())                                       GetProcAddress(g_hRADLL, "_RA_HardcoreModeIsActive");
    _RA_WarnDisableHardcore = (bool(CCONV *)(const char*))                            GetProcAddress(g_hRADLL, "_RA_WarnDisableHardcore");
    _RA_InstallSharedFunctions = (void(CCONV *)(bool(*)(), void(*)(), void(*)(), void(*)(), void(*)(char*), void(*)(), void(*)(const char*))) GetProcAddress(g_hRADLL, "_RA_InstallSharedFunctionsExt");

    return _RA_IntegrationVersion ? _RA_IntegrationVersion() : "0.000";
}

static unsigned long long ParseVersion(const char* sVersion)
{
    char* pPart;

    unsigned int major = strtoul(sVersion, &pPart, 10);
    if (*pPart == '.')
        ++pPart;

    unsigned int minor = strtoul(pPart, &pPart, 10);
    if (*pPart == '.')
        ++pPart;

    unsigned int patch = strtoul(pPart, &pPart, 10);
    if (*pPart == '.')
        ++pPart;

    unsigned int revision = strtoul(pPart, &pPart, 10);

    // 64-bit max signed value is 9223 37203 68547 75807
    unsigned long long version = (major * 100000) + minor;
    version = (version * 100000) + patch;
    version = (version * 100000) + revision;
    return version;
}

//	Console IDs: see enum EmulatorID in header
void RA_Init(HWND hMainHWND, int nConsoleID, const char* sClientVersion)
{
    const char* sVerInstalled = _RA_InstallIntegration();

    const char* sHostName = nullptr;
    if (_RA_HostName != nullptr)
        sHostName = _RA_HostName();

    if (sHostName == nullptr)
    {
        sHostName = "www.retroachievements.org";
    }
    else if (_RA_InitOffline != nullptr && strcmp(sHostName, "OFFLINE") == 0)
    {
        _RA_InitOffline(hMainHWND, nConsoleID, sClientVersion);
        return;
    }

    DWORD nBytesRead = 0;
    DWORD nStatusCode = 0;
    char buffer[1024];
    ZeroMemory(buffer, 1024);
    if (DoBlockingHttpGet(sHostName, "LatestIntegration.html", buffer, 1024, &nBytesRead, &nStatusCode) == FALSE)
    {
        if (_RA_InitOffline != nullptr)
        {
            sprintf_s(buffer, sizeof(buffer) / sizeof(buffer[0]), "Cannot access %s (status code %u)\nWorking offline.", sHostName, nStatusCode);
            MessageBoxA(nullptr, buffer, "Warning", MB_OK | MB_ICONWARNING);

            _RA_InitOffline(hMainHWND, nConsoleID, sClientVersion);
        }
        else
        {
            sprintf_s(buffer, sizeof(buffer) / sizeof(buffer[0]), "Cannot access %s (status code %u)\nPlease try again later.", sHostName, nStatusCode);
            MessageBoxA(nullptr, buffer, "Warning", MB_OK | MB_ICONWARNING);

            RA_Shutdown();
        }
        return;
    }

    // remove trailing whitespace
    size_t nIndex = strlen(buffer);
    while (nIndex > 0 && isspace(buffer[nIndex - 1]))
        --nIndex;
    buffer[nIndex] = '\0';

    // expected response is a single line containing the version of the DLL available on the server (i.e. 0.074 or 0.074.1)
    const unsigned long long nLatestDLLVer = ParseVersion(buffer);

    unsigned long long nVerInstalled = ParseVersion(sVerInstalled);
    if (nVerInstalled < nLatestDLLVer)
    {
        RA_Shutdown();	//	Unhook the DLL, it's out of date. We may need to overwrite it.

        char sErrorMsg[2048];
        sprintf_s(sErrorMsg, 2048, "%s\nLatest Version: %s\n%s",
            nVerInstalled == 0 ?
            "Cannot find or load RA_Integration.dll" :
            "A new version of the RetroAchievements Toolset is available!",
            buffer,
            "Automatically update your RetroAchievements Toolset file?");

        int nMBReply = MessageBoxA(nullptr, sErrorMsg, "Warning", MB_YESNO | MB_ICONWARNING);
        if (nMBReply == IDYES)
        {
            FetchIntegrationFromWeb(sHostName, &nStatusCode);

            if (nStatusCode == 200)
            {
                sVerInstalled = _RA_InstallIntegration();
                nVerInstalled = ParseVersion(sVerInstalled);
            }

            if (nVerInstalled < nLatestDLLVer)
            {
                sprintf_s(buffer, sizeof(buffer) / sizeof(buffer[0]), "Failed to update Toolset (status code %u).", nStatusCode);
                MessageBoxA(nullptr, buffer, "Error", MB_OK | MB_ICONERROR);
            }
        }
    }

    if (nVerInstalled < nLatestDLLVer)
    {
        RA_Shutdown();

        sprintf_s(buffer, sizeof(buffer) / sizeof(buffer[0]), "The latest Toolset is required to earn achievements.");
        MessageBoxA(nullptr, buffer, "Warning", MB_OK | MB_ICONWARNING);
    }
    else
    {
        if (!_RA_InitI(hMainHWND, nConsoleID, sClientVersion))
            RA_Shutdown();
    }
}

void RA_InstallSharedFunctions(bool(*fpIsActive)(void), void(*fpCauseUnpause)(void), void(*fpCausePause)(void), void(*fpRebuildMenu)(void), void(*fpEstimateTitle)(char*), void(*fpResetEmulation)(void), void(*fpLoadROM)(const char*))
{
    _RA_GameIsActive = fpIsActive;
    _RA_CauseUnpause = fpCauseUnpause;
    _RA_CausePause = fpCausePause;
    _RA_RebuildMenu = fpRebuildMenu;
    _RA_GetEstimatedGameTitle = fpEstimateTitle;
    _RA_ResetEmulation = fpResetEmulation;
    _RA_LoadROM = fpLoadROM;

    //	Also install *within* DLL! FFS
    if (_RA_InstallSharedFunctions != nullptr)
        _RA_InstallSharedFunctions(fpIsActive, fpCauseUnpause, fpCausePause, fpRebuildMenu, fpEstimateTitle, fpResetEmulation, fpLoadROM);
}

void RA_Shutdown()
{
    //	Call shutdown on toolchain
    if (_RA_Shutdown != nullptr)
        _RA_Shutdown();

    //	Clear func ptrs
    _RA_IntegrationVersion = nullptr;
    _RA_InitI = nullptr;
    _RA_Shutdown = nullptr;
    _RA_UpdateOverlay = nullptr;
    _RA_UpdatePopups = nullptr;
    _RA_RenderOverlay = nullptr;
    _RA_RenderPopups = nullptr;
    _RA_OnLoadNewRom = nullptr;
    _RA_IdentifyRom = nullptr;
    _RA_ActivateGame = nullptr;
    _RA_InstallMemoryBank = nullptr;
    _RA_ClearMemoryBanks = nullptr;
    _RA_UpdateAppTitle = nullptr;
    _RA_HandleHTTPResults = nullptr;
    _RA_ConfirmLoadNewRom = nullptr;
    _RA_CreatePopupMenu = nullptr;
    _RA_InvokeDialog = nullptr;
    _RA_SetPaused = nullptr;
    _RA_OnLoadState = nullptr;
    _RA_OnSaveState = nullptr;
    _RA_OnReset = nullptr;
    _RA_DoAchievementsFrame = nullptr;
    _RA_InstallSharedFunctions = nullptr;

    _RA_GameIsActive = nullptr;
    _RA_CauseUnpause = nullptr;
    _RA_CausePause = nullptr;
    _RA_RebuildMenu = nullptr;
    _RA_GetEstimatedGameTitle = nullptr;
    _RA_ResetEmulation = nullptr;
    _RA_LoadROM = nullptr;
    _RA_SetConsoleID = nullptr;
    _RA_HardcoreModeIsActive = nullptr;
    _RA_WarnDisableHardcore = nullptr;
    _RA_AttemptLogin = nullptr;

    //	Uninstall DLL
    if (g_hRADLL)
    {
        FreeLibrary(g_hRADLL);
        g_hRADLL = nullptr;
    }
}

#endif //RA_EXPORTS
