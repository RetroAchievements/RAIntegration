#ifndef RA_ERRORS_H
#define RA_ERRORS_H
#pragma once


#define WIN32_LEAN_AND_MEAN
#include <Windows.h>     // for GetLastError, FORMAT_MESSAGE_ALLOCATE_BUFFER, FORMAT_MESSAGE_FROM_SYSTEM, FORMAT_MESSAGE_IGNORE_INSERTS, FormatMessage
#include <system_error>  // For everything else
namespace ra {

using tstring = std::basic_string<TCHAR>;

// can't be constexpr because of GetLastError


_EXTERN_C
int CALLBACK ShowError(const tstring& message, HWND hwnd = ::GetActiveWindow()) noexcept;
_END_EXTERN_C

// returns the last system error as an std::error_code, used when throwing system_error exceptions
auto GetLastErrorCode() noexcept->std::error_code;

// Returns the last Win32 error, in string format. Returns an empty string if there
// is no error. This is mainly for throwing std::system_error
tstring GetLastErrorMsg();


} // namespace ra

#endif // !RA_ERRORS_H
