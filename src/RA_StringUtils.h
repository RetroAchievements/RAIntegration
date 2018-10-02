#ifndef RA_STRINGUTILS_H
#define RA_STRINGUTILS_H
#pragma once

#include "ra_utility.h" // "redefinitions"

#include <string>

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
_NODISCARD std::wstring Widen(_Inout_ std::wstring&& wstr) noexcept;
_NODISCARD std::string Narrow(_In_z_ const char* str);
_NODISCARD std::string Narrow(_In_ const std::string& wstr);
_NODISCARD std::string Narrow(_Inout_ std::string&& wstr) noexcept;

/// <summary>Removes one "\r", "\n", or "\r\n" from the end of a string.</summary>
/// <returns>Reference to <paramref name="str" /> for chaining.</returns>
_NODISCARD std::string& TrimLineEnding(_Inout_ std::string& str) noexcept;
_NODISCARD std::string ByteAddressToString(_In_ ByteAddress nAddr);

template<typename CharT, class = std::enable_if_t<is_char_v<CharT>>>
_NODISCARD inline auto NativeStr(_In_ const CharT* str)
{
    static_assert(!std::is_same_v<CharT, unsigned char>, "Conversion for unsigned strings is currently unsupported!");
#if _MBCS
    return Narrow(str);
#elif _UNICODE
    return Widen(str);
#else
#error Unknown character set detected! Only MultiByte and Unicode are supported!
#endif // _MBCS
}

template<typename CharT, class = std::enable_if_t<is_char_v<CharT>>>
_NODISCARD inline auto NativeStr(_In_ const std::basic_string<CharT>& str)
{
    static_assert(!std::is_same_v<CharT, unsigned char>, "Conversion for unsigned strings is currently unsupported!");
#if _MBCS
    return Narrow(str);
#elif _UNICODE
    return Widen(str);
#else
#error Unknown character set detected! Only MultiByte and Unicode are supported!
#endif // _MBCS
}

template<typename CharT, class = std::enable_if_t<is_char_v<CharT>>>
_NODISCARD inline auto NativeStr(_In_ std::basic_string<CharT>&& str) noexcept
{
    static_assert(!std::is_same_v<CharT, unsigned char>, "Conversion for unsigned strings is currently unsupported!");
#if _MBCS
    return Narrow(std::forward<std::basic_string<CharT>>(str));
#elif _UNICODE
    return Widen(std::forward<std::basic_string<CharT>>(str));
#else
#error Unknown character set detected! Only MultiByte and Unicode are supported!
#endif // _MBCS
}

} // namespace ra


#endif // !RA_STRINGUTILS_H
