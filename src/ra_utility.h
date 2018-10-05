#ifndef RA_UTILITY_H
#define RA_UTILITY_H
#pragma once

#include "ra_fwd.h"
#include "ra_type_traits.h"

/*
    For most of these functions you won't have to specify the template argument as it will be auto-deduced unless
    stated otherwise.
*/
namespace ra {
template<typename SignedType, class = std::enable_if_t<std::is_signed_v<SignedType>>> _NODISCARD _CONSTANT_FN
to_unsigned(_In_ SignedType st) noexcept { return static_cast<std::make_unsigned_t<SignedType>>(st); }

template<typename UnsignedType, class = std::enable_if_t<std::is_unsigned_v<UnsignedType>>> _NODISCARD _CONSTANT_FN
to_signed(_In_ UnsignedType st) noexcept { return static_cast<std::make_signed_t<UnsignedType>>(st); }

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
    if constexpr (sizeof(Arithmetic) < sizeof(double))
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

using LPCTSTR = const TCHAR*;
using LPTSTR = TCHAR*;

// More functions to be Unicode compatible w/o sacrificing MBCS
_EXTERN_C
/* A wrapper for converting a string to an unsigned long depending on the character set specified */
_NODISCARD inline auto __cdecl
tstrtoul(_In_z_ LPCTSTR _String,
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
} /* end function tstrtoul */
_END_EXTERN_C

// Don't depend on std::rel_ops for stuff here because it assumes each parameter has the same type
/// <summary>
///   Relational operator overloads, mainly for comparing an <c>enum class</c> with it's <c>underlying_type</c>.
/// </summary>
namespace rel_ops {

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator==(_In_ const Enum a, _In_ const std::underlying_type_t<Enum> b) noexcept { return (etoi(a) == b); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator==(_In_ const std::underlying_type_t<Enum> a, _In_ const Enum b) noexcept { return (a == etoi(b)); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator!=(_In_ const Enum a, _In_ const std::underlying_type_t<Enum> b) noexcept { return (!(etoi(a) == b)); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator!=(_In_ const std::underlying_type_t<Enum> a, _In_ const Enum b) noexcept { return (!(a == etoi(b))); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator<(_In_ const Enum a, _In_ const std::underlying_type_t<Enum> b) noexcept { return (etoi(a) < b); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator<(_In_ const std::underlying_type_t<Enum> a, _In_ const Enum b) noexcept { return (a < etoi(b)); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator>(_In_ const Enum a, _In_ const std::underlying_type_t<Enum> b) noexcept { return (b < etoi(a)); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator>(_In_ const std::underlying_type_t<Enum> a, _In_ const Enum b) noexcept { return (etoi(b) < a); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator<=(_In_ const Enum a, _In_ const std::underlying_type_t<Enum> b) noexcept { return (!(b < etoi(a))); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator<=(_In_ const std::underlying_type_t<Enum> a, _In_ const Enum b) noexcept { return (!(etoi(b) < a)); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator>=(_In_ const std::underlying_type_t<Enum> a, _In_ const Enum b) noexcept { return (!(a < etoi(b))); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator>=(_In_ const Enum a, _In_ const std::underlying_type_t<Enum> b) noexcept { return (!(etoi(a) < b)); }

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
