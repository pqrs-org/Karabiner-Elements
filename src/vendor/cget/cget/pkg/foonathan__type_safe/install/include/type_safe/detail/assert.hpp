// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_DETAIL_ASSERT_HPP_INCLUDED
#define TYPE_SAFE_DETAIL_ASSERT_HPP_INCLUDED

#include <debug_assert.hpp>

#include <type_safe/config.hpp>

namespace type_safe
{
namespace detail
{
    struct assert_handler : debug_assert::set_level<TYPE_SAFE_ENABLE_ASSERTIONS>,
                            debug_assert::default_handler
    {};

    struct precondition_error_handler
    : debug_assert::set_level<TYPE_SAFE_ENABLE_PRECONDITION_CHECKS>,
      debug_assert::default_handler
    {};

    inline void on_disabled_exception() noexcept
    {
        struct handler : debug_assert::set_level<1>, debug_assert::default_handler
        {};
        DEBUG_UNREACHABLE(handler{}, "attempt to throw an exception but exceptions are disabled");
    }
} // namespace detail
} // namespace type_safe

#endif // TYPE_SAFE_DETAIL_ASSERT_HPP_INCLUDED`
