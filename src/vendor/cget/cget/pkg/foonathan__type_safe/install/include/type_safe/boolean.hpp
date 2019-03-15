// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_BOOLEAN_HPP_INCLUDED
#define TYPE_SAFE_BOOLEAN_HPP_INCLUDED

#include <functional>
#include <iosfwd>
#include <type_traits>
#include <utility>

#include <type_safe/detail/force_inline.hpp>

namespace type_safe
{
class boolean;

/// \exclude
namespace detail
{
    template <typename T>
    struct is_boolean : std::false_type
    {};

    template <>
    struct is_boolean<bool> : std::true_type
    {};

    template <>
    struct is_boolean<boolean> : std::true_type
    {};

    template <typename T>
    using enable_boolean = typename std::enable_if<is_boolean<T>::value>::type;
} // namespace detail

/// A type safe boolean class.
///
/// It is a tiny, no overhead wrapper over `bool`.
/// It can only be constructed from `bool` values
/// and does not implicitly convert to integral types.
/// \module types
class boolean
{
public:
    boolean() = delete;

    /// \effects Creates a boolean from the given `value`.
    /// \notes This function does not participate in overload resolution if `T` is not a boolean
    /// type. \param 1 \exclude
    template <typename T, typename = detail::enable_boolean<T>>
    TYPE_SAFE_FORCE_INLINE constexpr boolean(T value) noexcept : value_(value)
    {}

    /// \effects Assigns the given `value` to the boolean.
    /// \notes This function does not participate in overload resolution if `T` is not a boolean
    /// type. \param 1 \exclude
    template <typename T, typename = detail::enable_boolean<T>>
    TYPE_SAFE_FORCE_INLINE boolean& operator=(T value) noexcept
    {
        value_ = value;
        return *this;
    }

    /// \returns The stored `bool` value.
    TYPE_SAFE_FORCE_INLINE explicit constexpr operator bool() const noexcept
    {
        return value_;
    }

    /// \returns The same as `!static_cast<bool>(*this)`.
    TYPE_SAFE_FORCE_INLINE constexpr boolean operator!() const noexcept
    {
        return boolean(!value_);
    }

private:
    bool value_;
};

//=== comparison ===//
/// [ts::boolean]() equality comparison.
/// \returns `true` if (1) both [ts::boolean]() objects have the same value,
/// (2)/(3) the boolean has the same value as the given value,
/// `false` otherwise.
/// \notes (2)/(3) do not participate in overload resolution if `T` is not a boolean type.
/// \group boolean_comp_equal
/// \module types
TYPE_SAFE_FORCE_INLINE constexpr bool operator==(const boolean& a, const boolean& b) noexcept
{
    return static_cast<bool>(a) == static_cast<bool>(b);
}

/// \group boolean_comp_equal
/// \param 1
/// \exclude
template <typename T, typename = detail::enable_boolean<T>>
TYPE_SAFE_FORCE_INLINE constexpr bool operator==(const boolean& a, T b) noexcept
{
    return static_cast<bool>(a) == static_cast<bool>(b);
}

/// \group boolean_comp_equal
/// \param 1
/// \exclude
template <typename T, typename = detail::enable_boolean<T>>
TYPE_SAFE_FORCE_INLINE constexpr bool operator==(T a, const boolean& b) noexcept
{
    return static_cast<bool>(a) == static_cast<bool>(b);
}

/// [ts::boolean]() in-equality comparison.
/// \returns `false` if (1) both [ts::boolean]() objects have the same value,
/// (2)/(3) the boolean has the same value as the given value,
/// `true` otherwise.
/// \notes (2)/(3) do not participate in overload resolution if `T` is not a boolean type.
/// \group boolean_comp_unequal
/// \module types
TYPE_SAFE_FORCE_INLINE constexpr bool operator!=(const boolean& a, const boolean& b) noexcept
{
    return static_cast<bool>(a) != static_cast<bool>(b);
}

/// \group boolean_comp_unequal
/// \param 1
/// \exclude
template <typename T, typename = detail::enable_boolean<T>>
TYPE_SAFE_FORCE_INLINE constexpr bool operator!=(const boolean& a, T b) noexcept
{
    return static_cast<bool>(a) != static_cast<bool>(b);
}

/// \group boolean_comp_unequal
/// \param 1
/// \exclude
template <typename T, typename = detail::enable_boolean<T>>
TYPE_SAFE_FORCE_INLINE constexpr bool operator!=(T a, const boolean& b) noexcept
{
    return static_cast<bool>(a) != static_cast<bool>(b);
}

//=== input/output ===/
/// [ts::boolean]() input operator.
/// \effects Reads a `bool` from the [std::istream]() and assigns it to the given [ts::boolean]().
/// \output_section Input/output
/// \module types
template <typename Char, class CharTraits>
std::basic_istream<Char, CharTraits>& operator>>(std::basic_istream<Char, CharTraits>& in,
                                                 boolean&                              b)
{
    bool val;
    in >> val;
    b = val;
    return in;
}

/// [ts::boolean]() output operator.
/// \effects Converts the given [ts::boolean]() to `bool` and writes it to the [std::ostream]().
/// \module types
template <typename Char, class CharTraits>
std::basic_ostream<Char, CharTraits>& operator<<(std::basic_ostream<Char, CharTraits>& out,
                                                 const boolean&                        b)
{
    return out << static_cast<bool>(b);
}

//=== comparison functors ===//
/// \exclude
#define TYPE_SAFE_DETAIL_MAKE_PREDICATE(Name, Op)                                                  \
    struct Name                                                                                    \
    {                                                                                              \
        using is_transparent = int;                                                                \
                                                                                                   \
        template <typename T1, typename T2>                                                        \
        constexpr bool operator()(T1&& a, T2&& b) const                                            \
            noexcept(noexcept(std::forward<T1>(a) Op std::forward<T2>(b)))                         \
        {                                                                                          \
            return static_cast<bool>(std::forward<T1>(a) Op std::forward<T2>(b));                  \
        }                                                                                          \
    };

/// Comparison functors similar to the `std::` version,
/// but explicitly cast the result of the comparison to `bool`.
///
/// This allows using types where the comparison operator returns [ts::boolean](),
/// as it can not be implicitly converted to `bool`
/// so, for example, [std::less]() can not be used.
/// \notes These comparison functors are always transparent,
/// i.e. can be used with two different types.
/// \group comparison_functors Comparison function objects
/// \module types
TYPE_SAFE_DETAIL_MAKE_PREDICATE(equal_to, ==)
/// \group comparison_functors
TYPE_SAFE_DETAIL_MAKE_PREDICATE(not_equal_to, !=)
/// \group comparison_functors
TYPE_SAFE_DETAIL_MAKE_PREDICATE(less, <)
/// \group comparison_functors
TYPE_SAFE_DETAIL_MAKE_PREDICATE(less_equal, <=)
/// \group comparison_functors
TYPE_SAFE_DETAIL_MAKE_PREDICATE(greater, >)
/// \group comparison_functors
TYPE_SAFE_DETAIL_MAKE_PREDICATE(greater_equal, >=)

#undef TYPE_SAFE_DETAIL_MAKE_PREDICATE
} // namespace type_safe

namespace std
{
/// Hash specialization for [ts::boolean]().
/// \module types
template <>
struct hash<type_safe::boolean>
{
    std::size_t operator()(type_safe::boolean b) const noexcept
    {
        return std::hash<bool>()(static_cast<bool>(b));
    }
};
} // namespace std

#endif // TYPE_SAFE_BOOLEAN_HPP_INCLUDED
