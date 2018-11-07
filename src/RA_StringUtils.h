#ifndef RA_STRINGUTILS_H
#define RA_STRINGUTILS_H
#pragma once

#include "ra_utility.h"

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

/// <summary>
/// Constructs a <see cref="std::basic_string" /> from a format string and parameters
/// </summary>
template<typename CharT, typename = std::enable_if_t<is_char_v<CharT>>>
_NODISCARD inline auto StringPrintf(_In_z_ _Printf_format_string_ const CharT* const __restrict sFormat, ...)
{
    assert(sFormat != nullptr);
    std::basic_string<CharT> sFormatted;

    va_list pArgs;
    va_start(pArgs, sFormat);
    int nNeeded = 0;
    if constexpr(std::is_same_v<CharT, char>)
    {
        assert(std::string_view{ sFormat }.find("%n") == std::string_view::npos);
        assert(sFormatted.capacity() > 0U && (sFormatted.capacity() < RSIZE_MAX/sizeof(char)));
        nNeeded = std::vsnprintf(sFormatted.data(), sFormatted.capacity(), sFormat, pArgs);
    }
    else if constexpr(std::is_same_v<CharT, wchar_t>)
    {
        // needs to be more than default (7 bytes, "3.5" chars?) (4096 bytes, 2048 chars)
        // must be a bug in MSVC
        sFormatted.reserve(4096U);
        assert(std::wstring_view{ sFormat }.find(L"%n") == std::wstring_view::npos);
        assert(sFormatted.capacity() > 0U && (sFormatted.capacity() < RSIZE_MAX/sizeof(wchar_t)));
        nNeeded = std::vswprintf(sFormatted.data(), sFormatted.capacity(), sFormat, pArgs);
    }
    va_end(pArgs);
    assert(nNeeded >= 0);
    assert(nNeeded < RSIZE_MAX/sizeof(CharT));
    if (nNeeded >= 0)
    {
        if (ra::to_unsigned(nNeeded) >= sFormatted.size())
        {
            sFormatted.resize(ra::to_unsigned(nNeeded + 1));
            int nCheck = 0;
            va_start(pArgs, sFormat);
            if constexpr (std::is_same_v<CharT, char>)
                nCheck = std::vsnprintf(sFormatted.data(), sFormatted.capacity(), sFormat, pArgs);
            else if constexpr (std::is_same_v<CharT, wchar_t>)
                nCheck = std::vswprintf(sFormatted.data(), sFormatted.capacity(), sFormat, pArgs);
            va_end(pArgs);
            assert(nCheck == nNeeded);
        }

        sFormatted.resize(ra::to_unsigned(nNeeded));
    }

    return sFormatted;
}
// This one does depend on the character set
template<typename Arithmetic, class = std::enable_if_t<std::is_arithmetic_v<Arithmetic>>>
_NODISCARD inline auto to_tstring(_In_ Arithmetic a) noexcept
{
#if _MBCS
    return std::to_string(a);
#elif _UNICODE
    return std::to_wstring(a);
#else
#error Unknown character set detected, only MultiByte and Unicode character sets are supported!
#endif // _MBCS
} // end function to_tstring

// More functions to be Unicode compatible w/o sacrificing MBCS
/* A wrapper for converting a string to an unsigned long depending on _String */
/* The template parameter does not need to be specified */
template<typename CharT, typename = std::enable_if_t<ra::is_char_v<CharT>>>
_Success_(0 < return < ULONG_MAX) _NODISCARD inline auto __cdecl
tcstoul(_In_z_                   const CharT* __restrict str,
        _Out_opt_ _Deref_post_z_ CharT**      __restrict endptr = nullptr,
        _In_                     int                     base   = 0 /*auto-detect base*/)
{
    auto ret = 0UL;
    if constexpr (std::is_same_v<CharT, char>)
        ret = std::strtoul(str, endptr, base);
    else if constexpr (std::is_same_v<CharT, wchar_t>)
        ret = std::wcstoul(str, endptr, base);

    assert((errno != ERANGE) && (ret != ULONG_MAX)); // out of range error
    assert(ret != 0); // invalid argument, no conversion done
    return ret;
} /* end function tcstoul */

/// <summary>
///   Returns the number of characters of a null-terminated byte string or wide-character string depending on
///   the character set specified
/// </summary>
/// <param name="_Str">The string.</param>
/// <returns>Number of characters</returns>
/// <remarks>The template argument does not have to be specified.</remarks>
template<typename CharT, typename = std::enable_if_t<ra::is_char_v<CharT>>>
_NODISCARD _Success_(0 < return < strsz) inline auto __cdecl
tcslen_s(_In_reads_or_z_(strsz) const CharT* const str, _In_ std::size_t strsz = UINT16_MAX) noexcept
{
    assert(str != nullptr);
    std::size_t ret = 0U;
    if constexpr (std::is_same_v<CharT, char>)
        ret = strnlen_s(str, strsz);
    else if constexpr (std::is_same_v<CharT, wchar_t>)
        ret = wcsnlen_s(str, strsz);

    assert(ret != 0); // somehow str was a nullptr
    assert(ret != strsz); // str isn't null-terminated
    return ret;
} /* end function tcslen_s */

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
