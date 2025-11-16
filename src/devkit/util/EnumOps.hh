#ifndef RA_ENUM_OPS_H
#define RA_ENUM_OPS_H
#pragma once

#include "util\Compat.hh"
#include "util\TypeCasts.hh"
#include "util\TypeTraits.hh"

namespace ra {

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

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_NODISCARD _CONSTANT_FN operator~(_In_ Enum a) noexcept
{
    return (itoe<Enum>(~etoi(a)));
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

#endif // !RA_ENUM_OPS
