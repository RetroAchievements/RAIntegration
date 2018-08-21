#ifndef RA_UTILITY_H
#define RA_UTILITY_H
#pragma once

#include "ra_fwd.h"
#include "ra_type_traits.h"

namespace ra {

inline namespace int_literals {

// Don't feel like casting non-stop
// There's also standard literals for strings and clock types

// N.B.: The parameter can only be either "unsigned long-long", char16_t, or char32_t

_NODISCARD _CONSTANT_FN operator""_z(_In_ unsigned long long n) noexcept { return static_cast<std::size_t>(n); }
_NODISCARD _CONSTANT_FN operator""_i(_In_ unsigned long long n) noexcept { return static_cast<std::intptr_t>(n); }
_NODISCARD _CONSTANT_FN operator""_tt(_In_ unsigned long long n) noexcept { return static_cast<std::time_t>(n); }
_NODISCARD _CONSTANT_FN operator""_dw(_In_ unsigned long long n) noexcept { return static_cast<::DWORD>(n); }
_NODISCARD _CONSTANT_FN operator""_ss(_In_ unsigned long long n) noexcept { return static_cast<std::streamsize>(n); }
_NODISCARD _CONSTANT_FN operator""_h(_In_ unsigned long long n) noexcept { return static_cast<std::int16_t>(n); }
_NODISCARD _CONSTANT_FN operator""_hu(_In_ unsigned long long n) noexcept { return static_cast<std::uint16_t>(n); }
_NODISCARD _CONSTANT_FN operator""_hhu(_In_ unsigned long long n) noexcept { return static_cast<std::uint8_t>(n); }
_NODISCARD _CONSTANT_FN operator""_dp(_In_ unsigned long long n) noexcept { return static_cast<DataPos>(n); }
_NODISCARD _CONSTANT_FN operator""_ba(_In_ unsigned long long n) noexcept { return static_cast<ByteAddress>(n); }
_NODISCARD _CONSTANT_FN operator""_achid(_In_ unsigned long long n) noexcept { return static_cast<AchievementID>(n); }
_NODISCARD _CONSTANT_FN operator""_lbid(_In_ unsigned long long n) noexcept { return static_cast<LeaderboardID>(n); }
_NODISCARD _CONSTANT_FN operator""_gameid(_In_ unsigned long long n) noexcept { return static_cast<GameID>(n); }

} // inline namespace int_literals

// NB: The "requires" doesn't exist in VC++ and probably won't until C++20 so we are using type_traits instead

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

/// <summary>Converts the integral representation of an <c>enum</c> to a string.</summary>
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
/// <typeparam name="Integral"></typeparam>
/// <param name="i">
///   The <typeparamref name="Integral" /> to cast. Must match the enum's <c>std::underlying_type</c>.
/// </param>
/// <returns><paramref name="i" /> as an <typeparamref name="Enum" />.</returns>
/// <remarks>Cast <paramref name="i" /> the enum's <c>std::underlying_type</c> if it's not already.</remarks>
template<
    typename Enum,
    typename Integral,
    typename = std::enable_if_t<std::is_integral_v<Integral> and
    std::is_enum_v<Enum> and
    std::is_same_v<Integral, std::underlying_type_t<Enum>>>
> _NODISCARD _CONSTANT_FN
itoe(_In_ Integral i) noexcept { return static_cast<Enum>(i); }

// function alias template for etoi (EnumToIntegral)
template<typename Enum> _NODISCARD _CONSTANT_VAR to_integral = etoi<Enum>;

template<typename Enum>
_NODISCARD _CONSTANT_VAR to_enum = itoe<Enum, std::underlying_type_t<Enum>>;

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
template<
    typename CharT,
    typename Traits = std::char_traits<CharT>,
    class = std::enable_if_t<is_char_v<CharT>>> _NODISCARD _CONSTANT_FN
filesize(_In_ const std::basic_string<CharT>& filename) noexcept
{
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

// c string version, don't attempt to use unsigned char as CharT or it will show up as <<error type>>
template<typename CharT, class = std::enable_if_t<is_char_v<CharT>>>
_Success_(return != 0) _NODISCARD _CONSTANT_FN __cdecl
strlen_as_int(_In_z_ const CharT* str) noexcept
{
    // TBD: Should we care about unsigned char? It could slow things down... doesn't seem like it's needed anyway
    static_assert(!std::is_same_v<CharT, std::uint8_t>);

    if constexpr (std::is_same_v<CharT, char>)
        return(static_cast<int>(to_signed(std::strlen(str)))); // size_t varies
    else /* wchar_t is the only possibility at this point */
        return(static_cast<int>(to_signed(std::wcslen(str))));
}

// This is mostly needed in GDI functions, filters out char16_t and char32_t but there's another check to make sure
// CharT isn't unsigned char.
_Success_(return != 0)
/// <summary>
///   Casts the length of <paramref name="str" /> to <c>int</c> where a <c>value_type</c> of <c>int</c> is excepted.
///   DO NOT use an rvalue reference to an std::string here, as the pointer will dangle.
/// </summary>
/// <typeparam name="CharT">
///   Represents a character type, which should be identical to <c>std::basic_string_view::value_type</c>. It must either
///   be <c>char</c> or <c>wchar_t</c>, otherwise the library will fail to compile.
/// </typeparam>
/// <param name="str">A <c>const_reference</c> to a <c>std::basic_string</c>.</param>
/// <returns>Length of <paramref name="str" /> as an <c>int</c>.</returns>
/// <remarks>This function's main purpose is to be used in Windows GDI function, it may be applied whether</remarks>
template<typename CharT, class = std::enable_if_t<is_char_v<CharT>>> _NODISCARD inline auto
strlen_as_int(_In_ const std::basic_string<CharT>& str) noexcept
{
    static_assert(!std::is_same_v<CharT, std::uint8_t>);
    return(static_cast<int>(to_signed(str.length()))); // std::basic_string<CharT>::size_type varies
}

/// <summary>
///   <para>Special bitwise operator overloads.</para>
///   <para>Mainly used for <c>enum class</c>.</para>
///   <para>
///     Don't depend on <a href="https://en.cppreference.com/w/cpp/utility">std::rel_ops</a> for stuff here because
///     they assume the types are the same; symmetrical (for this reason they will be eventually deprectated in the
///     next version of isocpp).
///   </para>
/// </summary>
/// <remarks>
///   <a href="https://en.cppreference.com/w/cpp/types/byte">std::byte</a> has similar overloads but only work with
///   <c>std::byte</c>. These are meant to work with any <c>enum class</c>. It also as a
///   <c>std::to_integer</c> function, though it doesn't actually work (type constraints were not written in SFINAE
///   context).
/// </remarks>
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


/// <summary>Special bitwise operator overloads. Mainly used for <c>enum class</c>s.</summary>
/// <remarks>
///   <a href="https://en.cppreference.com/w/cpp/types/byte">std::byte</a> has similar overloads but only work with
///   <c>std::byte</c>. These are meant to work with <i>any</i><c>enum class</c>. It also as a
///   <c>std::to_integer</c> function, though it doesn't actually work (type constraints were not written in SFINAE
///   context).
/// </remarks>
namespace bitwise_ops {

// NB: You will still need to use etoi if a bitwise operator is used directly in an if statement. Also, bit-wise
// assignments aren't marked [[nodiscard]] because value used is self-contained and can be safely discarded.

template<
    typename Enum,
    class = std::enable_if_t<std::is_enum_v<Enum> and std::is_convertible_v<std::underlying_type_t<Enum>, bool>>
> _NODISCARD _CONSTANT_FN
operator&(_In_ const Enum a, _In_ const Enum b) noexcept { return(itoe<Enum>(etoi(a) & etoi(b))); }

template<typename Enum, class = std::enable_if_t<std::is_enum_v<Enum>>> _CONSTANT_FN&
operator&=(_Inout_ Enum& a, _In_ const Enum b) noexcept { return (a = a & b); }

template<typename Enum, class = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator|(_In_ const Enum a, _In_ const Enum b) noexcept { return(itoe<Enum>(etoi(a) | etoi(b))); }

template<typename Enum, class = std::enable_if_t<std::is_enum_v<Enum>>> _CONSTANT_FN&
operator|=(_Inout_ Enum& a, _In_ const Enum b) noexcept { return (a = a | b); }

template<typename Enum, class = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator^(_In_ const Enum a, _In_ const Enum b) noexcept { return(itoe<Enum>(etoi(a) ^ etoi(b))); }

template<typename Enum, class = std::enable_if_t<std::is_enum_v<Enum>>> _CONSTANT_FN&
operator^=(_Inout_ Enum& a, _In_ const Enum b) noexcept { return (a = a ^ b); }

// The underlying_type must be signed or you could risk an arithmetic overflow
template<
    typename Enum,
    class = std::enable_if_t<std::is_enum_v<Enum> and std::is_signed_v<std::underlying_type_t<Enum>>>
> _NODISCARD _CONSTANT_FN
operator~(_In_ const Enum& a) noexcept { return(itoe<Enum>(~etoi(a))); }

template<typename Enum, class = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator<<(const Enum a, const std::underlying_type_t<Enum> b) noexcept
{
    // too many risks of using signed numbers with bit shift, so we will make them unsigned no matter what
    using underlying_type = std::underlying_type_t<Enum>;
    if constexpr (std::is_signed_v<underlying_type>)
        return (itoe<Enum>(to_unsigned(etoi(a)) << to_unsigned(b)));
    else
        return (itoe<Enum>((etoi(a)) << b));
}

template<typename Enum, class = std::enable_if_t<std::is_enum_v<Enum>>> _CONSTANT_FN&
operator<<=(_Inout_ Enum& a, _In_ const std::underlying_type_t<Enum> b) noexcept { return (a = a << b); }

template<typename Enum, class = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_FN
operator>>(const Enum a, const std::underlying_type_t<Enum> b) noexcept
{
    // too many risks of using signed numbers with bit shift, so we will make them unsigned no matter what
    using underlying_type = std::underlying_type_t<Enum>;
    if constexpr (std::is_signed_v<underlying_type>)
        return (itoe<Enum>(to_unsigned(etoi(a)) >> to_unsigned(b)));
    else
        return (itoe<Enum>((etoi(a)) >> b));
}

template<typename Enum, class = std::enable_if_t<std::is_enum_v<Enum>>> _CONSTANT_FN&
operator>>=(_Inout_ Enum& a, _In_ const std::underlying_type_t<Enum> b) noexcept { return (a = a >> b); }

} // namespace bitwise_ops
} // namespace ra


#endif // !RA_UTILITY_H
