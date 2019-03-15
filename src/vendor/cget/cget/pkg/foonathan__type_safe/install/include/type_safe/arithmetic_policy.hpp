// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_ARITHMETIC_POLICY_HPP_INCLUDED
#define TYPE_SAFE_ARITHMETIC_POLICY_HPP_INCLUDED

#include <limits>
#include <stdexcept>
#include <type_traits>

#include <type_safe/config.hpp>
#include <type_safe/detail/assert.hpp>
#include <type_safe/detail/force_inline.hpp>

namespace type_safe
{
/// An `ArithmeticPolicy` that behaves like the default integer implementations:
/// Signed under/overflow is UB, unsigned under/overflow wraps around.
/// \module types
class default_arithmetic
{
public:
    template <typename T>
    TYPE_SAFE_FORCE_INLINE static constexpr T do_addition(const T& a, const T& b) noexcept
    {
        return a + b;
    }

    template <typename T>
    TYPE_SAFE_FORCE_INLINE static constexpr T do_subtraction(const T& a, const T& b) noexcept
    {
        return a - b;
    }

    template <typename T>
    TYPE_SAFE_FORCE_INLINE static constexpr T do_multiplication(const T& a, const T& b) noexcept
    {
        return a * b;
    }

    template <typename T>
    TYPE_SAFE_FORCE_INLINE static constexpr T do_division(const T& a, const T& b) noexcept
    {
        return a / b;
    }

    template <typename T>
    TYPE_SAFE_FORCE_INLINE static constexpr T do_modulo(const T& a, const T& b) noexcept
    {
        return a % b;
    }
};

/// \exclude
namespace detail
{
    struct signed_integer_tag
    {};
    struct unsigned_integer_tag
    {};

    template <typename T>
    using arithmetic_tag_for =
        typename std::conditional<std::is_signed<T>::value, signed_integer_tag,
                                  unsigned_integer_tag>::type;

    template <typename T>
    constexpr bool will_addition_error(signed_integer_tag, const T& a, const T& b)
    {
        return b > T(0) ? a > std::numeric_limits<T>::max() - b
                        : a < std::numeric_limits<T>::min() - b;
    }
    template <typename T>
    constexpr bool will_addition_error(unsigned_integer_tag, const T& a, const T& b)
    {
        return std::numeric_limits<T>::max() - b < a;
    }

    template <typename T>
    constexpr bool will_subtraction_error(signed_integer_tag, const T& a, const T& b)
    {
        return b > T(0) ? a < std::numeric_limits<T>::min() + b
                        : a > std::numeric_limits<T>::max() + b;
    }
    template <typename T>
    constexpr bool will_subtraction_error(unsigned_integer_tag, const T& a, const T& b)
    {
        return a < b;
    }

    template <typename T>
    constexpr bool will_multiplication_error(signed_integer_tag, const T& a, const T& b)
    {
        return a > T(0) ? (b > T(0) ? a > std::numeric_limits<T>::max() / b : // a, b > 0
                               b < std::numeric_limits<T>::min() / a)
                        :                                                    // a > 0, b <= 0
                   (b > T(0) ? a < std::numeric_limits<T>::min() / b :       // a <= 0, b > 0
                        a != T(0) && b < std::numeric_limits<T>::max() / a); // a, b <= 0
    }
    template <typename T>
    constexpr bool will_multiplication_error(unsigned_integer_tag, const T& a, const T& b)
    {
        return b != T(0) && a > std::numeric_limits<T>::max() / b;
    }

    template <typename T>
    constexpr bool will_division_error(signed_integer_tag, const T& a, const T& b)
    {
        return b == T(0) || (b == T(-1) && a == std::numeric_limits<T>::min());
    }
    template <typename T>
    constexpr bool will_division_error(unsigned_integer_tag, const T&, const T& b)
    {
        return b == T(0);
    }

    template <typename T>
    constexpr bool will_modulo_error(signed_integer_tag, const T&, const T& b)
    {
        return b == T(0);
    }
    template <typename T>
    constexpr bool will_modulo_error(unsigned_integer_tag, const T&, const T& b)
    {
        return b == T(0);
    }
} // namespace detail

/// An `ArithmeticPolicy` where under/overflow is always undefined behavior,
/// albeit checked when assertions are enabled.
/// \module types
class undefined_behavior_arithmetic
{
public:
    template <typename T>
    TYPE_SAFE_FORCE_INLINE static constexpr T do_addition(const T& a, const T& b) noexcept
    {
        return detail::will_addition_error(detail::arithmetic_tag_for<T>{}, a, b)
                   ? DEBUG_UNREACHABLE(detail::precondition_error_handler{},
                                       "addition will result in overflow")
                   : T(a + b);
    }

    template <typename T>
    TYPE_SAFE_FORCE_INLINE static constexpr T do_subtraction(const T& a, const T& b) noexcept
    {
        return detail::will_subtraction_error(detail::arithmetic_tag_for<T>{}, a, b)
                   ? DEBUG_UNREACHABLE(detail::precondition_error_handler{},
                                       "subtraction will result in underflow")
                   : T(a - b);
    }

    template <typename T>
    TYPE_SAFE_FORCE_INLINE static constexpr T do_multiplication(const T& a, const T& b) noexcept
    {
        return detail::will_multiplication_error(detail::arithmetic_tag_for<T>{}, a, b)
                   ? DEBUG_UNREACHABLE(detail::precondition_error_handler{},
                                       "multiplication will result in overflow")
                   : T(a * b);
    }

    template <typename T>
    TYPE_SAFE_FORCE_INLINE static constexpr T do_division(const T& a, const T& b) noexcept
    {
        return detail::will_division_error(detail::arithmetic_tag_for<T>{}, a, b)
                   ? DEBUG_UNREACHABLE(detail::precondition_error_handler{},
                                       "division by zero/overflow")
                   : T(a / b);
    }

    template <typename T>
    TYPE_SAFE_FORCE_INLINE static constexpr T do_modulo(const T& a, const T& b) noexcept
    {
        return detail::will_modulo_error(detail::arithmetic_tag_for<T>{}, a, b)
                   ? DEBUG_UNREACHABLE(detail::precondition_error_handler{}, "modulo by zero")
                   : T(a % b);
    }
};

/// An `ArithmeticPolicy` where under/overflow throws an exception.
/// \notes If exceptions are not supported,
/// this is will assert.
/// \module types
class checked_arithmetic
{
public:
    class error : public std::range_error
    {
    public:
        error(const char* msg) : std::range_error(msg)
        {
#if !TYPE_SAFE_USE_EXCEPTIONS
            DEBUG_UNREACHABLE(detail::precondition_error_handler{}, msg);
#endif
        }
    };

    template <typename T>
    TYPE_SAFE_FORCE_INLINE static constexpr T do_addition(const T& a, const T& b)
    {
        return detail::will_addition_error(detail::arithmetic_tag_for<T>{}, a, b)
               ? TYPE_SAFE_THROW(error("addition will result in overflow")),
               a : a + b;
    }

    template <typename T>
    TYPE_SAFE_FORCE_INLINE static constexpr T do_subtraction(const T& a, const T& b)
    {
        return detail::will_subtraction_error(detail::arithmetic_tag_for<T>{}, a, b)
               ? TYPE_SAFE_THROW(error("subtraction will result in underflow")),
               a : a - b;
    }

    template <typename T>
    TYPE_SAFE_FORCE_INLINE static constexpr T do_multiplication(const T& a, const T& b)
    {
        return detail::will_multiplication_error(detail::arithmetic_tag_for<T>{}, a, b)
               ? TYPE_SAFE_THROW(error("multiplication will result in overflow")),
               a : a * b;
    }

    template <typename T>
    TYPE_SAFE_FORCE_INLINE static constexpr T do_division(const T& a, const T& b)
    {
        return detail::will_division_error(detail::arithmetic_tag_for<T>{}, a, b)
               ? TYPE_SAFE_THROW(error("division by zero/overflow")),
               a : a / b;
    }

    template <typename T>
    TYPE_SAFE_FORCE_INLINE static constexpr T do_modulo(const T& a, const T& b)
    {
        return detail::will_modulo_error(detail::arithmetic_tag_for<T>{}, a, b)
               ? TYPE_SAFE_THROW(error("modulo by zero")),
               a : a % b;
    }
};

#if TYPE_SAFE_ARITHMETIC_UB
/// The default `ArithmeticPolicy`.
///
/// It depends on the [TYPE_SAFE_ARITHMETIC_UB]() macro,
/// and is either [ts::undefined_behavior_arithmetic]() or [ts::default_arithmetic]().
/// \exclude target
/// \module types
using arithmetic_policy_default = undefined_behavior_arithmetic;
#else
using arithmetic_policy_default = default_arithmetic;
#endif
} // namespace type_safe

#endif // TYPE_SAFE_ARITHMETIC_POLICY_HPP_INCLUDED
