// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_DETAIL_MAP_INVOKE_HPP_INCLUDED
#define TYPE_SAFE_DETAIL_MAP_INVOKE_HPP_INCLUDED

#include <utility>

namespace type_safe
{
namespace detail
{
    template <typename Func, typename Value, typename... Args>
    auto map_invoke(Func&& f, Value&& v, Args&&... args)
        -> decltype(std::forward<Func>(f)(std::forward<Value>(v), std::forward<Args>(args)...))
    {
        return std::forward<Func>(f)(std::forward<Value>(v), std::forward<Args>(args)...);
    }

    template <typename Func, typename Value>
    auto map_invoke(Func&& f, Value&& v) -> decltype(std::forward<Value>(v).*std::forward<Func>(f))
    {
        return std::forward<Value>(v).*std::forward<Func>(f);
    }

    template <typename Func, typename Value, typename... Args>
    auto map_invoke(Func&& f, Value&& v, Args&&... args)
        -> decltype((std::forward<Value>(v).*std::forward<Func>(f))(std::forward<Args>(args)...))
    {
        return (std::forward<Value>(v).*std::forward<Func>(f))(std::forward<Args>(args)...);
    }
} // namespace detail
} // namespace type_safe

#endif // TYPE_SAFE_MAP_INVOKE_HPP_INCLUDED
