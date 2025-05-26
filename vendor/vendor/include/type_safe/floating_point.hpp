// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_FLOATING_POINT_HPP_INCLUDED
#define TYPE_SAFE_FLOATING_POINT_HPP_INCLUDED

#if defined(TYPE_SAFE_IMPORT_STD_MODULE)
import std;
#else
#include <functional>
#include <iosfwd>
#include <type_traits>
#endif

#include <type_safe/detail/force_inline.hpp>

namespace type_safe
{
template <typename FloatT>
class floating_point;

/// \exclude
namespace detail
{
    template <typename From, typename To>
    struct is_safe_floating_point_conversion
    : std::integral_constant<bool, std::is_floating_point<From>::value
                                       && std::is_floating_point<To>::value
                                       && sizeof(From) <= sizeof(To)>
    {};

    template <typename From, typename To>
    using enable_safe_floating_point_conversion =
        typename std::enable_if<is_safe_floating_point_conversion<From, To>::value>::type;

    template <typename From, typename To>
    using fallback_safe_floating_point_conversion =
        typename std::enable_if<!is_safe_floating_point_conversion<From, To>::value>::type;

    template <typename A, typename B>
    struct is_safe_floating_point_comparison
    : std::integral_constant<bool, is_safe_floating_point_conversion<A, B>::value
                                       || is_safe_floating_point_conversion<B, A>::value>
    {};

    template <typename A, typename B>
    using enable_safe_floating_point_comparison =
        typename std::enable_if<is_safe_floating_point_comparison<A, B>::value>::type;

    template <typename A, typename B>
    using fallback_safe_floating_point_comparison =
        typename std::enable_if<!is_safe_floating_point_comparison<A, B>::value>::type;

    template <typename A, typename B>
    struct is_safe_floating_point_operation
    : std::integral_constant<bool,
                             std::is_floating_point<A>::value && std::is_floating_point<B>::value>
    {};

    template <typename A, typename B>
    using floating_point_result_t = floating_point<typename std::enable_if<
        is_safe_floating_point_operation<A, B>::value,
        typename std::conditional<sizeof(A) < sizeof(B), B, A>::type>::type>;
    template <typename A, typename B>
    using fallback_floating_point_result =
        typename std::enable_if<!is_safe_floating_point_operation<A, B>::value>::type;
} // namespace detail

/// A type safe floating point class.
///
/// It is a tiny, no overhead wrapper over a standard floating point type.
/// It behaves exactly like the built-in types except it does not allow narrowing conversions.
///
/// \requires `FloatT` must be a floating point type.
/// \notes It intentionally does not provide equality or increment/decrement operators.
/// \module types
template <typename FloatT>
class floating_point
{
    static_assert(std::is_floating_point<FloatT>::value, "must be a floating point type");

public:
    using floating_point_type = FloatT;

    //=== constructors ===//
#if TYPE_SAFE_DELETE_FUNCTIONS
    /// \exclude
    floating_point() = delete;
#endif

    /// \effects Initializes the floating point with the given value.
    /// \notes These functions do not participate in overload resolution,
    /// if `T` is not a floating point type safely convertible to this type.
    /// \group constructor
    /// \param 1
    /// \exclude
    template <typename T,
              typename = detail::enable_safe_floating_point_conversion<T, floating_point_type>>
    TYPE_SAFE_FORCE_INLINE constexpr floating_point(const T& val) noexcept : value_(val)
    {}

    /// \group constructor
    /// \param 1
    /// \exclude
    template <typename T,
              typename = detail::enable_safe_floating_point_conversion<T, floating_point_type>>
    TYPE_SAFE_FORCE_INLINE constexpr floating_point(const floating_point<T>& val) noexcept
    : value_(static_cast<T>(val))
    {}

#if TYPE_SAFE_DELETE_FUNCTIONS
    /// \exclude
    template <typename T,
              typename = detail::fallback_safe_floating_point_conversion<T, floating_point_type>>
    constexpr floating_point(T) = delete;
    /// \exclude
    template <typename T,
              typename = detail::fallback_safe_floating_point_conversion<T, floating_point_type>>
    constexpr floating_point(const floating_point<T>&) = delete;
#endif

    //=== assignment ===//
    /// \effects Assigns the floating point the given value.
    /// \notes These functions do not participate in overload resolution,
    /// if `T` is not a floating point type safely convertible to this type.
    /// \group assignment
    /// \param 1
    /// \exclude
    template <typename T,
              typename = detail::enable_safe_floating_point_conversion<T, floating_point_type>>
    TYPE_SAFE_FORCE_INLINE floating_point& operator=(const T& val) noexcept
    {
        value_ = val;
        return *this;
    }

    /// \group assignment
    /// \param 1
    /// \exclude
    template <typename T,
              typename = detail::enable_safe_floating_point_conversion<T, floating_point_type>>
    TYPE_SAFE_FORCE_INLINE floating_point& operator=(const floating_point<T>& val) noexcept
    {
        value_ = static_cast<T>(val);
        return *this;
    }

#if TYPE_SAFE_DELETE_FUNCTIONS
    /// \exclude
    template <typename T,
              typename = detail::fallback_safe_floating_point_conversion<T, floating_point_type>>
    floating_point& operator=(T) = delete;
    /// \exclude
    template <typename T,
              typename = detail::fallback_safe_floating_point_conversion<T, floating_point_type>>
    floating_point& operator=(const floating_point<T>&) = delete;
#endif

    //=== conversion back ===//
    /// \returns The stored value as the native floating point type.
    /// \group conversion
    TYPE_SAFE_FORCE_INLINE explicit constexpr operator floating_point_type() const noexcept
    {
        return value_;
    }

    /// \group conversion
    TYPE_SAFE_FORCE_INLINE constexpr floating_point_type get() const noexcept
    {
        return value_;
    }

    //=== unary operators ===//
    /// \returns The value unchanged.
    TYPE_SAFE_FORCE_INLINE constexpr floating_point operator+() const noexcept
    {
        return *this;
    }

    /// \returns The negative value.
    TYPE_SAFE_FORCE_INLINE constexpr floating_point operator-() const noexcept
    {
        return -value_;
    }

//=== compound assignment ====//
/// \entity TYPE_SAFE_DETAIL_MAKE_OP
/// \exclude
#define TYPE_SAFE_DETAIL_MAKE_OP(Op)                                                               \
    /** \group compound_assign                                                                     \
     * \module types                                                                               \
     * \param 1                                                                                    \
     * \exclude                                                                                    \
     */                                                                                            \
    template <typename T,                                                                          \
              typename = detail::enable_safe_floating_point_conversion<T, floating_point_type>>    \
    TYPE_SAFE_FORCE_INLINE floating_point& operator Op(const T& other) noexcept                    \
    {                                                                                              \
        return *this Op floating_point<T>(other);                                                  \
    }                                                                                              \
    /** \exclude */                                                                                \
    template <typename T,                                                                          \
              typename = detail::fallback_safe_floating_point_conversion<T, floating_point_type>>  \
    floating_point& operator Op(floating_point<T>) = delete;                                       \
    /** \exclude */                                                                                \
    template <typename T,                                                                          \
              typename = detail::fallback_safe_floating_point_conversion<T, floating_point_type>>  \
    floating_point& operator Op(T) = delete;

    /// \effects Same as the operation on the floating point type.
    /// \notes These functions do not participate in overload resolution,
    /// if `T` is not a floating point type safely convertible to this type.
    /// \group compound_assign Compound assignment
    /// \module types
    /// \param 1
    /// \exclude
    template <typename T,
              typename = detail::enable_safe_floating_point_conversion<T, floating_point_type>>
    TYPE_SAFE_FORCE_INLINE floating_point& operator+=(const floating_point<T>& other) noexcept
    {
        value_ += static_cast<T>(other);
        return *this;
    }
    TYPE_SAFE_DETAIL_MAKE_OP(+=)

    /// \group compound_assign
    /// \param 1
    /// \exclude
    template <typename T,
              typename = detail::enable_safe_floating_point_conversion<T, floating_point_type>>
    TYPE_SAFE_FORCE_INLINE floating_point& operator-=(const floating_point<T>& other) noexcept
    {
        value_ -= static_cast<T>(other);
        return *this;
    }
    TYPE_SAFE_DETAIL_MAKE_OP(-=)

    /// \group compound_assign
    /// \param 1
    /// \exclude
    template <typename T,
              typename = detail::enable_safe_floating_point_conversion<T, floating_point_type>>
    TYPE_SAFE_FORCE_INLINE floating_point& operator*=(const floating_point<T>& other) noexcept
    {
        value_ *= static_cast<T>(other);
        return *this;
    }
    TYPE_SAFE_DETAIL_MAKE_OP(*=)

    /// \group compound_assign
    /// \param 1
    /// \exclude
    template <typename T,
              typename = detail::enable_safe_floating_point_conversion<T, floating_point_type>>
    TYPE_SAFE_FORCE_INLINE floating_point& operator/=(const floating_point<T>& other) noexcept
    {
        value_ /= static_cast<T>(other);
        return *this;
    }
    TYPE_SAFE_DETAIL_MAKE_OP(/=)

#undef TYPE_SAFE_DETAIL_MAKE_OP

private:
    floating_point_type value_;
};

//=== comparison ===//
/// \exclude
#define TYPE_SAFE_DETAIL_MAKE_OP(Op)                                                               \
    /** \group float_comp                                                                          \
     * \param 2                                                                                    \
     * \exclude */                                                                                 \
    template <typename A, typename B,                                                              \
              typename = detail::enable_safe_floating_point_conversion<A, B>>                      \
    TYPE_SAFE_FORCE_INLINE constexpr bool operator Op(const A& a, const floating_point<B>& b)      \
    {                                                                                              \
        return floating_point<A>(a) Op b;                                                          \
    }                                                                                              \
    /** \group float_comp                                                                          \
     * \param 2                                                                                    \
     * \exclude  */                                                                                \
    template <typename A, typename B,                                                              \
              typename = detail::enable_safe_floating_point_comparison<A, B>>                      \
    TYPE_SAFE_FORCE_INLINE constexpr bool operator Op(const floating_point<A>& a, const B& b)      \
    {                                                                                              \
        return a Op floating_point<B>(b);                                                          \
    }                                                                                              \
    /** \exclude */                                                                                \
    template <typename A, typename B,                                                              \
              typename = detail::fallback_safe_floating_point_comparison<A, B>>                    \
    constexpr bool operator Op(floating_point<A>, floating_point<B>) = delete;                     \
    /** \exclude */                                                                                \
    template <typename A, typename B,                                                              \
              typename = detail::fallback_safe_floating_point_comparison<A, B>>                    \
    constexpr bool operator Op(A, floating_point<B>) = delete;                                     \
    /** \exclude */                                                                                \
    template <typename A, typename B,                                                              \
              typename = detail::fallback_safe_floating_point_comparison<A, B>>                    \
    constexpr bool operator Op(floating_point<A>, B) = delete;

/// \returns The result of the comparison of the stored floating point value in the
/// [ts::floating_point](). \notes These functions do not participate in overload resolution unless
/// `A` and `B` are both floating point types. \group float_comp Comparison operators \module types
/// \param 2
/// \exclude
template <typename A, typename B, typename = detail::enable_safe_floating_point_comparison<A, B>>
TYPE_SAFE_FORCE_INLINE constexpr bool operator<(const floating_point<A>& a,
                                                const floating_point<B>& b) noexcept
{
    return static_cast<A>(a) < static_cast<B>(b);
}
TYPE_SAFE_DETAIL_MAKE_OP(<)

/// \group float_comp
/// \param 2
/// \exclude
template <typename A, typename B, typename = detail::enable_safe_floating_point_comparison<A, B>>
TYPE_SAFE_FORCE_INLINE constexpr bool operator<=(const floating_point<A>& a,
                                                 const floating_point<B>& b) noexcept
{
    return static_cast<A>(a) <= static_cast<B>(b);
}
TYPE_SAFE_DETAIL_MAKE_OP(<=)

/// \group float_comp
/// \param 2
/// \exclude
template <typename A, typename B, typename = detail::enable_safe_floating_point_comparison<A, B>>
TYPE_SAFE_FORCE_INLINE constexpr bool operator>(const floating_point<A>& a,
                                                const floating_point<B>& b) noexcept
{
    return static_cast<A>(a) > static_cast<B>(b);
}
TYPE_SAFE_DETAIL_MAKE_OP(>)

/// \group float_comp
/// \param 2
/// \exclude
template <typename A, typename B, typename = detail::enable_safe_floating_point_comparison<A, B>>
TYPE_SAFE_FORCE_INLINE constexpr bool operator>=(const floating_point<A>& a,
                                                 const floating_point<B>& b) noexcept
{
    return static_cast<A>(a) >= static_cast<B>(b);
}
TYPE_SAFE_DETAIL_MAKE_OP(>=)

#undef TYPE_SAFE_DETAIL_MAKE_OP

//=== binary operations ===//
/// \entity TYPE_SAFE_DETAIL_MAKE_OP
/// \exclude
#define TYPE_SAFE_DETAIL_MAKE_OP(Op)                                                               \
    /** \group float_binary_op */                                                                  \
    template <typename A, typename B>                                                              \
    TYPE_SAFE_FORCE_INLINE constexpr auto operator Op(const A&                 a,                  \
                                                      const floating_point<B>& b) noexcept         \
        ->detail::floating_point_result_t<A, B>                                                    \
    {                                                                                              \
        return floating_point<A>(a) Op b;                                                          \
    }                                                                                              \
    /** \group float_binary_op */                                                                  \
    template <typename A, typename B>                                                              \
    TYPE_SAFE_FORCE_INLINE constexpr auto operator Op(const floating_point<A>& a,                  \
                                                      const B&                 b) noexcept                         \
        ->detail::floating_point_result_t<A, B>                                                    \
    {                                                                                              \
        return a Op floating_point<B>(b);                                                          \
    }                                                                                              \
    /** \exclude */                                                                                \
    template <typename A, typename B, typename = detail::fallback_floating_point_result<A, B>>     \
    constexpr int operator Op(floating_point<A>, floating_point<B>) noexcept = delete;             \
    /** \exclude */                                                                                \
    template <typename A, typename B, typename = detail::fallback_floating_point_result<A, B>>     \
    constexpr int operator Op(A, floating_point<B>) noexcept = delete;                             \
    /** \exclude */                                                                                \
    template <typename A, typename B, typename = detail::fallback_floating_point_result<A, B>>     \
    constexpr int operator Op(floating_point<A>, B) noexcept = delete;

/// \returns The result of the binary operation of the stored floating point value in the
/// [ts::floating_point](). The type is a [ts::floating_point]() of the bigger floating point type.
/// \notes These functions do not participate in overload resolution,
/// unless `A` and `B` are both floating point types.
/// \module types
/// \group float_binary_op Binary operations
template <typename A, typename B>
TYPE_SAFE_FORCE_INLINE constexpr auto operator+(const floating_point<A>& a,
                                                const floating_point<B>& b) noexcept
    -> detail::floating_point_result_t<A, B>
{
    return static_cast<A>(a) + static_cast<B>(b);
}
TYPE_SAFE_DETAIL_MAKE_OP(+)

/// \group float_binary_op
template <typename A, typename B>
TYPE_SAFE_FORCE_INLINE constexpr auto operator-(const floating_point<A>& a,
                                                const floating_point<B>& b) noexcept
    -> detail::floating_point_result_t<A, B>
{
    return static_cast<A>(a) - static_cast<B>(b);
}
TYPE_SAFE_DETAIL_MAKE_OP(-)

/// \group float_binary_op
template <typename A, typename B>
TYPE_SAFE_FORCE_INLINE constexpr auto operator*(const floating_point<A>& a,
                                                const floating_point<B>& b) noexcept
    -> detail::floating_point_result_t<A, B>
{
    return static_cast<A>(a) * static_cast<B>(b);
}
TYPE_SAFE_DETAIL_MAKE_OP(*)

/// \group float_binary_op
template <typename A, typename B>
TYPE_SAFE_FORCE_INLINE constexpr auto operator/(const floating_point<A>& a,
                                                const floating_point<B>& b) noexcept
    -> detail::floating_point_result_t<A, B>
{
    return static_cast<A>(a) / static_cast<B>(b);
}
TYPE_SAFE_DETAIL_MAKE_OP(/)

#undef TYPE_SAFE_DETAIL_MAKE_OP

//=== input/output ===/
/// \effects Reads a float from the [std::istream]() and assigns it to the given
/// [ts::floating_point](). \module types \output_section Input/output
template <typename Char, class CharTraits, typename FloatT>
std::basic_istream<Char, CharTraits>& operator>>(std::basic_istream<Char, CharTraits>& in,
                                                 floating_point<FloatT>&               f)
{
    FloatT val;
    in >> val;
    f = val;
    return in;
}

/// \effects Converts the given [ts::floating_point]() to the underlying floating point and writes
/// it to the [std::ostream](). \module types
template <typename Char, class CharTraits, typename FloatT>
std::basic_ostream<Char, CharTraits>& operator<<(std::basic_ostream<Char, CharTraits>& out,
                                                 const floating_point<FloatT>&         f)
{
    return out << static_cast<FloatT>(f);
}
} // namespace type_safe

namespace std
{
/// Hash specialization for [ts::floating_point].
/// \module types
template <typename FloatT>
struct hash<type_safe::floating_point<FloatT>>
{
    std::size_t operator()(const type_safe::floating_point<FloatT>& f) const noexcept
    {
        return std::hash<FloatT>()(static_cast<FloatT>(f));
    }
};
} // namespace std

#endif // TYPE_SAFE_FLOATING_POINT_HPP_INCLUDED
