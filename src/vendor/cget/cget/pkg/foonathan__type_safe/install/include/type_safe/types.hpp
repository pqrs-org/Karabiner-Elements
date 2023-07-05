// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_TYPES_HPP_INCLUDED
#define TYPE_SAFE_TYPES_HPP_INCLUDED

#if defined(TYPE_SAFE_IMPORT_STD_MODULE)
import std;
#else
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#endif

#include <type_safe/boolean.hpp>
#include <type_safe/config.hpp>
#include <type_safe/detail/constant_parser.hpp>
#include <type_safe/floating_point.hpp>
#include <type_safe/integer.hpp>

namespace type_safe
{
/// \exclude
namespace detail
{
    template <typename T, typename U, U Value>
    constexpr T validate_value()
    {
        static_assert(sizeof(T) <= sizeof(U)
                          && std::is_signed<U>::value == std::is_signed<T>::value,
                      "mismatched types");
        static_assert(U(std::numeric_limits<T>::min()) <= Value
                          && Value <= U(std::numeric_limits<T>::max()),
                      "integer literal overflow");
        return static_cast<T>(Value);
    }

    template <typename T, char... Digits>
    constexpr T parse_signed()
    {
        return validate_value<T, long long, parse<long long, Digits...>()>();
    }

    template <typename T, char... Digits>
    constexpr T parse_unsigned()
    {
        return validate_value<T, unsigned long long, parse<unsigned long long, Digits...>()>();
    }
} // namespace detail

#if TYPE_SAFE_ENABLE_WRAPPER
/// \exclude
#    define TYPE_SAFE_DETAIL_WRAP(templ, x) templ<x>
#else
#    define TYPE_SAFE_DETAIL_WRAP(templ, x) x
#endif

inline namespace types
{
    //=== fixed width integer ===//
    /// \module types
    using int8_t = TYPE_SAFE_DETAIL_WRAP(integer, std::int8_t);
    /// \module types
    using int16_t = TYPE_SAFE_DETAIL_WRAP(integer, std::int16_t);
    /// \module types
    using int32_t = TYPE_SAFE_DETAIL_WRAP(integer, std::int32_t);
    /// \module types
    using int64_t = TYPE_SAFE_DETAIL_WRAP(integer, std::int64_t);
    /// \module types
    using uint8_t = TYPE_SAFE_DETAIL_WRAP(integer, std::uint8_t);
    /// \module types
    using uint16_t = TYPE_SAFE_DETAIL_WRAP(integer, std::uint16_t);
    /// \module types
    using uint32_t = TYPE_SAFE_DETAIL_WRAP(integer, std::uint32_t);
    /// \module types
    using uint64_t = TYPE_SAFE_DETAIL_WRAP(integer, std::uint64_t);

    inline namespace literals
    {
        /// \module types
        template <char... Digits>
        constexpr int8_t operator"" _i8()
        {
            return int8_t(detail::parse_signed<std::int8_t, Digits...>());
        }

        /// \module types
        template <char... Digits>
        constexpr int16_t operator"" _i16()
        {
            return int16_t(detail::parse_signed<std::int16_t, Digits...>());
        }

        /// \module types
        template <char... Digits>
        constexpr int32_t operator"" _i32()
        {
            return int32_t(detail::parse_signed<std::int32_t, Digits...>());
        }

        /// \module types
        template <char... Digits>
        constexpr int64_t operator"" _i64()
        {
            return int64_t(detail::parse_signed<std::int64_t, Digits...>());
        }

        /// \module types
        template <char... Digits>
        constexpr uint8_t operator"" _u8()
        {
            return uint8_t(detail::parse_unsigned<std::uint8_t, Digits...>());
        }

        /// \module types
        template <char... Digits>
        constexpr uint16_t operator"" _u16()
        {
            return uint16_t(detail::parse_unsigned<std::uint16_t, Digits...>());
        }

        /// \module types
        template <char... Digits>
        constexpr uint32_t operator"" _u32()
        {
            return uint32_t(detail::parse_unsigned<std::uint32_t, Digits...>());
        }

        /// \module types
        template <char... Digits>
        constexpr uint64_t operator"" _u64()
        {
            return uint64_t(detail::parse_unsigned<std::uint64_t, Digits...>());
        }
    } // namespace literals

    /// \module types
    using int_fast8_t = TYPE_SAFE_DETAIL_WRAP(integer, std::int_fast8_t);
    /// \module types
    using int_fast16_t = TYPE_SAFE_DETAIL_WRAP(integer, std::int_fast16_t);
    /// \module types
    using int_fast32_t = TYPE_SAFE_DETAIL_WRAP(integer, std::int_fast32_t);
    /// \module types
    using int_fast64_t = TYPE_SAFE_DETAIL_WRAP(integer, std::int_fast64_t);
    /// \module types
    using uint_fast8_t = TYPE_SAFE_DETAIL_WRAP(integer, std::uint_fast8_t);
    /// \module types
    using uint_fast16_t = TYPE_SAFE_DETAIL_WRAP(integer, std::uint_fast16_t);
    /// \module types
    using uint_fast32_t = TYPE_SAFE_DETAIL_WRAP(integer, std::uint_fast32_t);
    /// \module types
    using uint_fast64_t = TYPE_SAFE_DETAIL_WRAP(integer, std::uint_fast64_t);

    /// \module types
    using int_least8_t = TYPE_SAFE_DETAIL_WRAP(integer, std::int_least8_t);
    /// \module types
    using int_least16_t = TYPE_SAFE_DETAIL_WRAP(integer, std::int_least16_t);
    /// \module types
    using int_least32_t = TYPE_SAFE_DETAIL_WRAP(integer, std::int_least32_t);
    /// \module types
    using int_least64_t = TYPE_SAFE_DETAIL_WRAP(integer, std::int_least64_t);
    /// \module types
    using uint_least8_t = TYPE_SAFE_DETAIL_WRAP(integer, std::uint_least8_t);
    /// \module types
    using uint_least16_t = TYPE_SAFE_DETAIL_WRAP(integer, std::uint_least16_t);
    /// \module types
    using uint_least32_t = TYPE_SAFE_DETAIL_WRAP(integer, std::uint_least32_t);
    /// \module types
    using uint_least64_t = TYPE_SAFE_DETAIL_WRAP(integer, std::uint_least64_t);

    /// \module types
    using intmax_t = TYPE_SAFE_DETAIL_WRAP(integer, std::intmax_t);
    /// \module types
    using uintmax_t = TYPE_SAFE_DETAIL_WRAP(integer, std::uintmax_t);
    /// \module types
    using intptr_t = TYPE_SAFE_DETAIL_WRAP(integer, std::intptr_t);
    /// \module types
    using uintptr_t = TYPE_SAFE_DETAIL_WRAP(integer, std::uintptr_t);

    //=== special integer types ===//
    /// \module types
    using ptrdiff_t = TYPE_SAFE_DETAIL_WRAP(integer, std::ptrdiff_t);
    /// \module types
    using size_t = TYPE_SAFE_DETAIL_WRAP(integer, std::size_t);

    /// \module types
    using int_t = TYPE_SAFE_DETAIL_WRAP(integer, int);
    /// \module types
    using unsigned_t = TYPE_SAFE_DETAIL_WRAP(integer, unsigned);

    inline namespace literals
    {
        /// \module types
        template <char... Digits>
        constexpr ptrdiff_t operator"" _isize()
        {
            return ptrdiff_t(detail::parse_signed<std::ptrdiff_t, Digits...>());
        }

        /// \module types
        template <char... Digits>
        constexpr size_t operator"" _usize()
        {
            return size_t(detail::parse_unsigned<std::size_t, Digits...>());
        }

        /// \module types
        template <char... Digits>
        constexpr int_t operator"" _i()
        {
            // int is at least 16 bits
            return int_t(detail::parse_signed<std::int16_t, Digits...>());
        }

        /// \module types
        template <char... Digits>
        constexpr unsigned_t operator"" _u()
        {
            // int is at least 16 bits
            return unsigned_t(detail::parse_unsigned<std::uint16_t, Digits...>());
        }
    } // namespace literals

    //=== floating point types ===//
    /// \module types
    using float_t = TYPE_SAFE_DETAIL_WRAP(floating_point, std::float_t);
    /// \module types
    using double_t = TYPE_SAFE_DETAIL_WRAP(floating_point, std::double_t);

    inline namespace literals
    {
        /// \module types
        constexpr float_t operator"" _f(long double val)
        {
            return float_t(static_cast<std::float_t>(val));
        }

        /// \module types
        constexpr double_t operator"" _d(long double val)
        {
            return double_t(static_cast<std::double_t>(val));
        }
    } // namespace literals

//=== boolean ===//
#if TYPE_SAFE_ENABLE_WRAPPER
    /// \module types
    using bool_t = boolean;
#else
    using bool_t = bool;
#endif
} // namespace types

#undef TYPE_SAFE_DETAIL_WRAP
} // namespace type_safe

#endif // TYPE_SAFE_TYPES_HPP_INCLUDED
