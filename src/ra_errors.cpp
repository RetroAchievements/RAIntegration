#include "ra_errors.h"

#include <ra_utility>
#include "RA_Core.h"
#include <Windows.h>


namespace ra {

auto CALLBACK
GetLastErrorCode() noexcept
->std::error_code{ return { static_cast<int>(::GetLastError()), std::system_category() };}

_Use_decl_annotations_ int CALLBACK
ShowError(const tstring& message, HWND hwnd) noexcept
{
    return ::MessageBox(hwnd, message.c_str(), TEXT("Error"), MB_OK | MB_ICONERROR);
}

_CONSTANT_FN CALLBACK ValidModule() noexcept -> decltype(std::declval<std::add_pointer_t<HINSTANCE__>>()!= nullptr)
{
    return { g_hThisDLLInst != nullptr };
}


int CALLBACK
ShowLastError() noexcept
{
    tstring myMsg{ tstring{"Win32 Code: "} + to_tstring(::GetLastError()) + TEXT('\n') };
    myMsg.append(GetLastErrorMsg() + TEXT('\n'));
    return ShowError(myMsg);
}

// Returns the last Win32 error, in string format. Returns an empty string if there
// is no error. This is mainly for throwing std::system_error
tstring CALLBACK
GetLastErrorMsg() noexcept
{
    //Get the error message, if any.
    auto errorID{ ::GetLastError() };
    if (errorID == 0U)
        return TEXT("No error message has been recorded");

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

void CALLBACK ThrowLastError() { throw std::system_error{ GetLastErrorCode(), GetLastErrorMsg() }; }


} // namespace ra
