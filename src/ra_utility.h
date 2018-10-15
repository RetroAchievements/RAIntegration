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

#pragma warning(push)
#pragma warning(disable : 26472) // we aren't using gsl and our version of narrow_cast does not trigger this
template<typename SignedType, class = std::enable_if_t<std::is_signed_v<SignedType>>> _NODISCARD _CONSTANT_FN
to_unsigned(_In_ SignedType st) noexcept { return static_cast<std::make_unsigned_t<SignedType>>(st); }

template<typename UnsignedType, class = std::enable_if_t<std::is_unsigned_v<UnsignedType>>> _NODISCARD _CONSTANT_FN
to_signed(_In_ UnsignedType st) noexcept { return static_cast<std::make_signed_t<UnsignedType>>(st); }
#pragma warning(pop)

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

// not sure if this stuff should be exposed directly to ra without a using
enum class ByteWidthDifference
{
    // Sometimes Zero is needed because size_type/difference_type depends on the platform
    Zero  = sizeof(std::int64_t) - sizeof(std::int64_t),
    Four  = sizeof(std::int64_t) - sizeof(std::int32_t),
    Six   = sizeof(std::int64_t) - sizeof(std::int16_t),
    Seven = sizeof(std::int64_t) - sizeof(std::int8_t),
};

namespace detail {

enum class CastsNeeded
{
    One  = etoi(ByteWidthDifference::Zero),
    Two   = etoi(ByteWidthDifference::Four),
    Three = etoi(ByteWidthDifference::Six),
    Four  = etoi(ByteWidthDifference::Seven)
};

} /* namespace detail */

template<typename Arithmetic, typename = std::enable_if_t<std::is_arithmetic_v<Arithmetic>>>
_NODISCARD _CONSTANT_FN flip_sign(_In_ Arithmetic a) noexcept
{
    if constexpr (std::is_signed_v<Arithmetic>)
        return to_unsigned(a);
    if constexpr (std::is_unsigned_v<Arithmetic>)
        return to_signed(a);
};

/// <summary>
///   Converts '<paramref name="from" />' into a <typeparamref name="NarrowType" />.
/// </summary>
/// <typeparam name="Narrowed">
///   A narrower version of <typeparamref name="WideType" />. This template parameter must be
///   specified.
/// </typeparam>
/// <typeparam name="WideType">
///   The <c>value_type</c> of the input, must have a larger size than
///   <typeparamref name="NarrowType" />.
/// </typeparam>
/// <param name="from">The arithmetic value to narrowed.</param>
/// <returns><typeparamref name="WideType" /> as a <typeparamref name="NarrowType" />.</returns>
/// <remarks>
///   This function is used for explicit narrowing conversions when a platform type is different than
///   a function parameter's type, such as <c>std::size_t</c> and <c>unsigned int</c>.
/// </remarks>
template<
    typename NarrowType,
    typename WideType,
    typename = std::enable_if_t<std::is_arithmetic_v<NarrowType> && std::is_arithmetic_v<WideType> &&
    has_smaller_or_same_size_than_v<NarrowType, WideType>>
> _NODISCARD _CONSTANT_FN
narrow_cast(_In_ WideType from) noexcept
{
    _CONSTANT_LOC difference{ sizeof(WideType) - sizeof(NarrowType) };
    switch (const auto eNumCast{ itoe<detail::CastsNeeded>(difference) }; eNumCast)
    {
        case detail::CastsNeeded::One:
        {
            if constexpr(has_same_sign_v<NarrowType, WideType>)
                return static_cast<NarrowType>(from);
            else if constexpr (has_opposite_sign_v<NarrowType, WideType>)
                return static_cast<NarrowType>(flip_sign(from));
        }break;

        case detail::CastsNeeded::Two:
        {
            if constexpr (has_same_sign_v<NarrowType, WideType>)
                return static_cast<NarrowType>(static_cast<WideType>(from));
            else if constexpr (has_opposite_sign_v<NarrowType, WideType>)
                return static_cast<NarrowType>(flip_sign(from));
        }break;

        case detail::CastsNeeded::Three:
        {
            if constexpr (has_opposite_sign_v<NarrowType, WideType> && std::is_signed_v<NarrowType>)
                return static_cast<NarrowType>(static_cast<std::int32_t>(flip_sign(from)));

            else if constexpr (has_opposite_sign_v<NarrowType, WideType> && std::is_unsigned_v<NarrowType>)
                return static_cast<NarrowType>(static_cast<std::uint32_t>(flip_sign(from)));

            else if constexpr (has_same_sign_v<NarrowType, WideType> && std::is_signed_v<NarrowType>)
                return static_cast<NarrowType>(static_cast<std::int32_t>(static_cast<WideType>(from)));

            else if constexpr (has_same_sign_v<NarrowType, WideType> && std::is_unsigned_v<NarrowType>)
                return static_cast<NarrowType>(static_cast<std::uint32_t>(static_cast<WideType>(from)));
        }break;

        case detail::CastsNeeded::Four:
        {
            if constexpr (has_opposite_sign_v<NarrowType, WideType> && std::is_signed_v<NarrowType>)
            {
                return (static_cast<NarrowType>(static_cast<std::int16_t>(static_cast<std::int32_t>(
                    flip_sign(from)))));
            }
            else if constexpr (has_opposite_sign_v<NarrowType, WideType> && std::is_unsigned_v<NarrowType>)
            {
                return (static_cast<NarrowType>(static_cast<std::uint16_t>(static_cast<std::uint32_t>(
                    flip_sign(from)))));
            }
            else if constexpr (has_same_sign_v<NarrowType, WideType> && std::is_signed_v<NarrowType>)
                return (static_cast<NarrowType>(static_cast<std::int16_t>(static_cast<std::int32_t>(
                    static_cast<WideType>(from)))));

            else if constexpr (has_same_sign_v<NarrowType, WideType> && std::is_unsigned_v<NarrowType>)
            {
                return (static_cast<NarrowType>(static_cast<std::uint16_t>(static_cast<std::uint32_t>(
                    static_cast<WideType>(from)))));
            }
        }
    }
    return static_cast<NarrowType>(from);
}

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

/// <summary>
///   Divides <paramref name="a" /> by <paramref name="b" />, use for extremely precise division.
/// </summary>
/// <typeparam name="Arithmetic">A valid arithmetic type.</typeparam>
/// <typeparam name="Arithmetic2">
///   A valid arithmetic type, can be the same as <typeparamref name="Arithmetic" />.
/// </typeparam>
/// <param name="a">The dividend.</param>
/// <param name="b">
///   The divisor, if this is <c>0</c>, the function will return <c>0</c> instead of failing.
/// </param>
/// <returns>
///   The quotient resulting from dividing <paramref name="a" /> and <paramref name="b" />.
/// </returns>
template<
    typename Arithmetic,
    typename Arithmetic2,
    class = std::enable_if_t<std::is_arithmetic_v<Arithmetic> && std::is_arithmetic_v<Arithmetic2>>
> _NODISCARD _CONSTANT_FN
divide(_In_ Arithmetic a, _In_ Arithmetic2 b) noexcept
{
    if (b == Arithmetic2{})
        return 0.0L;
    return std::divides<long double>()(to_floating(a), to_floating(b));
}

} // namespace ra

#endif // !RA_UTILITY_H
