#include "CppUnitTest.h"

// this has to be forward declared - not sure why - including the header file doesn't work
struct IUnknown;

// NOTE: most of RA_Interface is not actually testable, the purpose of this project is to
// ensure it compiles as the file is not used by the DLL directly. The emulators compile
// the file directly into their code and it handles downloading and hooking up the DLL.
// Those functions which can be mocked will be redefined here and eliminated from the CPP
// file via a "#ifndef RA_UTEST".

// these functions are redefined for the unit tests
static std::wstring GetIntegrationPath();

// ALSO NOTE: the cpp file is pulled in directly, instead of using the header file so we
// can access the file-scoped methods and variables. The cpp file will include the header
//file, so both are tested.
#include "RA_Interface.cpp"

#include "RA_BuildVer.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

static std::wstring GetIntegrationPath()
{
    // attempt to locate the DLL. assume it lives in a folder above the test folder:
    // - bin/release/RA_Integration.dll
    // - bin/release/tests/RA_Interface.tests.dll

    wchar_t sBuffer[2048];

    HMODULE hm;
    if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)&GetIntegrationPath, &hm))
    {
        swprintf_s(sBuffer, sizeof(sBuffer) / sizeof(sBuffer[0]), L"GetModuleHandle failed with error %d", GetLastError());
        Assert::Fail(sBuffer);
    }
    else
    {
        DWORD iIndex = GetModuleFileNameW(hm, sBuffer, 2048);
        do
        {
            while (iIndex > 0 && sBuffer[iIndex - 1] != '\\' && sBuffer[iIndex - 1] != '/')
                --iIndex;
            wcscpy_s(&sBuffer[iIndex], sizeof(sBuffer) / sizeof(sBuffer[0]) - iIndex, L"RA_Integration.dll");

            DWORD dwAttrib = GetFileAttributesW(sBuffer);
            if (dwAttrib != INVALID_FILE_ATTRIBUTES)
                return std::wstring(sBuffer);
        } while (iIndex-- > 0);

        Assert::Fail(L"Could not find RA_Integration.dll");
    }

    return L"";
}

namespace ra {
namespace tests {

TEST_CLASS(RA_Interface_Tests)
{
public:
    static void AssertInstalled()
    {
        Assert::IsNotNull(g_hRADLL);

        Assert::IsNotNull((const void*)_RA_IntegrationVersion);
        Assert::IsNotNull((const void*)_RA_HostName);
        Assert::IsNotNull((const void*)_RA_InitI);
        Assert::IsNotNull((const void*)_RA_InitOffline);
        Assert::IsNotNull((const void*)_RA_Shutdown);
        Assert::IsNotNull((const void*)_RA_AttemptLogin);
        Assert::IsNotNull((const void*)_RA_NavigateOverlay);
        Assert::IsNotNull((const void*)_RA_UpdateOverlay);
        Assert::IsNotNull((const void*)_RA_RenderOverlay);
        Assert::IsNotNull((const void*)_RA_IsOverlayFullyVisible);
        Assert::IsNotNull((const void*)_RA_OnLoadNewRom);
        Assert::IsNotNull((const void*)_RA_IdentifyRom);
        Assert::IsNotNull((const void*)_RA_ActivateGame);
        Assert::IsNotNull((const void*)_RA_InstallMemoryBank);
        Assert::IsNotNull((const void*)_RA_ClearMemoryBanks);
        Assert::IsNotNull((const void*)_RA_UpdateAppTitle);
        Assert::IsNotNull((const void*)_RA_HandleHTTPResults);
        Assert::IsNotNull((const void*)_RA_ConfirmLoadNewRom);
        Assert::IsNotNull((const void*)_RA_CreatePopupMenu);
        Assert::IsNotNull((const void*)_RA_InvokeDialog);
        Assert::IsNotNull((const void*)_RA_SetPaused);
        Assert::IsNotNull((const void*)_RA_OnLoadState);
        Assert::IsNotNull((const void*)_RA_OnSaveState);
        Assert::IsNotNull((const void*)_RA_OnReset);
        Assert::IsNotNull((const void*)_RA_DoAchievementsFrame);
        Assert::IsNotNull((const void*)_RA_SetConsoleID);
        Assert::IsNotNull((const void*)_RA_HardcoreModeIsActive);
        Assert::IsNotNull((const void*)_RA_WarnDisableHardcore);
        Assert::IsNotNull((const void*)_RA_InstallSharedFunctions);
    }

    void AssertShutdown()
    {
        Assert::IsNull((const void*)_RA_IntegrationVersion);
        Assert::IsNull((const void*)_RA_HostName);
        Assert::IsNull((const void*)_RA_InitI);
        Assert::IsNull((const void*)_RA_InitOffline);
        Assert::IsNull((const void*)_RA_Shutdown);
        Assert::IsNull((const void*)_RA_AttemptLogin);
        Assert::IsNull((const void*)_RA_NavigateOverlay);
        Assert::IsNull((const void*)_RA_UpdateOverlay);
        Assert::IsNull((const void*)_RA_RenderOverlay);
        Assert::IsNull((const void*)_RA_IsOverlayFullyVisible);
        Assert::IsNull((const void*)_RA_OnLoadNewRom);
        Assert::IsNull((const void*)_RA_IdentifyRom);
        Assert::IsNull((const void*)_RA_ActivateGame);
        Assert::IsNull((const void*)_RA_InstallMemoryBank);
        Assert::IsNull((const void*)_RA_ClearMemoryBanks);
        Assert::IsNull((const void*)_RA_UpdateAppTitle);
        Assert::IsNull((const void*)_RA_HandleHTTPResults);
        Assert::IsNull((const void*)_RA_ConfirmLoadNewRom);
        Assert::IsNull((const void*)_RA_CreatePopupMenu);
        Assert::IsNull((const void*)_RA_InvokeDialog);
        Assert::IsNull((const void*)_RA_SetPaused);
        Assert::IsNull((const void*)_RA_OnLoadState);
        Assert::IsNull((const void*)_RA_OnSaveState);
        Assert::IsNull((const void*)_RA_OnReset);
        Assert::IsNull((const void*)_RA_DoAchievementsFrame);
        Assert::IsNull((const void*)_RA_SetConsoleID);
        Assert::IsNull((const void*)_RA_HardcoreModeIsActive);
        Assert::IsNull((const void*)_RA_WarnDisableHardcore);
        Assert::IsNull((const void*)_RA_InstallSharedFunctions);

        Assert::IsNull(g_hRADLL);
    }

    TEST_METHOD(TestInitializeShutdownInterface)
    {
        // ensure all pointers are initially set to null
        AssertShutdown();

        // load the dll, and make sure all pointers are set
        Assert::AreEqual(RA_INTEGRATION_VERSION, _RA_InstallIntegration());
        AssertInstalled();

        // unload the dll, and make sure all pointers are set back to null
        RA_Shutdown();
        AssertShutdown();
    }
};

} // namespace tests
} // namespace ra
