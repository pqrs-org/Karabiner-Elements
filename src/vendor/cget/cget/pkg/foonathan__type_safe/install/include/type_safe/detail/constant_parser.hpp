// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_DETAIL_CONSTANT_PARSER_HPP_INCLUDED
#define TYPE_SAFE_DETAIL_CONSTANT_PARSER_HPP_INCLUDED

#include <type_traits>

namespace type_safe
{
namespace detail
{
    template <typename... Ts>
    struct conditional_impl;

    template <typename Else>
    struct conditional_impl<Else>
    {
        using type = Else;
    };

    template <typename Result, typename... Tail>
    struct conditional_impl<std::true_type, Result, Tail...>
    {
        using type = Result;
    };

    template <typename Result, typename... Tail>
    struct conditional_impl<std::false_type, Result, Tail...>
    {
        using type = typename conditional_impl<Tail...>::type;
    };

    template <typename... Ts>
    using conditional = typename conditional_impl<Ts...>::type;

    template <bool Value>
    using bool_constant = std::integral_constant<bool, Value>;

    struct decimal_digit
    {};
    struct lower_hexadecimal_digit
    {};
    struct upper_hexadecimal_digit
    {};
    struct no_digit
    {};

    template <char C>
    using digit_category
        = conditional<bool_constant<C >= '0' && C <= '9'>, decimal_digit,
                      bool_constant<C >= 'a' && C <= 'f'>, lower_hexadecimal_digit,
                      bool_constant<C >= 'A' && C <= 'F'>, upper_hexadecimal_digit, no_digit>;

    template <char C, typename Cat>
    struct to_digit_impl
    {
        static_assert(!std::is_same<Cat, no_digit>::value, "invalid character, expected digit");
    };

    template <char C>
    struct to_digit_impl<C, decimal_digit>
    {
        static constexpr auto value = static_cast<int>(C) - static_cast<int>('0');
    };

    template <char C>
    struct to_digit_impl<C, lower_hexadecimal_digit>
    {
        static constexpr auto value = static_cast<int>(C) - static_cast<int>('a') + 10;
    };

    template <char C>
    struct to_digit_impl<C, upper_hexadecimal_digit>
    {
        static constexpr auto value = static_cast<int>(C) - static_cast<int>('A') + 10;
    };

    template <typename T, T Base, char C>
    constexpr T to_digit()
    {
        using impl = to_digit_impl<C, digit_category<C>>;
        static_assert(impl::value < Base, "invalid digit for base");
        return impl::value;
    }

    template <char... Digits>
    struct parse_loop;

    template <>
    struct parse_loop<>
    {
        template <typename T, T>
        static constexpr T parse(T value)
        {
            return value;
        }
    };

    template <char... Tail>
    struct parse_loop<'\'', Tail...>
    {
        template <typename T, T Base>
        static constexpr T parse(T value)
        {
            return parse_loop<Tail...>::template parse<T, Base>(value);
        }
    };

    template <char Head, char... Tail>
    struct parse_loop<Head, Tail...>
    {
        template <typename T, T Base>
        static constexpr T parse(T value)
        {
            return parse_loop<Tail...>::template parse<T, Base>(value * Base
                                                                + to_digit<T, Base, Head>());
        }
    };

    template <typename T, T Base, char Head, char... Tail>
    constexpr T do_parse_loop()
    {
        return parse_loop<Tail...>::template parse<T, Base>(to_digit<T, Base, Head>());
    }

    template <typename T, char... Digits>
    struct parse_base
    {
        static constexpr T parse()
        {
            return do_parse_loop<T, 10, Digits...>();
        }
    };

    template <typename T, char Head, char... Tail>
    struct parse_base<T, '0', Head, Tail...>
    {
        static constexpr T parse()
        {
            return do_parse_loop<T, 8, Head, Tail...>();
        }
    };

    template <typename T, char... Tail>
    struct parse_base<T, '0', 'x', Tail...>
    {
        static constexpr T parse()
        {
            return do_parse_loop<T, 16, Tail...>();
        }
    };

    template <typename T, char... Tail>
    struct parse_base<T, '0', 'X', Tail...>
    {
        static constexpr T parse()
        {
            return do_parse_loop<T, 16, Tail...>();
        }
    };

    template <typename T, char... Tail>
    struct parse_base<T, '0', 'b', Tail...>
    {
        static constexpr T parse()
        {
            return do_parse_loop<T, 2, Tail...>();
        }
    };

    template <typename T, char... Tail>
    struct parse_base<T, '0', 'B', Tail...>
    {
        static constexpr T parse()
        {
            return do_parse_loop<T, 2, Tail...>();
        }
    };

    template <typename T, char... Digits>
    constexpr T parse()
    {
        return parse_base<T, Digits...>::parse();
    }
} // namespace detail
} // namespace type_safe

#endif // TYPE_SAFE_DETAIL_CONSTANT_PARSER_HPP_INCLUDED
