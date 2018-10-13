#ifndef RA_UTILITY_H
#define RA_UTILITY_H
#pragma once

#include "ra_fwd.h"
#include "ra_type_traits.h"

// don't like asserts but it's good for debugging
#ifndef assert
#include <cassert>
#endif /* !assert */

/*
    For most of these functions you won't have to specify the template argument as it will be auto-deduced unless
    stated otherwise.
*/
namespace ra {

template<typename SignedType, class = std::enable_if_t<std::is_signed_v<SignedType>>> _NODISCARD _CONSTANT_FN
to_unsigned(_In_ SignedType st) noexcept { return static_cast<std::make_unsigned_t<SignedType>>(st); }

template<typename UnsignedType, class = std::enable_if_t<std::is_unsigned_v<UnsignedType>>> _NODISCARD _CONSTANT_FN
to_signed(_In_ UnsignedType st) noexcept { return static_cast<std::make_signed_t<UnsignedType>>(st); }

/// <summary>
///   Converts '<paramref name="from" />' into a <typeparamref name="NarrowedType" />.
/// </summary>
/// <typeparam name="Narrowed">
///   A narrower version of <typeparamref name="WideType" />. This template parameter must be
///   specified.
/// </typeparam>
/// <typeparam name="WideType">
///   The <c>value_type</c> of the input, must have a larger size than
///   <typeparamref name="NarrowedType" />.
/// </typeparam>
/// <param name="from">The arithmetic value to narrowed.</param>
/// <returns><typeparamref name="WideType" /> as a <typeparamref name="NarrowedType" />.</returns>
/// <remarks>
///   This function is used for explicit narrowing conversions when a platform type is different than
///   a function parameter's type, such as <c>std::size_t</c> and <c>unsigned int</c>.
/// </remarks>
template<
    typename NarrowedType,
    typename WideType,
    class = std::enable_if_t<
    std::is_arithmetic_v<NarrowedType> &&
    std::is_arithmetic_v<WideType> &&
    has_smaller_or_same_size_than_v<NarrowedType, WideType>>
> _NODISCARD _CONSTANT_FN
narrow_cast(_In_ WideType from) noexcept { return static_cast<NarrowedType>(static_cast<WideType>(from)); }

template<typename Arithmetic, class = std::enable_if_t<std::is_arithmetic_v<Arithmetic>>> _NODISCARD inline auto
to_tstring(_In_ Arithmetic a) noexcept
{
#if _MBCS
    return std::to_string(a);
#elif _UNICODE
    return std::to_wstring(a);
#else
#error Unknown character set detected, only MultiByte and Unicode character sets are supported!
#endif // _MBCS
} // end function to_tstring

/// <summary>Casts <paramref name="a" /> into a <typeparamref name="FloatingPoint" />.</summary>
/// <typeparam name="Arithmetic">A valid arithmetic type.</typeparam>
/// <param name="a">The number to be converted.</param>
/// <returns>
///   The return type depends on the size of <typeparamref name="Arithmetic" />. If it's 8 bytes, it will
///   <c>double</c>, if it's anything less it will be float. <c>long double</c> is not considered as it is currently
///   the same as <c>double</c> size-wise.
/// </returns>
template<typename Arithmetic, class = std::enable_if_t<std::is_arithmetic_v<Arithmetic>>> _NODISCARD _CONSTANT_FN
to_floating(_In_ Arithmetic a) noexcept
{
    if constexpr (is_same_size_v<Arithmetic, double>)
        return static_cast<double>(a);
    if constexpr (has_smaller_size_than_v<Arithmetic, double>)
        return static_cast<float>(a);
}

template<typename FloatingPoint, class = std::enable_if_t<std::is_floating_point_v<FloatingPoint>>>
_NODISCARD _CONSTANT_FN ftoi(_In_ FloatingPoint fp) noexcept { return std::lround(fp); }

template<typename FloatingPoint, class = std::enable_if_t<std::is_floating_point_v<FloatingPoint>>>
_NODISCARD _CONSTANT_FN ftou(_In_ FloatingPoint fp) noexcept { return to_unsigned(std::lround(fp)); }

template<typename Arithmetic, class = std::enable_if_t<std::is_arithmetic_v<Arithmetic>>>
_NODISCARD _CONSTANT_FN sqr(_In_ Arithmetic a) noexcept { return std::pow(a, 2); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_VAR
etoi(_In_ Enum e) noexcept { return static_cast<std::underlying_type_t<Enum>>(e); }

template<
    typename Enum,
    typename Integral = std::underlying_type_t<Enum>,
    typename = std::enable_if_t<
    std::is_enum_v<Enum> &&
    std::is_integral_v<Integral> &&
    std::is_convertible_v<Integral, std::underlying_type_t<Enum>>>
> _NODISCARD _CONSTANT_VAR
itoe(_In_ Integral i) noexcept { return static_cast<Enum>(i); }

// function alias templates for etoi (EnumToIntegral) and itoe (IntegralToEnum)
template<typename Enum> _CONSTANT_VAR to_integral{ etoi<Enum> };
template<typename Enum, typename Integral = std::underlying_type_t<Enum>>
_CONSTANT_VAR to_enum{ itoe<Enum, Integral> };

/// <summary>Calculates the size of any standard fstream.</summary>
/// <param name="filename">The filename.</param>
/// <typeparam name="CharT">
///   The character type, it should be auto deduced if valid. Otherwise you'll
///   get an intellisense error.
/// </typeparam>
/// <returns>The size of the file stream.</returns>
template<typename CharT, class = std::enable_if_t<is_char_v<CharT>>> _NODISCARD _CONSTANT_FN
filesize(_In_ const std::basic_string<CharT>& filename) noexcept
{
    using file_type = std::basic_fstream<CharT>;
    file_type file{ filename, std::ios::in | std::ios::ate | std::ios::binary };
    return file.tellg();
} // end function filesize

/// <summary>Calculates the size of any standard fstream.</summary>
/// <param name="filename">The filename.</param>
/// <typeparam name="CharT">
///   The character type, it should be auto deduced if valid. Otherwise you'll get an intellisense error.
/// </typeparam>
/// <returns>The size of the file stream.</returns>
/// <remarks>This overload is provided as a safe-guard against unexpected lifetimes.</remarks>
template<typename CharT, class = std::enable_if_t<is_char_v<CharT>>> _NODISCARD _CONSTANT_FN
filesize(std::basic_string<CharT>&& filename) noexcept
{
    using file_type = std::basic_fstream<CharT>;
    auto bstr{ std::move(filename) };
    file_type file{ bstr, std::ios::in | std::ios::ate | std::ios::binary };
    return file.tellg();
} // end function filesize

// More functions to be Unicode compatible w/o sacrificing MBCS
_EXTERN_C
/* A wrapper for converting a string to an unsigned long depending on the character set specified */
_NODISCARD inline auto __cdecl
tcstoul(_In_z_ LPCTSTR const _String,
         _Out_opt_ _Deref_post_z_ LPTSTR* _EndPtr = nullptr,
         _In_ int _Radix = 10) /* calling functions might throw */
{
#if _MBCS
    return std::strtoul(_String, _EndPtr, _Radix);
#elif _UNICODE
    return std::wcstoul(_String, _EndPtr, _Radix);
#else
#error Unsupported character set detected.
#endif /* _MBCS */
} /* end function tcstoul */

/*
    https://docs.microsoft.com/en-us/cpp/c-language/maximum-string-length?view=vs-2017
    Error Checking: First part might happen in MBCS mode but the standard does not reserve an error code for it.
    Remarks: The second part is Microsoft Specific. A single null-terminated c string may not exceed 2048 bytes,
             but may reach a total size of 65,535 bytes after concatenation.
    This check will only occur during code analysis, the standard does not have data contracts yet,
    i.e, expects, ensures... (GSL does though however).
*/
_Success_((return != SIZE_MAX) && (return <= 2048U))
/* Returns the length of a null-terminated byte string or wide-character string depending on the character set specified */
_NODISCARD inline auto __cdecl tstrlen(_In_z_ LPCTSTR _Str) noexcept
{
#if _MBCS
    return std::strlen(_Str);   
#elif _UNICODE
    return std::wcslen(_Str);
#else
#error Unsupported character set detected.
#endif /* _MBCS */
} /* end function tstrlen */

_END_EXTERN_C

// Pro-tip: don't use C functions, if you must there must be a lot of checks done.
// TODO: Check if an arg is nullptr, it's kind of complicated
/// <summary>Writes output to <paramref name="buffer" /> with arguments specified.</summary>
/// <typeparam name="CharT">
///   A character type, specify this explicitly to use a different character set than configured.
///   Valid character types are <c>char</c>, <c>signed char</c>, and <c>wchar_T</c>. Using any other
///   type will disable this function.
/// </typeparam>
/// <param name="buffer">
///   Pointer to a character string to write to, if this is <c>nullptr</c>, the operation is
///   undefined.
/// </param>
/// <param name="bufsz">
///   Up to <c><paramref name="bufsz" /> - 1</c> characters may be written, plus the null terminator.
///   If the size is <c>0</c> or greater than <c>RSIZE_MAX</c>, the operation is undefined.
/// </param>
/// <param name="format">
///   Pointer to a null-terminated multibyte string or wide-character string specifying how to
///   interpret the data. The format string consists of ordinary multibyte characters(except %),
///   which are copied unchanged into the output stream, and conversion specifications. The %n
///   specifier is considered undefined.
/// </param>
/// <param name="...args">
///   Forwarded arguments specifying data to print. If the number of arguments is more than the
///   number of format specifiers present, the operation is undefined.
/// </param>
/// <returns>
///   The number of characters written to <paramref name="buffer" /> excluding the null character.
/// </returns>
template<typename CharT = TCHAR, typename ...Args, typename = std::enable_if_t<is_char_v<CharT>>>
_Success_(return >= 0) /*_NODISCARD*/ inline auto __cdecl
stprintf_s(_Inout_updates_bytes_(bufsz)  CharT*       const __restrict buffer,
           _In_range_(1, RSIZE_MAX)      rsize_t      const            bufsz,
           _In_z_ _Printf_format_string_ const CharT* const __restrict format,
           _In_                          Args&&...                     args)
{
    assert((buffer != nullptr) && (format != nullptr));
    assert((bufsz > 0) && (bufsz <= RSIZE_MAX));
    
    if constexpr(std::is_same_v<CharT, char>)
    {
        [[maybe_unused]] std::string_view test{ format };
        assert(test.find("%n") == std::string::npos);
        [[maybe_unused]] const auto num_specs{ std::count(test.begin(), test.end(), '%') };
        assert(num_specs == sizeof...(args));
    }
    else if constexpr(std::is_same_v<CharT, wchar_t>)
    {
        [[maybe_unused]] std::wstring_view test{ format };
        assert(test.find(L"%n") == std::wstring::npos);
        [[maybe_unused]] const auto num_specs{ std::count(test.begin(), test.end(), L'%') };
        assert(num_specs == sizeof...(args));
    }
    
    // MSVC doesn't seem to have constraint handlers like C11
    if constexpr (std::is_same_v<CharT, char>)
    {
        const auto ret{ std::snprintf(buffer, bufsz, format, std::forward<Args>(args)...) };
        assert(std::char_traits<char>::length(buffer) < bufsz);
        assert(ret > 0);
        return ret;
    }
    if constexpr (std::is_same_v<CharT, wchar_t>)
    {
        const auto ret{ std::swprintf(buffer, bufsz, format, std::forward<Args>(args)...) };
        assert(std::char_traits<wchar_t>::length(buffer) < bufsz);
        assert(ret > 0);
        return ret;
    }
}

// Don't depend on std::rel_ops for stuff here because it assumes each parameter has the same type
/// <summary>
///   Relational operator overloads, mainly for comparing an <c>enum class</c> with it's <c>underlying_type</c>.
/// </summary>
namespace rel_ops {

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator==(_In_ Enum a, _In_ std::underlying_type_t<Enum> b) noexcept { return (etoi(a) == b); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator==(_In_ std::underlying_type_t<Enum> a, _In_ Enum b) noexcept { return (a == etoi(b)); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator!=(_In_ Enum a, _In_ std::underlying_type_t<Enum> b) noexcept { return (!(etoi(a) == b)); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator!=(_In_ std::underlying_type_t<Enum> a, _In_ Enum b) noexcept { return (!(a == etoi(b))); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator<(_In_ Enum a, _In_ std::underlying_type_t<Enum> b) noexcept { return (etoi(a) < b); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator<(_In_ std::underlying_type_t<Enum> a, _In_ Enum b) noexcept { return (a < etoi(b)); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator>(_In_ Enum a, _In_ std::underlying_type_t<Enum> b) noexcept { return (b < etoi(a)); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator>(_In_ std::underlying_type_t<Enum> a, _In_ Enum b) noexcept { return (etoi(b) < a); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator<=(_In_ Enum a, _In_ std::underlying_type_t<Enum> b) noexcept { return (!(b < etoi(a))); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator<=(_In_ const std::underlying_type_t<Enum> a, _In_ const Enum b) noexcept { return (!(etoi(b) < a)); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator>=(_In_ std::underlying_type_t<Enum> a, _In_ Enum b) noexcept { return (!(a < etoi(b))); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator>=(_In_ Enum a, _In_ std::underlying_type_t<Enum> b) noexcept { return (!(etoi(a) < b)); }

} // namespace rel_ops

namespace bitwise_ops {

template<typename Enum, class = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator|(_In_ Enum a, _In_ Enum b) noexcept { return(itoe<Enum>(etoi(a) | etoi(b))); }

template<typename Enum, class = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator&(_In_ Enum a, _In_ Enum b) noexcept { return(itoe<Enum>(etoi(a) & etoi(b))); }

template<typename Enum, class = std::enable_if_t<std::is_enum_v<Enum>>> _CONSTANT_FN&
operator|=(_Inout_ Enum& a, _In_ Enum b) noexcept { return (a = a | b); }

template<typename Enum, class = std::enable_if_t<std::is_enum_v<Enum>>> _CONSTANT_FN&
operator&=(_Inout_ Enum& a, _In_ Enum b) noexcept { return (a = a & b); }

} // namespace bitwise_ops
} // namespace ra

#endif // !RA_UTILITY_H
