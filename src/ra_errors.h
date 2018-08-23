#ifndef RA_ERRORS_H
#define RA_ERRORS_H
#pragma once


#define NOMINMAX
#include <WTypes.h>    
#include <system_error>

namespace ra {

using tstring = std::basic_string<TCHAR>;

_EXTERN_C
_Success_(return != 0)
int CALLBACK ShowError(_In_ const tstring& message, _In_opt_ HWND hwnd = nullptr) noexcept;

// Shows the last system error with a message without throwing
int CALLBACK ShowLastError() noexcept;
_END_EXTERN_C

/// <summary>
///   This will throw the last <c>system_error</c> exception reported by windows,
///   calling functions should at least call <see cref="ra::ShowError" /> when
///   catching it so people can visually identify the exception.
/// </summary>
[[noreturn]] void ThrowLastError();
// returns the last system error as an std::error_code, used when throwing system_error exceptions
_NODISCARD std::error_code CALLBACK GetLastErrorCode() noexcept;

// Returns the last Win32 error, in string format. Returns an empty string if there
// is no error. This is mainly for throwing std::system_error
_NODISCARD tstring CALLBACK GetLastErrorMsg() noexcept;



} // namespace ra

#endif // !RA_ERRORS_H
