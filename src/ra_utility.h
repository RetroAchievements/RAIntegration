#ifndef RA_UTILITY_H
#define RA_UTILITY_H
#pragma once

#include "ra_fwd.h"
#include "ra_type_traits.h"

namespace ra {

// for x86_64 (or x64) calling conventions don't matter but there is a slight performance boost and size reduction
// using __stdcall unless something explicitly define a function with __cdecl. These can't be exported as is anyway.

inline namespace int_literals {

// Don't feel like casting non-stop
// There's also standard literals for strings and clock types

// N.B.: The parameter can only be either "unsigned long-long", char16_t, or char32_t

/// <summary>
///   <para>
///     Use it if you need an unsigned integer without the need for explicit
///     narrowing conversions.
///   </para>
///   <para>
///     usage: <c>auto some_int{21_z};</c> where <c>21</c> is
///            <paramref name="n" />.
///   </para>
/// </summary>
/// <param name="n">The integer.</param>
/// <returns>The integer as <c>std::size_t</c>.</returns>
_NODISCARD _CONSTANT_FN operator""_z(_In_ unsigned long long n) noexcept
{
    return static_cast<std::size_t>(n);
} // end operator""_z


/// <summary>
  ///   <para>
  ///     Use it if you need an integer without the need for explicit narrowing
  ///     conversions.
  ///   </para>
  ///   <para>
  ///     usage: <c>auto some_int{1_i};</c> where <c>1</c> is
  ///            <paramref name="n" />.
  ///   </para>
  /// </summary>
  /// <param name="n">The integer.</param>
  /// <returns>The integer as <c>std::intptr_t</c>.</returns>
_NODISCARD _CONSTANT_FN operator""_i(_In_ unsigned long long n) noexcept
{
    return static_cast<std::intptr_t>(n);
} // end operator""_i

  // We need one for DWORD, because it doesn't match LPDWORD for some stuff
_NODISCARD _CONSTANT_FN operator""_dw(_In_ unsigned long long n) noexcept
{
    return static_cast<DWORD>(n);
} // end operator""_dw

  // streamsize varies as well
_NODISCARD _CONSTANT_VAR operator""_ss(_In_ unsigned long long n) noexcept
{
    return static_cast<std::streamsize>(n);
} // end operator""_ss

  // Literal for unsigned short
_NODISCARD _CONSTANT_FN operator""_hu(_In_ unsigned long long n) noexcept
{
    return static_cast<std::uint16_t>(n);
} // end operator""_hu


_NODISCARD _CONSTANT_FN operator""_dp(_In_ unsigned long long n) noexcept
{
    return static_cast<ra::DataPos>(n);
} // end operator""_dp

  // If you follow the standard every alias is considered mutually exclusive, if not don't worry about it
  // These are here if you follow the standards (starts with ISO C++11/C11) rvalue conversions


_NODISCARD _CONSTANT_FN operator""_ba(_In_ unsigned long long n) noexcept
{
    return static_cast<ByteAddress>(n);
} // end operator""_ba


_NODISCARD _CONSTANT_FN operator""_achid(_In_ unsigned long long n) noexcept
{
    return static_cast<AchievementID>(n);
} // end operator""_achid


_NODISCARD _CONSTANT_FN operator""_lbid(_In_ unsigned long long n) noexcept
{
    return static_cast<LeaderboardID>(n);
} // end operator""_lbid


_NODISCARD _CONSTANT_FN operator""_gameid(_In_ unsigned long long n) noexcept
{
    return static_cast<GameID>(n);
} // end operator""_gameid

} // inline namespace int_literals

template<typename SignedType, class = std::enable_if_t<std::is_signed_v<SignedType>>>
_NODISCARD _CONSTANT_FN
to_unsigned(_In_ SignedType st) noexcept { return static_cast<std::make_unsigned_t<SignedType>>(st); }

template<typename UnsignedType, class = std::enable_if_t<std::is_unsigned_v<UnsignedType>>>
_NODISCARD _CONSTANT_FN
to_signed(_In_ UnsignedType st) noexcept { return static_cast<std::make_signed_t<UnsignedType>>(st); }

template<typename Arithmetic, class = std::enable_if_t<std::is_arithmetic_v<Arithmetic>>>
_NODISCARD ra::tstring
to_tstring(_In_ Arithmetic a) noexcept
{
#if _MBCS
    return std::to_string(a);
#elif _UNICODE
    return std::to_wstring(a);
#else
#error Unknown character set detected, only MultiByte and Unicode character sets are supported!
#endif // UNICODE
} // end function to_tstring

template<typename Arithmetic, class = std::enable_if_t<std::is_arithmetic_v<Arithmetic>>>
_NODISCARD _CONSTANT_FN
to_floating(_In_ Arithmetic a) noexcept
{
    if constexpr (is_same_size_v<Arithmetic, float>)
        return static_cast<float>(a);
    else
        return static_cast<double>(a);
}

template<
    typename FloatingPoint,
    class = std::enable_if_t<std::is_floating_point_v<FloatingPoint> and std::is_signed_v<FloatingPoint>>
>
_NODISCARD _CONSTANT_FN __cdecl ftol(_In_ FloatingPoint fp) noexcept { return std::lround(fp); }

template<
    typename FloatingPoint,
    class = std::enable_if_t<std::is_floating_point_v<FloatingPoint> and std::is_unsigned_v<FloatingPoint>>
>
_NODISCARD _CONSTANT_FN __cdecl ftoul(_In_ FloatingPoint fp) noexcept { return to_unsigned(std::lround(fp)); }

template<typename Arithmetic, class = std::enable_if_t<std::is_arithmetic_v<Arithmetic>>>
_NODISCARD _CONSTANT_FN __cdecl
sqr(_In_ Arithmetic a) noexcept { return std::pow(a, 2); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_NODISCARD _CONSTANT_VAR
etoi(_In_ Enum e) noexcept { return static_cast<std::underlying_type_t<Enum>>(e); }

/// <summary>Converts the integral representation of an <typeparamref name="Enum" /> as a string.</summary>
/// <param name="e">The Enum.</param>
/// <returns>Number as string.</returns>
template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_NODISCARD _CONSTANT_VAR
etos(_In_ Enum e) noexcept { return std::to_string(etoi(e)); }


/// <summary>
///   Casts an <typeparamref name="Integral" /> into an <typeparamref name="Enum" />. Only the
///   <typeparamref name="Enum" /> type parameter needs to be specified.
/// </summary>
/// <typeparam name="Enum"></typeparam>
/// <param name="i">
///   The <typeparamref name="Integral" /> to cast. Must match the enum's <c>std::underlying_type</c>.
/// </param>
/// <returns><paramref name="i" /> as an <typeparamref name="Enum" />.</returns>
/// <remarks>Cast <paramref name="i" /> the enum's <c>std::underlying_type</c>.</remarks>
template<
    typename Enum,
    typename Integral = std::underlying_type_t<Enum>,
    typename = std::enable_if_t<std::is_integral_v<Integral> and
    std::is_enum_v<Enum> and
    std::is_same_v<Integral, std::underlying_type_t<Enum>>>
> _NODISCARD _CONSTANT_VAR
itoe(_In_ Integral i) noexcept { return static_cast<Enum>(i); }

// function alias template for etoi (EnumToIntegral)
template<typename Enum> _NODISCARD _CONSTANT_VAR to_integral = etoi<Enum>;

template<typename Enum, typename Integral = std::underlying_type_t<Enum>>
_NODISCARD _CONSTANT_VAR to_enum = itoe<Enum, Integral>;

/// <summary>Calculates the size of any standard fstream.</summary>
/// <param name="filename">The filename.</param>
/// <typeparam name="CharT">
///   The character type, it should be auto deduced if valid. Otherwise you'll get an intellisense error.
/// </typeparam>
/// <typeparam name="Traits">
///   The character traits of <typeparamref name="CharT" />. Should be auto-deduced, Otherwise you'll get an
///   intellisense error.
/// </typeparam>
/// <returns>The size of the file stream.</returns>
template<typename CharT, class = std::enable_if_t<is_char_v<CharT>>> _NODISCARD _CONSTANT_FN
filesize(_In_ std::basic_string<CharT>& filename) noexcept
->decltype(std::declval<std::basic_fstream<CharT>&>().tellg())
{
    // It's always the little things...
    using file_type = std::basic_fstream<CharT>;
    file_type file{ filename, std::ios::in | std::ios::ate | std::ios::binary };
    return file.tellg();
} // end function filesize

// More functions to be Unicode compatible w/o sacrificing MBCS (lots of errors)

_NODISCARD inline auto __cdecl
tstrtoul(_In_z_ LPCTSTR _String,
         _Out_opt_ _Deref_post_z_ LPTSTR* _EndPtr = nullptr,
         _In_ int _Radix = 10) // calling functions might throw
{
#if _MBCS
    return std::strtoul(_String, _EndPtr, _Radix);
#elif _UNICODE
    return std::wcstoul(_String, _EndPtr, _Radix);
#else
#error Unsupported character set detected.
#endif // _MBCS
}

// Don't depend on std::rel_ops for stuff here because they assume the types are the same
namespace rel_ops {

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_Success_(return != false) _NODISCARD _CONSTANT_VAR
operator==(_In_ const Enum a, _In_ const std::underlying_type_t<Enum> b) noexcept { return (etoi(a) == b); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_Success_(return != false) _NODISCARD _CONSTANT_VAR
operator==(_In_ const std::underlying_type_t<Enum> a, _In_ const Enum b) noexcept { return (a == etoi(b)); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_Success_(return != false) _NODISCARD _CONSTANT_VAR
operator!=(_In_ const Enum a, _In_ const std::underlying_type_t<Enum> b) noexcept { return (!(etoi(a) == b)); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_Success_(return != false) _NODISCARD _CONSTANT_VAR
operator!=(_In_ const std::underlying_type_t<Enum> a, _In_ const Enum b) noexcept { return (!(a == etoi(b))); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_Success_(return != false) _NODISCARD _CONSTANT_VAR
operator<(_In_ const Enum a, _In_ const std::underlying_type_t<Enum> b) noexcept { return (etoi(a) < b); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_Success_(return != false) _NODISCARD _CONSTANT_VAR
operator<(_In_ const std::underlying_type_t<Enum> a, _In_ const Enum b) noexcept { return (a < etoi(b)); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_Success_(return != false) _NODISCARD _CONSTANT_VAR
operator>(_In_ const Enum a, _In_ const std::underlying_type_t<Enum> b) noexcept { return (b < etoi(a)); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_Success_(return != false) _NODISCARD _CONSTANT_VAR
operator>(_In_ const std::underlying_type_t<Enum> a, _In_ const Enum b) noexcept { return (etoi(b) < a); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_Success_(return != false) _NODISCARD _CONSTANT_VAR
operator<=(_In_ const Enum a, _In_ const std::underlying_type_t<Enum> b) noexcept { return (!(b < etoi(a))); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_Success_(return != false) _NODISCARD _CONSTANT_VAR
operator<=(_In_ const std::underlying_type_t<Enum> a, _In_ const Enum b) noexcept { return (!(etoi(b) < a)); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_Success_(return != false) _NODISCARD _CONSTANT_VAR
operator>=(_In_ const std::underlying_type_t<Enum> a, _In_ const Enum b) noexcept { return (!(a < etoi(b))); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_Success_(return != false) _NODISCARD _CONSTANT_VAR
operator>=(_In_ const Enum a, _In_ const std::underlying_type_t<Enum> b) noexcept { return (!(etoi(a) < b)); }

} // namespace rel_ops

namespace bitwise_ops {

// NB: You will still need to use etoi if a bitwise operator is used directly in an if statement

template<
    typename Enum,
    class = std::enable_if_t<std::is_enum_v<Enum> and std::is_convertible_v<std::underlying_type_t<Enum>, bool>>
> _NODISCARD _CONSTANT_FN
operator&(_In_ const Enum a, _In_ const Enum b) noexcept { return(itoe<Enum>(etoi(a) & etoi(b))); }

template<typename Enum, class = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN&
operator&=(_Inout_ Enum& a, _In_ const Enum b) noexcept { return (a = a & b); }

template<typename Enum, class = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator|(_In_ const Enum a, _In_ const Enum b) noexcept { return(itoe<Enum>(etoi(a) | etoi(b))); }

template<typename Enum, class = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN&
operator|=(_Inout_ Enum& a, _In_ const Enum b) noexcept { return (a = a | b); }

template<typename Enum, class = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator^(_In_ const Enum a, _In_ const Enum b) noexcept { return(itoe<Enum>(etoi(a) ^ etoi(b))); }

template<typename Enum, class = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN&
operator^=(_Inout_ Enum& a, _In_ const Enum b) noexcept
{
    return (a = a ^ b);
}

// The underlying_type must be signed or you could risk an arithmetic overflow
template<
    typename Enum,
    class = std::enable_if_t<std::is_enum_v<Enum> and std::is_signed_v<std::underlying_type_t<Enum>>>
> _NODISCARD _CONSTANT_FN
operator~(_In_ const Enum& a) noexcept { return(itoe<Enum>(~etoi(a))); }

template<typename Enum, class = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator<<(const Enum a, const std::underlying_type_t<Enum> b) noexcept
{
    // too many risks of using signed numbers with bitwise shit, so we will make them unsigned no matter what
    using underlying_type = std::underlying_type_t<Enum>;
    if constexpr(std::is_signed_v<underlying_type>)
        return (itoe<Enum>(to_unsigned(etoi(a)) << to_unsigned(b)));
    else
        return (itoe<Enum>((etoi(a)) << b));
}

template<typename Enum, class = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN&
operator<<=(_Inout_ Enum& a, _In_ const std::underlying_type_t<Enum> b) noexcept { return (a = a << b); }

template<typename Enum, class = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator>>(const Enum a, const std::underlying_type_t<Enum> b) noexcept
{
    // too many risks of using signed numbers with bitwise shift, so we will make them unsigned no matter what
    using underlying_type = std::underlying_type_t<Enum>;
    if constexpr(std::is_signed_v<underlying_type>)
        return (itoe<Enum>(to_unsigned(etoi(a)) >> to_unsigned(b)));
    else
        return (itoe<Enum>((etoi(a)) >> b));
}

template<typename Enum, class = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN&
operator>>=(_Inout_ Enum& a, _In_ const std::underlying_type_t<Enum> b) noexcept { return (a = a >> b); }

} // namespace bitwise_ops

} // namespace ra


#endif // !RA_UTILITY_H
