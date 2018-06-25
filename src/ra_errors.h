#ifndef RA_ERRORS_H
#define RA_ERRORS_H
#pragma once

// TODO: Make a pch for repetitive includes, for now we'll try to reduce it.
//       STL stuff is header only so we have to include them in headers unless
//       another header includes in its translation
#include <system_error> // this is the minimum we need here
#include <ra_fwd> // this is self-contained




namespace ra {

using tstring = ::std::basic_string<::TCHAR>;

_EXTERN_C
int CALLBACK ShowError(_In_ const tstring& message, _In_ HWND hwnd = ::GetActiveWindow()) noexcept;

// We didn't want to include RA_Core in here
_NODISCARD _CONSTANT_FN CALLBACK
ValidModule() noexcept -> decltype(std::declval<std::add_pointer_t<HINSTANCE__>>()!= nullptr);

_NODISCARD _CONSTANT_FN CALLBACK
ValidHandle(_In_ HANDLE h) -> decltype(h != INVALID_HANDLE_VALUE) { return { h != INVALID_HANDLE_VALUE }; }

/// <summary>
///   Shows the last system error with a message without throwing.
/// </summary>
int CALLBACK ShowLastError() noexcept;
_END_EXTERN_C

/// <summary>
///   This will throw the last <c>system_error</c> exception reported by windows,
///   calling functions should at least call <see cref="ra::ShowError" /> when
///   catching it so people can visually identify the exception.
/// </summary>
[[noreturn]] void CALLBACK ThrowLastError();
 
/// <summary>
///   Returns the last system error as an <c>std::error_code</c>, used when
///   throwing <c>std::system_error</c> exceptions
/// </summary>
_NODISCARD auto CALLBACK GetLastErrorCode() noexcept->std::error_code;

// Returns the last Win32 error, in string format. This is mainly for throwing std::system_error
_NODISCARD tstring CALLBACK GetLastErrorMsg() noexcept;

} // namespace ra

#endif // !RA_ERRORS_H
