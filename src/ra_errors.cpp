#include "ra_errors.h"

namespace ra {

auto GetLastErrorCode() noexcept -> std::error_code {
    auto dwErrVal{ ::GetLastError() };
    return { static_cast<int>(dwErrVal), std::system_category() };
}

int CALLBACK ShowError(const tstring & message, HWND hwnd) noexcept {
    return ::MessageBox(hwnd, message.c_str(), TEXT("Error"), MB_OK | MB_ICONERROR);
}

// Returns the last Win32 error, in string format. Returns an empty string if there
// is no error. This is mainly for throwing std::system_error
tstring GetLastErrorMsg() {
    //Get the error message, if any.
    auto errorID{ ::GetLastError() };
    if (errorID == 0)
        return tstring{}; //No error message has been recorded

    tstring messageBuffer;
    auto size{
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, errorID,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            messageBuffer.data(), DWORD{ 0 }, nullptr) };

    tstring message{ messageBuffer.data(), size };



    return message;
}


} // namespace ra
