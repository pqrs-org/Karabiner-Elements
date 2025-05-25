// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_DETAIL_ALL_OF_HPP_INCLUDED
#define TYPE_SAFE_DETAIL_ALL_OF_HPP_INCLUDED

#if defined(TYPE_SAFE_IMPORT_STD_MODULE)
import std;
#else
#include <type_traits>
#endif

namespace type_safe
{
namespace detail
{
    template <bool... Bs>
    struct bool_sequence
    {};

    template <bool... Bs>
    using all_of = std::is_same<bool_sequence<Bs...>, bool_sequence<(true || Bs)...>>;

    template <bool... Bs>
    using none_of = std::is_same<bool_sequence<Bs...>, bool_sequence<(false && Bs)...>>;

    template <bool... Bs>
    using any_of = std::integral_constant<bool, !none_of<Bs...>::value>;
} // namespace detail
} // namespace type_safe

#endif // TYPE_SAFE_DETAIL_ALL_OF_HPP_INCLUDED
