//
// detail/fiber.hpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_FIBER_HPP
#define ASIO_DETAIL_FIBER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_ASM_FIBER)
# include "asio/detail/asm_fiber.hpp"
#elif defined(ASIO_HAS_BOOST_CONTEXT_FIBER)
# include "asio/detail/boost_fiber.hpp"
#else
# error No spawn() implementation available
#endif

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

#if defined(ASIO_HAS_ASM_FIBER)
using fiber = asm_fiber;
using forced_unwind = asm_fiber_forced_unwind;
#elif defined(ASIO_HAS_BOOST_CONTEXT_FIBER)
using fiber = boost_fiber;
using forced_unwind = boost_forced_unwind;
#endif

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#endif // ASIO_DETAIL_FIBER_HPP
