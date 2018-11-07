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
// TODO: Finish narrow_cast in another PR
// we don't use gsl::narrow_cast, ra::narrow_cast won't trigger this when its done
#pragma warning(disable : 26472)
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
#pragma warning(pop)

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

// So we don't have to #define _CRT_SECURE_NO_WARNINGS, C99/11 style (no-alias optimization)
// To use instead of std::fopen for unique_ptr
template<typename CharT, typename = std::enable_if_t<is_char_v<CharT>>>
_NODISCARD FILE* __cdecl fopen_s(_In_z_ const CharT* __restrict filename,
                                 _In_z_ const CharT* __restrict mode,
                                 _In_opt_     FILE*  __restrict streamptr = nullptr)
{
    assert(filename != nullptr);
    assert(mode != nullptr);
    if constexpr (std::is_same_v<CharT, char>)
    {
        const auto errnoVal = ::fopen_s(&streamptr, filename, mode);
        assert(streamptr != nullptr); // we need it as a param so it doesn't alias
        assert(errnoVal == 0);
        return streamptr;
    }
    else if constexpr (std::is_same_v<CharT, wchar_t>)
    {
        const auto errnoVal = ::_wfopen_s(&streamptr, filename, mode);
        assert(streamptr != nullptr);
        assert(errnoVal == 0);
        return streamptr;
    }
}

} // namespace ra

#endif // !RA_UTILITY_H
