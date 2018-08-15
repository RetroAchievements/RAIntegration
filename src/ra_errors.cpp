#include "ra_errors.h"
#include "RA_Core.h"
  
namespace ra {

std::error_code CALLBACK GetLastErrorCode() noexcept
{
    return { to_signed(::GetLastError()), std::system_category() };
}

_Use_decl_annotations_
int CALLBACK ShowError(const tstring& message, HWND hwnd) noexcept
{
    if (hwnd == nullptr)
        hwnd = ::GetActiveWindow();

    return ::MessageBox(hwnd, message.c_str(), TEXT("Error"), MB_OK | MB_ICONERROR);
}

int CALLBACK ShowLastError() noexcept
{
    using tostringstream = std::basic_ostringstream<TCHAR>;
    tostringstream oss;
    oss << _T("Win32 Code: ") << ra::to_tstring(::GetLastError()) << _T('\n') << GetLastErrorMsg() << _T('\n');
    return ShowError(oss.str());
}

// Returns the last Win32 error, in string format. Returns an empty string if there
// is no error. This is mainly for throwing std::system_error
tstring CALLBACK GetLastErrorMsg() noexcept
{
    // Get the error message, if any.
    auto errorID{ ::GetLastError() };
    if (errorID == 0_dw)
        return tstring{};

    LPTSTR lpBuffer{ nullptr };
    auto size{ FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                             FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, errorID,
                             MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPTSTR>(&lpBuffer), 0_dw,
                             nullptr) };

    tstring message(lpBuffer, size);
    LocalFree(static_cast<HLOCAL>(lpBuffer));
    lpBuffer = nullptr;

    return message;
}

void ThrowLastError() { throw std::system_error{ GetLastErrorCode(), Narrow(GetLastErrorMsg()) }; }


} // namespace ra
