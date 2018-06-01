#ifndef RA_ERRORS_H
#define RA_ERRORS_H
#pragma once

// TODO: Make a pch for repetitive includes

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>    
#include <system_error>

namespace ra {

using tstring = std::basic_string<TCHAR>;

_EXTERN_C
int CALLBACK ShowError(_In_ const tstring& message, _In_ HWND hwnd = ::GetActiveWindow()) noexcept;

_NODISCARD auto CALLBACK
ValidModule() noexcept->decltype(std::declval<std::add_pointer_t<HINSTANCE__>>() != nullptr);

_NODISCARD inline constexpr auto CALLBACK
ValidHandle(_In_ HANDLE h) noexcept->decltype(h != INVALID_HANDLE_VALUE){ return { h != INVALID_HANDLE_VALUE }; }

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
_NODISCARD auto CALLBACK GetLastErrorCode() noexcept->std::error_code;

// Returns the last Win32 error, in string format. This is mainly for throwing std::system_error
_NODISCARD tstring CALLBACK GetLastErrorMsg() noexcept;

} // namespace ra

#endif // !RA_ERRORS_H
