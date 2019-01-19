#ifndef RA_MATH_H
#define RA_MATH_H
#pragma once

#include "ra_utility.h"

namespace ra {

/// <summary>Casts <paramref name="a" /> into a <typeparamref name="FloatingPoint" />.</summary>
/// <typeparam name="Arithmetic">A valid arithmetic type.</typeparam>
/// <param name="a">The number to be converted.</param>
/// <returns>
///   The return type depends on the size of <typeparamref name="Arithmetic" />. If it's 8 bytes, it will
///   <c>double</c>, if it's anything less it will be float. <c>long double</c> is not considered as it is currently
///   the same as <c>double</c> size-wise.
/// </returns>
template<typename Arithmetic, class = std::enable_if_t<std::is_arithmetic_v<Arithmetic>>>
_NODISCARD _CONSTANT_FN to_floating(_In_ Arithmetic a) noexcept
{
    if constexpr (is_same_size_v<Arithmetic, double>) return static_cast<double>(a);
    if constexpr (has_smaller_size_than_v<Arithmetic, double>) return static_cast<float>(a);
}

template<typename FloatingPoint, class = std::enable_if_t<std::is_floating_point_v<FloatingPoint>>>
_NODISCARD inline auto ftol(_In_ FloatingPoint fp) noexcept
{
    return std::lround(fp);
}

template<typename FloatingPoint, class = std::enable_if_t<std::is_floating_point_v<FloatingPoint>>>
_NODISCARD inline int ftoi(_In_ FloatingPoint fp) noexcept
{
    return std::lround(fp);
}

template<typename FloatingPoint, class = std::enable_if_t<std::is_floating_point_v<FloatingPoint>>>
_NODISCARD inline auto ftoll(_In_ FloatingPoint fp) noexcept
{
    return std::llround(fp);
}

template<typename FloatingPoint, class = std::enable_if_t<std::is_floating_point_v<FloatingPoint>>>
_NODISCARD inline auto ftoul(_In_ FloatingPoint fp) noexcept
{
    return to_unsigned(std::lround(fp));
}

template<typename FloatingPoint, class = std::enable_if_t<std::is_floating_point_v<FloatingPoint>>>
_NODISCARD inline auto ftoull(_In_ FloatingPoint fp) noexcept
{
    return to_unsigned(std::llround(fp));
}

template<typename Arithmetic, class = std::enable_if_t<std::is_arithmetic_v<Arithmetic>>>
_NODISCARD _CONSTANT_FN sqr(_In_ Arithmetic a) noexcept
{
    return (a*a);
}
} /* namespace ra */

#endif /* !RA_MATH_H */
