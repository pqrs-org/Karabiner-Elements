// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_DETAIL_ALIGNED_UNION_HPP_INCLUDED
#define TYPE_SAFE_DETAIL_ALIGNED_UNION_HPP_INCLUDED

#include <type_traits>

namespace type_safe
{
namespace detail
{
    // max for variadic number of types.
    template <typename T>
    constexpr const T& max(const T& a)
    {
        return a;
    }

    template <typename T>
    constexpr const T& max(const T& a, const T& b)
    {
        return a < b ? b : a;
    }

    template <typename T, typename... Ts>
    constexpr const T& max(const T& t, const Ts&... ts)
    {
        return max(t, max(ts...));
    }

    // std::aligned_union not available on all compilers
    template <typename... Types>
    struct aligned_union
    {
        static constexpr auto size_value      = detail::max(sizeof(Types)...);
        static constexpr auto alignment_value = detail::max(alignof(Types)...);

        using type = typename std::aligned_storage<size_value, alignment_value>::type;
    };

    template <typename... Types>
    using aligned_union_t = typename aligned_union<Types...>::type;
} // namespace detail
} // namespace type_safe

#endif // TYPE_SAFE_DETAIL_ALIGNED_UNION_HPP_INCLUDED
