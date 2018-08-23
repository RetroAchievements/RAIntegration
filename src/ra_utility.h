#ifndef RA_UTILITY_H
#define RA_UTILITY_H
#pragma once

#include "ra_fwd.h"
#include "ra_type_traits.h"

namespace ra {

// Requires: NarrowType is smaller than WideType and both are Arithmetic types (Integral or FloatingPoint)
template<
    typename NarrowType,
    typename WideType,
    class = std::enable_if_t<
    (is_smaller_type_v<NarrowType, WideType>) and
    (std::is_arithmetic_v<NarrowType> and std::is_arithmetic_v<WideType>)
    >
> _NODISCARD _CONSTANT_FN
narrow_cast(_In_ WideType wt) noexcept { return static_cast<NarrowType>(wt); }

template<typename SignedType, class = std::enable_if_t<std::is_signed_v<SignedType>>> _NODISCARD _CONSTANT_FN
to_unsigned(_In_ SignedType st) noexcept { return static_cast<std::make_unsigned_t<SignedType>>(st); }

template<typename UnsignedType, class = std::enable_if_t<std::is_unsigned_v<UnsignedType>>> _NODISCARD _CONSTANT_FN
to_signed(_In_ UnsignedType st) noexcept { return static_cast<std::make_signed_t<UnsignedType>>(st); }

template<typename Arithmetic, class = std::enable_if_t<std::is_arithmetic_v<Arithmetic>>> _NODISCARD ra::tstring
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

template<typename Arithmetic, class = std::enable_if_t<std::is_arithmetic_v<Arithmetic>>> _NODISCARD _CONSTANT_FN
to_floating(_In_ Arithmetic a) noexcept { return static_cast<double>(a); }

template<
    typename Arithmetic,
    typename Arithmetic2 = Arithmetic,
    class = std::enable_if_t<std::is_arithmetic_v<Arithmetic> and std::is_arithmetic_v<Arithmetic2>>
> _NODISCARD _CONSTANT_FN
divide(_In_ Arithmetic a, _In_ Arithmetic2 b) noexcept
{
    // These are all constant expressions so it should be relatively fast
    // Was originally going to do a division operator but it would have to be a member function
    if (b == Arithmetic2{})
        return double{}; // return an empty double (0.0) if b is an empty Arithmetic2 (depends)
    return std::divides<double>()(to_floating(a), to_floating(b));
}

template<
    typename FloatingPoint,
    class = std::enable_if_t<std::is_floating_point_v<FloatingPoint> and std::is_signed_v<FloatingPoint>>
>
_NODISCARD _CONSTANT_FN ftol(_In_ FloatingPoint fp) noexcept { return std::lround(fp); }

template<
    typename FloatingPoint,
    class = std::enable_if_t<std::is_floating_point_v<FloatingPoint> and std::is_unsigned_v<FloatingPoint>>
>
_NODISCARD _CONSTANT_FN ftoul(_In_ FloatingPoint fp) noexcept { return to_unsigned(std::lround(fp)); }

template<typename Arithmetic, class = std::enable_if_t<std::is_arithmetic_v<Arithmetic>>> _NODISCARD _CONSTANT_FN
sqr(_In_ Arithmetic a) noexcept { return std::pow(a, 2); }

template<typename Integral, class = std::enable_if_t<std::is_integral_v<Integral>>> _NODISCARD _CONSTANT_FN
is_even(_In_ Integral i) noexcept { return ((i % 2) == Integral{}); }

template<typename Integral, class = std::enable_if_t<std::is_integral_v<Integral>>> _NODISCARD _CONSTANT_FN
is_odd(_In_ Integral i) noexcept { return !(is_even(i)); }

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_VAR
etoi(_In_ Enum e) noexcept { return static_cast<std::underlying_type_t<Enum>>(e); }

// function alias template for etoi (EnumToIntegral)
template<typename Enum> _NODISCARD _CONSTANT_VAR to_integral = etoi<Enum>;

/// <summary>Calculates the size of any standard fstream.</summary>
/// <param name="filename">The filename.</param>
/// <typeparam name="CharT">
///   The character type, it should be auto deduced if valid. Otherwise you'll
///   get an intellisense error.
/// </typeparam>
/// <typeparam name="Traits">
///   The character traits of <typeparamref name="CharT" />.
/// </typeparam>
/// <returns>The size of the file stream.</returns>
/// <remarks>
///   <paramref name="filename" /> must be a <c>std::basic_string</c>.
/// </remarks>
template<typename CharT, class = std::enable_if_t<is_char_v<CharT>>> _NODISCARD _CONSTANT_FN
filesize(_In_ std::basic_string<CharT>& filename) noexcept
->decltype(std::declval<std::basic_fstream<CharT>&>().tellg())
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


// Don't depend on std::rel_ops for stuff here because they assume the types are the same
namespace rel_ops {

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_VAR
operator==(_In_ const Enum a, _In_ const decltype(etoi(std::declval<Enum&>())) b) noexcept
{
    return (etoi(a) == b);
}

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_VAR
operator==(_In_ const decltype(etoi(std::declval<Enum&>())) a, _In_ const Enum b) noexcept
{
    return (a == etoi(b));
}

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_VAR
operator!=(_In_ const Enum a, _In_ const decltype(etoi(std::declval<Enum&>())) b) noexcept
{
    return (!(etoi(a) == b));
}

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_VAR
operator!=(_In_ const decltype(etoi(std::declval<Enum&>())) a, _In_ const Enum b) noexcept
{
    return (!(a == etoi(b)));
}

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_VAR
operator<(_In_ const Enum a, _In_ const decltype(etoi(std::declval<Enum&>())) b) noexcept
{
    return (etoi(a) < b);
}

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_VAR
operator<(_In_ const decltype(etoi(std::declval<Enum&>())) a, _In_ const Enum b) noexcept
{
    return (a < etoi(b));
}

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_VAR
operator>(_In_ const Enum a, _In_ const decltype(etoi(std::declval<Enum&>())) b) noexcept
{
    return (b < etoi(a));
}

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_VAR
operator>(_In_ const decltype(etoi(std::declval<Enum&>())) a, _In_ const Enum b) noexcept
{
    return (etoi(b) < a);
}

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_VAR
operator<=(_In_ const Enum a, _In_ const decltype(etoi(std::declval<Enum&>())) b) noexcept
{
    return (!(b < etoi(a)));
}

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_VAR
operator<=(_In_ const decltype(etoi(std::declval<Enum&>())) a, _In_ const Enum b) noexcept
{
    return (!(etoi(b) < a));
}

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_VAR
operator>=(_In_ const decltype(etoi(std::declval<Enum&>())) a, _In_ const Enum b) noexcept
{
    return (!(a < etoi(b)));
}

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>> _NODISCARD _CONSTANT_VAR
operator>=(_In_ const Enum a, _In_ const decltype(etoi(std::declval<Enum&>())) b) noexcept
{
    return (!(etoi(a) < b));
}

} // namespace rel_ops

} // namespace ra


#endif // !RA_UTILITY_H
