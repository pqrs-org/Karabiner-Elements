// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_INTEGER_HPP_INCLUDED
#define TYPE_SAFE_INTEGER_HPP_INCLUDED

#include <functional>
#include <iosfwd>
#include <limits>
#include <type_traits>

#include <type_safe/arithmetic_policy.hpp>
#include <type_safe/detail/assert.hpp>
#include <type_safe/detail/force_inline.hpp>

namespace type_safe
{
template <typename IntegerT, class Policy = arithmetic_policy_default>
class integer;

/// \exclude
namespace detail
{
    template <typename T>
    struct is_integer
    : std::integral_constant<bool, std::is_integral<T>::value && !std::is_same<T, bool>::value
                                       && !std::is_same<T, char>::value>
    {};

    template <typename From, typename To>
    struct is_safe_integer_conversion
    : std::integral_constant<bool, detail::is_integer<From>::value && detail::is_integer<To>::value
                                       && sizeof(From) <= sizeof(To)
                                       && std::is_signed<From>::value == std::is_signed<To>::value>
    {};

    template <typename From, typename To>
    using enable_safe_integer_conversion =
        typename std::enable_if<is_safe_integer_conversion<From, To>::value>::type;

    template <typename From, typename To>
    using fallback_safe_integer_conversion =
        typename std::enable_if<!is_safe_integer_conversion<From, To>::value>::type;

    template <typename A, typename B>
    struct is_safe_integer_comparison
    : std::integral_constant<bool, is_safe_integer_conversion<A, B>::value
                                       || is_safe_integer_conversion<B, A>::value>
    {};

    template <typename A, typename B>
    using enable_safe_integer_comparison =
        typename std::enable_if<is_safe_integer_comparison<A, B>::value>::type;

    template <typename A, typename B>
    using fallback_safe_integer_comparison =
        typename std::enable_if<!is_safe_integer_comparison<A, B>::value>::type;

    template <typename A, typename B>
    struct is_safe_integer_operation
    : std::integral_constant<bool, detail::is_integer<A>::value && detail::is_integer<B>::value
                                       && std::is_signed<A>::value == std::is_signed<B>::value>
    {};

    template <typename A, typename B>
    struct integer_result_type
    : std::enable_if<is_safe_integer_operation<A, B>::value,
                     typename std::conditional<sizeof(A) < sizeof(B), B, A>::type>
    {};

    template <typename A, typename B>
    using integer_result_t = typename integer_result_type<A, B>::type;

    template <typename A, typename B>
    using fallback_integer_result =
        typename std::enable_if<!is_safe_integer_operation<A, B>::value>::type;
} // namespace detail

/// A type safe integer class.
///
/// This is a tiny, no overhead wrapper over a standard integer type.
/// It behaves exactly like the built-in types except that narrowing conversions are not allowed.
/// It also checks against `unsigned` under/overflow in debug mode
/// and marks it as undefined for the optimizer otherwise.
///
/// A conversion is considered safe if both integer types have the same signedness
/// and the size of the value being converted is less than or equal to the destination size.
///
/// \requires `IntegerT` must be an integral type except `bool` and `char` (use `signed
/// char`/`unsigned char`). \notes It intentionally does not provide the bitwise operations. \module
/// types
template <typename IntegerT, class Policy /* = arithmetic_policy_default*/>
class integer
{
    static_assert(detail::is_integer<IntegerT>::value, "must be a real integer type");

public:
    using integer_type = IntegerT;

    //=== constructors ===//
    /// \exclude
    integer() = delete;

    /// \effects Initializes it with the given value.
    /// \notes This function does not participate in overload resolution
    /// if `T` is not an integer type safely convertible to this type.
    /// \group constructor
    /// \param 1
    /// \exclude
    template <typename T, typename = detail::enable_safe_integer_conversion<T, integer_type>>
    TYPE_SAFE_FORCE_INLINE constexpr integer(const T& val) : value_(val)
    {}

    /// \group constructor
    /// \param 1
    /// \exclude
    template <typename T, typename = detail::enable_safe_integer_conversion<T, integer_type>>
    TYPE_SAFE_FORCE_INLINE constexpr integer(const integer<T, Policy>& val)
    : value_(static_cast<T>(val))
    {}

    /// \exclude
    template <typename T, typename = detail::fallback_safe_integer_conversion<T, integer_type>>
    constexpr integer(T) = delete;

    //=== assignment ===//
    /// \effects Assigns it with the given value.
    /// \notes This function does not participate in overload resolution
    /// if `T` is not an integer type safely convertible to this type.
    /// \group assignment
    /// \param 1
    /// \exclude
    template <typename T, typename = detail::enable_safe_integer_conversion<T, integer_type>>
    TYPE_SAFE_FORCE_INLINE integer& operator=(const T& val)
    {
        value_ = val;
        return *this;
    }

    /// \group assignment
    /// \param 1
    /// \exclude
    template <typename T, typename = detail::enable_safe_integer_conversion<T, integer_type>>
    TYPE_SAFE_FORCE_INLINE integer& operator=(const integer<T, Policy>& val)
    {
        value_ = static_cast<T>(val);
        return *this;
    }

    /// \exclude
    template <typename T, typename = detail::fallback_safe_integer_conversion<T, integer_type>>
    integer& operator=(T) = delete;

    //=== conversion back ===//
    /// \returns The stored value as the native integer type.
    /// \group conversion
    TYPE_SAFE_FORCE_INLINE explicit constexpr operator integer_type() const noexcept
    {
        return value_;
    }

    /// \group conversion
    TYPE_SAFE_FORCE_INLINE constexpr integer_type get() const noexcept
    {
        return value_;
    }

    //=== unary operators ===//
    /// \returns The integer type unchanged.
    TYPE_SAFE_FORCE_INLINE constexpr integer operator+() const
    {
        return *this;
    }

    /// \returns The negative integer type.
    /// \requires The integer type must not be unsigned.
    TYPE_SAFE_FORCE_INLINE constexpr integer operator-() const
    {
        static_assert(std::is_signed<integer_type>::value,
                      "cannot call unary minus on unsigned integer");
        return -value_;
    }

    /// \effects Increments the integer by one.
    /// \group increment
    TYPE_SAFE_FORCE_INLINE integer& operator++()
    {
        value_ = Policy::template do_addition(value_, integer_type(1));
        return *this;
    }

    /// \group increment
    TYPE_SAFE_FORCE_INLINE integer operator++(int)
    {
        auto res = *this;
        ++*this;
        return res;
    }

    /// \effects Decrements the integer by one.
    /// \group decrement
    TYPE_SAFE_FORCE_INLINE integer& operator--()
    {
        value_ = Policy::template do_subtraction(value_, integer_type(1));
        return *this;
    }

    /// \group decrement
    TYPE_SAFE_FORCE_INLINE integer operator--(int)
    {
        auto res = *this;
        --*this;
        return res;
    }

//=== compound assignment ====//
/// \exclude
#define TYPE_SAFE_DETAIL_MAKE_OP(Op)                                                               \
    /** \group compound_assign                                                                     \
     * \param 1                                                                                    \
     * \exclude */                                                                                 \
    template <typename T, typename = detail::enable_safe_integer_conversion<T, integer_type>>      \
    TYPE_SAFE_FORCE_INLINE integer& operator Op(const T& other)                                    \
    {                                                                                              \
        return *this Op integer<T, Policy>(other);                                                 \
    }                                                                                              \
    /** \exclude */                                                                                \
    template <typename T, typename = detail::fallback_safe_integer_conversion<T, integer_type>>    \
    integer& operator Op(integer<T, Policy>) = delete;                                             \
    /** \exclude */                                                                                \
    template <typename T, typename = detail::fallback_safe_integer_conversion<T, integer_type>>    \
    integer& operator Op(T) = delete;

    /// \effects Same as the operation on the integer type.
    /// \notes These functions do not participate in overload resolution,
    /// if `T` is not an integer type safely convertible to this type.
    /// \group compound_assign Compound assignment
    /// \param 1
    /// \exclude
    template <typename T, typename = detail::enable_safe_integer_conversion<T, integer_type>>
    TYPE_SAFE_FORCE_INLINE integer& operator+=(const integer<T, Policy>& other)
    {
        value_ = Policy::template do_addition(value_, static_cast<T>(other));
        return *this;
    }
    TYPE_SAFE_DETAIL_MAKE_OP(+=)

    /// \group compound_assign
    /// \param 1
    /// \exclude
    template <typename T, typename = detail::enable_safe_integer_conversion<T, integer_type>>
    TYPE_SAFE_FORCE_INLINE integer& operator-=(const integer<T, Policy>& other)
    {
        value_ = Policy::template do_subtraction(value_, static_cast<T>(other));
        return *this;
        return *this;
    }
    TYPE_SAFE_DETAIL_MAKE_OP(-=)

    /// \group compound_assign
    /// \param 1
    /// \exclude
    template <typename T, typename = detail::enable_safe_integer_conversion<T, integer_type>>
    TYPE_SAFE_FORCE_INLINE integer& operator*=(const integer<T, Policy>& other)
    {
        value_ = Policy::template do_multiplication(value_, static_cast<T>(other));
        return *this;
    }
    TYPE_SAFE_DETAIL_MAKE_OP(*=)

    /// \group compound_assign
    /// \param 1
    /// \exclude
    template <typename T, typename = detail::enable_safe_integer_conversion<T, integer_type>>
    TYPE_SAFE_FORCE_INLINE integer& operator/=(const integer<T, Policy>& other)
    {
        value_ = Policy::template do_division(value_, static_cast<T>(other));
        return *this;
    }
    TYPE_SAFE_DETAIL_MAKE_OP(/=)

    /// \group compound_assign
    /// \param 1
    /// \exclude
    template <typename T, typename = detail::enable_safe_integer_conversion<T, integer_type>>
    TYPE_SAFE_FORCE_INLINE integer& operator%=(const integer<T, Policy>& other)
    {
        value_ = Policy::template do_modulo(value_, static_cast<T>(other));
        return *this;
    }
    TYPE_SAFE_DETAIL_MAKE_OP(%=)

#undef TYPE_SAFE_DETAIL_MAKE_OP

private:
    integer_type value_;
};

//=== operations ===//
/// \exclude
namespace detail
{
    template <typename T>
    struct make_signed
    {
        using type = typename std::make_signed<T>::type;
    };

    template <typename T, class Policy>
    struct make_signed<integer<T, Policy>>
    {
        using type = integer<typename std::make_signed<T>::type, Policy>;
    };

    template <typename T>
    struct make_unsigned
    {
        using type = typename std::make_unsigned<T>::type;
    };

    template <typename T, class Policy>
    struct make_unsigned<integer<T, Policy>>
    {
        using type = integer<typename std::make_unsigned<T>::type, Policy>;
    };
} // namespace detail

/// [std::make_signed]() for [ts::integer]().
/// \module types
/// \exclude target
template <class Integer>
using make_signed_t = typename detail::make_signed<Integer>::type;

/// \returns A new integer of the corresponding signed integer type.
/// \requires The value of `i` must fit into signed type.
/// \module types
/// \param 1
/// \exclude
template <typename Integer,
          typename = typename std::enable_if<detail::is_integer<Integer>::value>::type>
TYPE_SAFE_FORCE_INLINE constexpr make_signed_t<Integer> make_signed(const Integer& i)
{
    using result_type = make_signed_t<Integer>;
    return i <= Integer(std::numeric_limits<result_type>::max())
               ? static_cast<result_type>(i)
               : DEBUG_UNREACHABLE(detail::precondition_error_handler{}, "conversion "
                                                                         "would "
                                                                         "overflow");
}

/// \returns A new [ts::integer]() of the corresponding signed integer type.
/// \requires The value of `i` must fit into signed type.
/// \module types
template <typename Integer, class Policy>
TYPE_SAFE_FORCE_INLINE constexpr make_signed_t<integer<Integer, Policy>> make_signed(
    const integer<Integer, Policy>& i)
{
    return make_signed(static_cast<Integer>(i));
}

/// [std::make_unsigned]() for [ts::integer]().
/// \module types
/// \exclude target
template <class Integer>
using make_unsigned_t = typename detail::make_unsigned<Integer>::type;

/// \returns A new integer of the corresponding unsigned integer type.
/// \requires The value of `i` must not be negative.
/// \module types
/// \param 1
/// \exclude
template <typename Integer,
          typename = typename std::enable_if<detail::is_integer<Integer>::value>::type>
TYPE_SAFE_FORCE_INLINE constexpr make_unsigned_t<Integer> make_unsigned(const Integer& i)
{
    using result_type = make_unsigned_t<Integer>;
    return i >= Integer(0) ? static_cast<result_type>(i)
                           : DEBUG_UNREACHABLE(detail::precondition_error_handler{},
                                               "conversion would underflow");
}

/// \returns A new [ts::integer]() of the corresponding unsigned integer type.
/// \requires The value of `i` must not be negative.
/// \module types
template <typename Integer, class Policy>
TYPE_SAFE_FORCE_INLINE constexpr make_unsigned_t<integer<Integer, Policy>> make_unsigned(
    const integer<Integer, Policy>& i)
{
    return make_unsigned(static_cast<Integer>(i));
}

/// \returns The absolute value of a built-in signed integer.
/// It will be changed to the unsigned return type as well.
/// \module types
/// \param 1
/// \exclude
template <typename SignedInteger,
          typename = typename std::enable_if<std::is_signed<SignedInteger>::value>::type>
TYPE_SAFE_FORCE_INLINE constexpr make_unsigned_t<SignedInteger> abs(const SignedInteger& i)
{
    return make_unsigned(i > 0 ? i : -i);
}

/// \returns The absolute value of an [ts::integer]().
/// \module types
/// \param 2
/// \exclude
template <typename SignedInteger, class Policy,
          typename = typename std::enable_if<std::is_signed<SignedInteger>::value>::type>
TYPE_SAFE_FORCE_INLINE constexpr make_unsigned_t<integer<SignedInteger, Policy>> abs(
    const integer<SignedInteger, Policy>& i)
{
    return make_unsigned(i > 0 ? i : -i);
}

/// \returns `i` unchanged.
/// \notes This is an optimization of `abs()` for unsigned integer types.
/// \module types
/// \param 1
/// \exclude
template <typename UnsignedInteger,
          typename = typename std::enable_if<std::is_unsigned<UnsignedInteger>::value>::type>
TYPE_SAFE_FORCE_INLINE constexpr UnsignedInteger abs(const UnsignedInteger& i)
{
    return i;
}

/// \returns `i` unchanged.
/// \notes This is an optimization of `abs()` for unsigned integer types.
/// \module types
/// \param 2
/// \exclude
template <typename UnsignedInteger, class Policy,
          typename = typename std::enable_if<std::is_unsigned<UnsignedInteger>::value>::type>
TYPE_SAFE_FORCE_INLINE constexpr integer<UnsignedInteger, Policy> abs(
    const integer<UnsignedInteger, Policy>& i)
{
    return i;
}

//=== comparison ===//
/// \exclude
#define TYPE_SAFE_DETAIL_MAKE_OP(Op)                                                               \
    /** \group int_comp                                                                            \
     * \param 3                                                                                    \
     * \exclude */                                                                                 \
    template <typename A, typename B, class Policy,                                                \
              typename = detail::enable_safe_integer_comparison<A, B>>                             \
    TYPE_SAFE_FORCE_INLINE constexpr bool operator Op(const A& a, const integer<B, Policy>& b)     \
    {                                                                                              \
        return integer<A, Policy>(a) Op b;                                                         \
    }                                                                                              \
    /** \group int_comp                                                                            \
     * \param 3                                                                                    \
     * \exclude */                                                                                 \
    template <typename A, class Policy, typename B,                                                \
              typename = detail::enable_safe_integer_comparison<A, B>>                             \
    TYPE_SAFE_FORCE_INLINE constexpr bool operator Op(const integer<A, Policy>& a, const B& b)     \
    {                                                                                              \
        return a Op integer<B, Policy>(b);                                                         \
    }                                                                                              \
    /** \exclude */                                                                                \
    template <typename A, class Policy, typename B,                                                \
              typename = detail::fallback_safe_integer_comparison<A, B>>                           \
    constexpr bool operator Op(integer<A, Policy>, integer<B, Policy>) = delete;                   \
    /** \exclude */                                                                                \
    template <typename A, typename B, class Policy,                                                \
              typename = detail::fallback_safe_integer_comparison<A, B>>                           \
    constexpr bool operator Op(A, integer<B, Policy>) = delete;                                    \
    /** \exclude */                                                                                \
    template <typename A, class Policy, typename B,                                                \
              typename = detail::fallback_safe_integer_comparison<A, B>>                           \
    constexpr bool operator Op(integer<A, Policy>, B) = delete;

/// \returns The result of the comparison of the stored integer value in the [ts::integer]().
/// \notes These functions do not participate in overload resolution
/// unless `A` and `B` are both integer types.
/// \group int_comp Comparison operators
/// \module types
/// \param 3
/// \exclude
template <typename A, typename B, class Policy,
          typename = detail::enable_safe_integer_comparison<A, B>>
TYPE_SAFE_FORCE_INLINE constexpr bool operator==(const integer<A, Policy>& a,
                                                 const integer<B, Policy>& b)
{
    return static_cast<A>(a) == static_cast<B>(b);
}
TYPE_SAFE_DETAIL_MAKE_OP(==)

/// \group int_comp Comparison operators
/// \param 3
/// \exclude
template <typename A, typename B, class Policy,
          typename = detail::enable_safe_integer_comparison<A, B>>
TYPE_SAFE_FORCE_INLINE constexpr bool operator!=(const integer<A, Policy>& a,
                                                 const integer<B, Policy>& b)
{
    return static_cast<A>(a) != static_cast<B>(b);
}
TYPE_SAFE_DETAIL_MAKE_OP(!=)

/// \group int_comp Comparison operators
/// \param 3
/// \exclude
template <typename A, typename B, class Policy,
          typename = detail::enable_safe_integer_comparison<A, B>>
TYPE_SAFE_FORCE_INLINE constexpr bool operator<(const integer<A, Policy>& a,
                                                const integer<B, Policy>& b)
{
    return static_cast<A>(a) < static_cast<B>(b);
}
TYPE_SAFE_DETAIL_MAKE_OP(<)

/// \group int_comp Comparison operators
/// \param 3
/// \exclude
template <typename A, typename B, class Policy,
          typename = detail::enable_safe_integer_comparison<A, B>>
TYPE_SAFE_FORCE_INLINE constexpr bool operator<=(const integer<A, Policy>& a,
                                                 const integer<B, Policy>& b)
{
    return static_cast<A>(a) <= static_cast<B>(b);
}
TYPE_SAFE_DETAIL_MAKE_OP(<=)

/// \group int_comp Comparison operators
/// \param 3
/// \exclude
template <typename A, typename B, class Policy,
          typename = detail::enable_safe_integer_comparison<A, B>>
TYPE_SAFE_FORCE_INLINE constexpr bool operator>(const integer<A, Policy>& a,
                                                const integer<B, Policy>& b)
{
    return static_cast<A>(a) > static_cast<B>(b);
}
TYPE_SAFE_DETAIL_MAKE_OP(>)

/// \group int_comp Comparison operators
/// \param 3
/// \exclude
template <typename A, typename B, class Policy,
          typename = detail::enable_safe_integer_comparison<A, B>>
TYPE_SAFE_FORCE_INLINE constexpr bool operator>=(const integer<A, Policy>& a,
                                                 const integer<B, Policy>& b)
{
    return static_cast<A>(a) >= static_cast<B>(b);
}
TYPE_SAFE_DETAIL_MAKE_OP(>=)

#undef TYPE_SAFE_DETAIL_MAKE_OP

//=== binary operations ===//
/// \entity TYPE_SAFE_DETAIL_MAKE_OP
/// \exclude
#define TYPE_SAFE_DETAIL_MAKE_OP(Op)                                                               \
    /** \exclude return                                                                            \
     * \group int_binary_op */                                                                     \
    template <typename A, typename B, class Policy>                                                \
    TYPE_SAFE_FORCE_INLINE constexpr auto operator Op(const A& a, const integer<B, Policy>& b)     \
        ->integer<detail::integer_result_t<A, B>, Policy>                                          \
    {                                                                                              \
        return integer<A, Policy>(a) Op b;                                                         \
    }                                                                                              \
    /** \exclude return                                                                            \
     * \group int_binary_op */                                                                     \
    template <typename A, class Policy, typename B>                                                \
    TYPE_SAFE_FORCE_INLINE constexpr auto operator Op(const integer<A, Policy>& a, const B& b)     \
        ->integer<detail::integer_result_t<A, B>, Policy>                                          \
    {                                                                                              \
        return a Op integer<B, Policy>(b);                                                         \
    }                                                                                              \
    /** \exclude */                                                                                \
    template <typename A, typename B, class Policy,                                                \
              typename = detail::fallback_integer_result<A, B>>                                    \
    constexpr int operator Op(integer<A, Policy>, integer<B, Policy>) = delete;                    \
    /** \exclude */                                                                                \
    template <typename A, typename B, class Policy,                                                \
              typename = detail::fallback_integer_result<A, B>>                                    \
    constexpr int operator Op(A, integer<B, Policy>) = delete;                                     \
    /** \exclude */                                                                                \
    template <typename A, class Policy, typename B,                                                \
              typename = detail::fallback_integer_result<A, B>>                                    \
    constexpr int operator Op(integer<A, Policy>, B) = delete;

/// \returns The result of the binary operation of the stored integer value in the [ts::integer]().
/// The type is a [ts::integer]() of the bigger integer type.
/// \notes These functions do not participate in overload resolution,
/// unless `A` and `B` are both integer types.
/// \group int_binary_op Binary operations
/// \module types
/// \exclude return
template <typename A, typename B, class Policy>
TYPE_SAFE_FORCE_INLINE constexpr auto operator+(const integer<A, Policy>& a,
                                                const integer<B, Policy>& b)
    -> integer<detail::integer_result_t<A, B>, Policy>
{
    using type = detail::integer_result_t<A, B>;
    return Policy::template do_addition<type>(static_cast<A>(a), static_cast<B>(b));
}
TYPE_SAFE_DETAIL_MAKE_OP(+)

/// \group int_binary_op
/// \exclude return
template <typename A, typename B, class Policy>
TYPE_SAFE_FORCE_INLINE constexpr auto operator-(const integer<A, Policy>& a,
                                                const integer<B, Policy>& b)
    -> integer<detail::integer_result_t<A, B>, Policy>
{
    using type = detail::integer_result_t<A, B>;
    return Policy::template do_subtraction<type>(static_cast<A>(a), static_cast<B>(b));
}
TYPE_SAFE_DETAIL_MAKE_OP(-)

/// \group int_binary_op
/// \exclude return
template <typename A, typename B, class Policy>
TYPE_SAFE_FORCE_INLINE constexpr auto operator*(const integer<A, Policy>& a,
                                                const integer<B, Policy>& b)
    -> integer<detail::integer_result_t<A, B>, Policy>
{
    using type = detail::integer_result_t<A, B>;
    return Policy::template do_multiplication<type>(static_cast<A>(a), static_cast<B>(b));
}
TYPE_SAFE_DETAIL_MAKE_OP(*)

/// \group int_binary_op
/// \exclude return
template <typename A, typename B, class Policy>
TYPE_SAFE_FORCE_INLINE constexpr auto operator/(const integer<A, Policy>& a,
                                                const integer<B, Policy>& b)
    -> integer<detail::integer_result_t<A, B>, Policy>
{
    using type = detail::integer_result_t<A, B>;
    return Policy::template do_division<type>(static_cast<A>(a), static_cast<B>(b));
}
TYPE_SAFE_DETAIL_MAKE_OP(/)

/// \group int_binary_op
/// \exclude return
template <typename A, typename B, class Policy>
TYPE_SAFE_FORCE_INLINE constexpr auto operator%(const integer<A, Policy>& a,
                                                const integer<B, Policy>& b)
    -> integer<detail::integer_result_t<A, B>, Policy>
{
    using type = detail::integer_result_t<A, B>;
    return Policy::template do_modulo<type>(static_cast<A>(a), static_cast<B>(b));
}
TYPE_SAFE_DETAIL_MAKE_OP(%)

#undef TYPE_SAFE_DETAIL_MAKE_OP

//=== input/output ===/
/// \effects Reads an integer from the [std::istream]() and assigns it to the given [ts::integer]().
/// \module types
/// \output_section Input/output
template <typename Char, class CharTraits, typename IntegerT, class Policy>
std::basic_istream<Char, CharTraits>& operator>>(std::basic_istream<Char, CharTraits>& in,
                                                 integer<IntegerT, Policy>&            i)
{
    IntegerT val;
    in >> val;
    i = val;
    return in;
}

/// \effects Converts the given [ts::integer]() to the underlying integer type and writes it to th
/// [std::ostream](). \module types
template <typename Char, class CharTraits, typename IntegerT, class Policy>
std::basic_ostream<Char, CharTraits>& operator<<(std::basic_ostream<Char, CharTraits>& out,
                                                 const integer<IntegerT, Policy>&      i)
{
    return out << static_cast<IntegerT>(i);
}
} // namespace type_safe

namespace std
{
/// Hash specialization for [ts::integer].
/// \module types
template <typename IntegerT, class Policy>
struct hash<type_safe::integer<IntegerT, Policy>>
{
    std::size_t operator()(const type_safe::integer<IntegerT, Policy>& i) const noexcept
    {
        return std::hash<IntegerT>()(static_cast<IntegerT>(i));
    }
};
} // namespace std

#endif // TYPE_SAFE_INTEGER_HPP_INCLUDED
