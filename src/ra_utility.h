#ifndef RA_UTILITY_H
#define RA_UTILITY_H
#pragma once

#include "ra_fwd.h"
#include "ra_type_traits.h"

/*
   For most of these functions you won't have to specify the template argument
   as it will be auto-deduced unless stated otherwise.
*/
namespace ra {

template<typename SignedType, typename = std::enable_if_t<std::is_signed_v<SignedType>>>
_NODISCARD _CONSTANT_FN to_unsigned(_In_ SignedType st) noexcept
{
    return gsl::narrow_cast<std::make_unsigned_t<SignedType>>(st);
}

template<typename UnsignedType, typename = std::enable_if_t<std::is_unsigned_v<UnsignedType>>>
_NODISCARD _CONSTANT_FN to_signed(_In_ UnsignedType st) noexcept
{
    return gsl::narrow_cast<std::make_signed_t<UnsignedType>>(st);
}

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_NODISCARD _CONSTANT_VAR etoi(_In_ Enum e) noexcept
{
    return static_cast<std::underlying_type_t<Enum>>(e);
}

template<typename Enum, typename Integral = std::underlying_type_t<Enum>,
         typename = std::enable_if_t<std::is_enum_v<Enum> && std::is_integral_v<Integral> &&
                                     std::is_convertible_v<Integral, std::underlying_type_t<Enum>>>>
_NODISCARD _CONSTANT_VAR itoe(_In_ Integral i) noexcept
{
    return static_cast<Enum>(i);
}


// function alias templates for etoi (EnumToIntegral) and itoe (IntegralToEnum)
template<typename Enum> _CONSTANT_VAR to_integral{etoi<Enum>};
template<typename Enum, typename Integral = std::underlying_type_t<Enum>> _CONSTANT_VAR to_enum{itoe<Enum, Integral>};

/// <summary>Calculates the size of any standard fstream.</summary>
/// <param name="filename">The filename.</param>
/// <typeparam name="CharT">
///   The character type, it should be auto deduced if valid. Otherwise you'll
///   get an intellisense error.
/// </typeparam>
/// <returns>The size of the file stream.</returns>
template<typename CharT, typename = std::enable_if_t<is_char_v<CharT>>>
_NODISCARD _CONSTANT_FN filesize(_In_ const std::basic_string<CharT>& filename) noexcept
{
    using file_type = std::basic_fstream<CharT>;
    file_type file{filename, std::ios::in | std::ios::ate | std::ios::binary};
    return file.tellg();
} // end function filesize

/// <summary>Calculates the size of any standard fstream.</summary>
/// <param name="filename">The filename.</param>
/// <typeparam name="CharT">
///   The character type, it should be auto deduced if valid. Otherwise you'll
///   get an intellisense error.
/// </typeparam>
/// <returns>The size of the file stream.</returns>
/// <remarks>
///   This overload is provided as a safe-guard against unexpected lifetimes.
/// </remarks>
template<typename CharT, typename = std::enable_if_t<is_char_v<CharT>>>
_NODISCARD _CONSTANT_FN filesize(std::basic_string<CharT>&& filename) noexcept
{
    using file_type = std::basic_fstream<CharT>;
    auto bstr{std::move(filename)};
    file_type file{bstr, std::ios::in | std::ios::ate | std::ios::binary};
    return file.tellg();
} // end function filesize

// Don't depend on std::rel_ops for stuff here because it assumes each parameter
// has the same type
/// <summary>
///   Relational operator overloads, mainly for comparing an <c>enum class</c>
///   with it's <c>underlying_type</c>.
/// </summary>
namespace rel_ops {
template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_NODISCARD _CONSTANT_FN operator==(_In_ Enum a, _In_ std::underlying_type_t<Enum> b) noexcept
{
    return (etoi(a) == b);
}

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_NODISCARD _CONSTANT_FN operator==(_In_ std::underlying_type_t<Enum> a, _In_ Enum b) noexcept
{
    return (a == etoi(b));
}

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_NODISCARD _CONSTANT_FN operator!=(_In_ Enum a, _In_ std::underlying_type_t<Enum> b) noexcept
{
    return (!(etoi(a) == b));
}

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_NODISCARD _CONSTANT_FN operator!=(_In_ std::underlying_type_t<Enum> a, _In_ Enum b) noexcept
{
    return (!(a == etoi(b)));
}

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_NODISCARD _CONSTANT_FN operator<(_In_ Enum a, _In_ std::underlying_type_t<Enum> b) noexcept
{
    return (etoi(a) < b);
}

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_NODISCARD _CONSTANT_FN operator<(_In_ std::underlying_type_t<Enum> a, _In_ Enum b) noexcept
{
    return (a < etoi(b));
}

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_NODISCARD _CONSTANT_FN operator>(_In_ Enum a, _In_ std::underlying_type_t<Enum> b) noexcept
{
    return (b < etoi(a));
}

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_NODISCARD _CONSTANT_FN operator>(_In_ std::underlying_type_t<Enum> a, _In_ Enum b) noexcept
{
    return (etoi(b) < a);
}

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_NODISCARD _CONSTANT_FN operator<=(_In_ Enum a, _In_ std::underlying_type_t<Enum> b) noexcept
{
    return (!(b < etoi(a)));
}

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_NODISCARD _CONSTANT_FN operator<=(_In_ const std::underlying_type_t<Enum> a, _In_ const Enum b) noexcept
{
    return (!(etoi(b) < a));
}

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_NODISCARD _CONSTANT_FN operator>=(_In_ std::underlying_type_t<Enum> a, _In_ Enum b) noexcept
{
    return (!(a < etoi(b)));
}

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_NODISCARD _CONSTANT_FN operator>=(_In_ Enum a, _In_ std::underlying_type_t<Enum> b) noexcept
{
    return (!(etoi(a) < b));
}
} // namespace rel_ops

namespace bitwise_ops {
template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_NODISCARD _CONSTANT_FN operator|(_In_ Enum a, _In_ Enum b) noexcept
{
    return (itoe<Enum>(etoi(a) | etoi(b)));
}

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_NODISCARD _CONSTANT_FN operator&(_In_ Enum a, _In_ Enum b) noexcept
{
    return (itoe<Enum>(etoi(a) & etoi(b)));
}

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_CONSTANT_FN& operator|=(_Inout_ Enum& a, _In_ Enum b) noexcept
{
    return (a = a | b);
}

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_CONSTANT_FN& operator&=(_Inout_ Enum& a, _In_ Enum b) noexcept
{
    return (a = a & b);
}
} // namespace bitwise_ops

namespace arith_ops {
template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_NODISCARD _CONSTANT_FN operator-(_In_ Enum a, _In_ std::underlying_type_t<Enum> b) noexcept
{
    return ra::itoe<Enum>(ra::etoi(a) - b);
}

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_CONSTANT_FN& operator-=(_Inout_ Enum& a, _In_ std::underlying_type_t<Enum> b) noexcept
{
    return (a = a - b);
}
} // namespace arith_ops

} // namespace ra

#endif // !RA_UTILITY_H
