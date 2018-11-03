#ifndef RA_STRINGUTILS_H
#define RA_STRINGUTILS_H
#pragma once

namespace ra {

_NODISCARD std::string Narrow(_In_ const std::wstring& wstr);
_NODISCARD std::string Narrow(_Inout_ std::wstring&& wstr) noexcept;
_NODISCARD std::string Narrow(_In_z_ const wchar_t* wstr);
_NODISCARD std::wstring Widen(_In_ const std::string& str);
_NODISCARD std::wstring Widen(_Inout_ std::string&& str) noexcept;
_NODISCARD std::wstring Widen(_In_z_ const char* str);

//	No-ops to help convert:
_NODISCARD std::wstring Widen(_In_z_ const wchar_t* wstr);
_NODISCARD std::wstring Widen(_In_ const std::wstring& wstr);
_NODISCARD std::string Narrow(_In_z_ const char* str);
_NODISCARD std::string Narrow(_In_ const std::string& wstr);

/// <summary>
/// Removes one "\r", "\n", or "\r\n" from the end of a string.
/// </summary>
/// <returns>Reference to <paramref name="str" /> for chaining.</returns>
std::string& TrimLineEnding(_Inout_ std::string& str) noexcept;

// rvalue reference version for the tests
std::string TrimLineEnding(std::string&& str) noexcept;

/// <summary>
/// Constructs a <see cref="std::string" /> from a sprintf format and paramaters
/// </summary>
_NODISCARD std::string StringPrintf(_In_z_ _Printf_format_string_ const char* const sFormat, ...);

/// <summary>
/// Determines if <paramref name="sString" /> starts with <paramref name="sMatch" />.
/// </summary>
_NODISCARD bool StringStartsWith(_In_ const std::wstring& sString, _In_ const std::wstring& sMatch) noexcept;

/// <summary>
/// Determines if <paramref name="sString" /> ends with <paramref name="sMatch" />.
/// </summary>
_NODISCARD bool StringEndsWith(_In_ const std::wstring& sString, _In_ const std::wstring& sMatch) noexcept;

} // namespace ra

#ifdef UNICODE
#define NativeStr(x) ra::Widen(x)
#define NativeStrType std::wstring
#else
#define NativeStr(x) ra::Narrow(x)
#define NativeStrType std::string
#endif

#endif // !RA_STRINGUTILS_H
