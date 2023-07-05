// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_DETAIL_ALIGNED_UNION_HPP_INCLUDED
#define TYPE_SAFE_DETAIL_ALIGNED_UNION_HPP_INCLUDED

#if defined(TYPE_SAFE_IMPORT_STD_MODULE)
import std;
#else
#    include <cstddef>
#    include <type_traits>
#endif

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

    template <typename... Types>
    class aligned_union
    {
    public:
        static constexpr auto size_value      = detail::max(sizeof(Types)...);
        static constexpr auto alignment_value = detail::max(alignof(Types)...);

        void* get() noexcept
        {
            return &storage_;
        }
        const void* get() const noexcept
        {
            return &storage_;
        }

    private:
        alignas(alignment_value) unsigned char storage_[size_value];
    };
} // namespace detail
} // namespace type_safe

#endif // TYPE_SAFE_DETAIL_ALIGNED_UNION_HPP_INCLUDED
