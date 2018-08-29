#ifndef RA_TYPE_TRAITS_H
#define RA_TYPE_TRAITS_H
#pragma once

#include <utility>

#if _HAS_CXX17
#ifndef _CONSTANT_VAR
#define _CONSTANT_VAR inline constexpr auto
#endif // !_CONSTANT_VAR
#else
#ifndef _CONSTANT_VAR
#define _CONSTANT_VAR constexpr auto
#endif // !_CONSTANT_VAR
#endif // _HAS_CXX17

#ifndef _CONSTANT_FN
#define _CONSTANT_FN  _CONSTANT_VAR
#endif // !_CONSTANT_FN

namespace ra {
namespace detail {

/// <summary>
///   A check to tell whether a type is a known character type. If this
///   function was reached, <typeparamref name="CharT" /> is a known character
///   type.
/// </summary>
/// <typeparam name="CharT">A type to be evaluated</typeparam>
template<typename CharT>
struct _NODISCARD is_char :
    std::bool_constant<(std::_Is_character<CharT>::value || std::is_same_v<CharT, wchar_t>)>
{
};

/// <summary>
///   This should only be used to compare the sizes of data types and not objects.
/// </summary>
template<typename T, typename U> struct _NODISCARD is_same_size : std::bool_constant<sizeof(T) == sizeof(U)> {};

} // namespace detail

template<typename CharacterType> _CONSTANT_VAR
is_char_v{ detail::is_char<CharacterType>::value };

template<typename ValueType, typename TestType> _CONSTANT_VAR
is_same_size_v{ detail::is_same_size<ValueType, TestType>::value };

namespace detail {
template<typename EqualityComparable, class = std::void_t<>>
struct _NODISCARD is_equality_comparable : std::false_type {};

template<typename EqualityComparable>
struct _NODISCARD is_equality_comparable<EqualityComparable, std::enable_if_t<std::is_convertible_v<
    decltype(std::declval<EqualityComparable&>() == std::declval<EqualityComparable&>()), bool>>> :
    std::true_type {};

template<typename EqualityComparable>
struct _NODISCARD is_nothrow_equality_comparable :
    std::bool_constant<noexcept(is_equality_comparable<EqualityComparable>::value)>
{
};

template<typename LessThanComparable, class = std::void_t<>>
struct _NODISCARD is_lessthan_comparable : std::false_type {};

template<typename LessThanComparable>
struct _NODISCARD is_lessthan_comparable<LessThanComparable, std::enable_if_t<std::is_convertible_v<
    decltype(std::declval<LessThanComparable&>() == std::declval<LessThanComparable&>()), bool>>> :
    std::true_type {};

template<typename LessThanComparable>
struct _NODISCARD is_nothrow_lessthan_comparable :
    std::bool_constant<noexcept(is_lessthan_comparable<LessThanComparable>::value)>
{
};

template<typename Comparable>
struct _NODISCARD is_comparable :
    std::bool_constant<(is_equality_comparable<Comparable>::value && is_lessthan_comparable<Comparable>::value)>
{
};

template<typename Comparable>
struct _NODISCARD is_nothrow_comparable : std::bool_constant<noexcept(is_comparable<Comparable>::value)> {};
} // namespace detail

template<typename EqualityComparable> _CONSTANT_VAR
is_equality_comparable_v{ detail::is_equality_comparable<EqualityComparable>::value };

template<typename EqualityComparable> _CONSTANT_VAR
is_nothrow_equality_comparable_v{ detail::is_nothrow_equality_comparable<EqualityComparable>::value };

template<typename LessThanComparable> _CONSTANT_VAR
is_lessthan_comparable_v{ detail::is_lessthan_comparable<LessThanComparable>::value };

template<typename LessThanComparable> _CONSTANT_VAR
is_nothrow_lessthan_comparable_v{ detail::is_nothrow_lessthan_comparable<LessThanComparable>::value };

template<typename Comparable> _CONSTANT_VAR
is_comparable_v{ detail::is_comparable<Comparable>::value };

template<typename Comparable> _CONSTANT_VAR
is_nothrow_comparable_v{ detail::is_nothrow_comparable<Comparable>::value };

} // namespace ra

#endif // !RA_TYPE_TRAITS_H
