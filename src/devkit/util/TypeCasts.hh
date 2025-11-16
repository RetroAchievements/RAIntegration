#ifndef RA_TYPE_CASTS_HH
#define RA_TYPE_CASTS_HH
#pragma once

#include "TypeTraits.hh"

/*
   For most of these functions you won't have to specify the template argument
   as it will be auto-deduced unless stated otherwise.
*/
namespace ra {

/* to_unsigned - convert a signed value to an unsigned value */
template<typename SignedType, typename = std::enable_if_t<std::is_signed_v<SignedType>>>
_NODISCARD _CONSTANT_FN to_unsigned(_In_ SignedType st) noexcept
{
    return gsl::narrow_cast<std::make_unsigned_t<SignedType>>(st);
}

/* to_signed - convert an unsigned value to a signed value */
template<typename UnsignedType, typename = std::enable_if_t<std::is_unsigned_v<UnsignedType>>>
_NODISCARD _CONSTANT_FN to_signed(_In_ UnsignedType st) noexcept
{
    return gsl::narrow_cast<std::make_signed_t<UnsignedType>>(st);
}

/* etoi - convert an enum to an int */
template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_NODISCARD _CONSTANT_VAR etoi(_In_ Enum e) noexcept
{
    return static_cast<std::underlying_type_t<Enum>>(e);
}

/* itoe - convert an int to an enum */
template<typename Enum, typename Integral = std::underlying_type_t<Enum>,
         typename = std::enable_if_t<std::is_enum_v<Enum> && std::is_integral_v<Integral> &&
                                     std::is_convertible_v<Integral, std::underlying_type_t<Enum>>>>
_NODISCARD _CONSTANT_VAR itoe(_In_ Integral i) noexcept
{
    return static_cast<Enum>(i);
}

/* ftol - convert a float to a long */
template<typename FloatingPoint, class = std::enable_if_t<std::is_floating_point_v<FloatingPoint>>>
_NODISCARD inline auto ftol(_In_ FloatingPoint fp) noexcept
{
    return std::lround(fp);
}

/* ftoi - convert a float to an int */
template<typename FloatingPoint, class = std::enable_if_t<std::is_floating_point_v<FloatingPoint>>>
_NODISCARD inline int ftoi(_In_ FloatingPoint fp) noexcept
{
    return std::lround(fp);
}

/* ftoll - convert a float to a long long */
template<typename FloatingPoint, class = std::enable_if_t<std::is_floating_point_v<FloatingPoint>>>
_NODISCARD inline auto ftoll(_In_ FloatingPoint fp) noexcept
{
    return std::llround(fp);
}

/* ftoul - convert a float to an unsigned long */
template<typename FloatingPoint, class = std::enable_if_t<std::is_floating_point_v<FloatingPoint>>>
_NODISCARD inline auto ftoul(_In_ FloatingPoint fp) noexcept
{
    return to_unsigned(std::lround(fp));
}

/* ftoull - convert a float to an unsigned long long */
template<typename FloatingPoint, class = std::enable_if_t<std::is_floating_point_v<FloatingPoint>>>
_NODISCARD inline auto ftoull(_In_ FloatingPoint fp) noexcept
{
    return to_unsigned(std::llround(fp));
}

} // namespace ra

#endif // !RA_TYPE_CASTS_H
