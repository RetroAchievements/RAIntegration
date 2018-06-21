#include "ra_errors.h"
#include "RA_Core.h"
  

namespace ra {

auto CALLBACK GetLastErrorCode() noexcept -> std::error_code
{
    return { static_cast<int>(::GetLastError()), std::system_category() };
}

_Use_decl_annotations_
int CALLBACK ShowError(const tstring& message, HWND hwnd) noexcept {
    return ::MessageBox(hwnd, message.c_str(), TEXT("Error"), MB_OK | MB_ICONERROR);
}

bool CALLBACK InvalidModule() noexcept { return !g_hThisDLLInst; }

int CALLBACK ShowLastError() noexcept
{
    // real quick workaround I got something in another branch -Samer
#if _MBCS
    auto myErrorCode{ std::to_string(::GetLastError()) };
#elif _UNICODE
    auto myErrorCode{ std::to_wstring(::GetLastError()) };
#else
#error Unsupported character set detected!
#endif // _MBCS

    auto myMsg{ tstring{"Win32 Code: "} + myErrorCode + '\n' + GetLastErrorMsg() + '\n' };
    return ShowError(myMsg);
}

// Returns the last Win32 error, in string format. Returns an empty string if there
// is no error. This is mainly for throwing std::system_error
tstring CALLBACK GetLastErrorMsg() noexcept
{
    //Get the error message, if any.
    auto errorID{ ::GetLastError() };
    if (errorID == 0U)
        return TEXT("No error message has been recorded");


    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms679351(v=vs.85).aspx
    // lpBuffer can't be larger than larger than 64KB (64x1024 bytes)

    // have no choice
    LPTSTR lpBuffer{ nullptr };
    auto size{
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, errorID,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPTSTR>(&lpBuffer), 0UL, nullptr) };

    tstring message(lpBuffer, size);
    LocalFree(static_cast<HLOCAL>(lpBuffer));
    lpBuffer = nullptr;

    return message;
}

void ThrowLastError() { throw std::system_error{ GetLastErrorCode(), GetLastErrorMsg() }; }


} // namespace ra
