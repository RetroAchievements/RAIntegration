#include "WindowsDebuggerDetector.hh"

#include "util/Log.hh"
#include "util/Strings.hh"

#include <TlHelp32.h>

namespace ra {
namespace services {
namespace impl {

static bool IsSuspiciousProcessRunning()
{
    bool bFound = false;

    const HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot)
    {
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(hSnapshot, &pe32))
        {
            do
            {
                if (tcslen_s(pe32.szExeFile) >= 15)
                {
                    std::string sFilename = ra::Narrow(pe32.szExeFile);
                    ra::StringMakeLowercase(sFilename);

                    // cannot reliably detect injection without ObRegisterCallbacks, which requires Vista,
                    // instead just look for the default process names (it's better than nothing)
                    // [Cheat Engine.exe, cheatengine-i386.exe, cheatengine-x86_64.exe]
                    if (sFilename.find("cheat") != std::string::npos &&
                        sFilename.find("engine") != std::string::npos)
                    {
                        RA_LOG_WARN("Cheat Engine detected");
                        bFound = true;
                        break;
                    }
                }
            } while (Process32Next(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    }

    return bFound;
}

bool WindowsDebuggerDetector::IsDebuggerPresent() const
{
#ifdef NDEBUG // allow debugger-limited functionality when using a DEBUG build
    if (::IsDebuggerPresent())
    {
        RA_LOG_WARN("Debugger detected");
        return true;
    }

    BOOL bDebuggerPresent;
    if (::CheckRemoteDebuggerPresent(GetCurrentProcess(), &bDebuggerPresent) && bDebuggerPresent)
    {
        RA_LOG_WARN("Remote debugger detected");
        return true;
    }
#endif

    // real cheating tools don't register as debuggers (or intercept and override the call)
    // also check for known bad agents and assume that the player is using them to cheat
    return IsSuspiciousProcessRunning();
}

} // namespace impl
} // namespace services
} // namespace ra
