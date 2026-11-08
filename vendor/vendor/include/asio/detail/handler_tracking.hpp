//
// detail/handler_tracking.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_HANDLER_TRACKING_HPP
#define ASIO_DETAIL_HANDLER_TRACKING_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN

class execution_context;

ASIO_INLINE_NAMESPACE_END
} // namespace asio

#if defined(ASIO_CUSTOM_HANDLER_TRACKING)

# include ASIO_CUSTOM_HANDLER_TRACKING

// The user-specified header must define the following macros:
// - ASIO_INHERIT_TRACKED_HANDLER
// - ASIO_ALSO_INHERIT_TRACKED_HANDLER
// - ASIO_HANDLER_TRACKING_INIT
// - ASIO_HANDLER_CREATION(args)
// - ASIO_HANDLER_COMPLETION(args)
// - ASIO_HANDLER_INVOCATION_BEGIN(args)
// - ASIO_HANDLER_INVOCATION_END
// - ASIO_HANDLER_OPERATION(args)
// - ASIO_HANDLER_REACTOR_REGISTRATION(args)
// - ASIO_HANDLER_REACTOR_DEREGISTRATION(args)
// - ASIO_HANDLER_REACTOR_READ_EVENT
// - ASIO_HANDLER_REACTOR_WRITE_EVENT
// - ASIO_HANDLER_REACTOR_ERROR_EVENT
// - ASIO_HANDLER_REACTOR_EVENTS(args)
// - ASIO_HANDLER_REACTOR_OPERATION(args)
//
// It may also define the following macros:
// - ASIO_HANDLER_SCHEDULER_RUN(args)

# if !defined(ASIO_HANDLER_SCHEDULER_RUN)
#  define ASIO_HANDLER_SCHEDULER_RUN(args) (void)0
# endif // !defined(ASIO_HANDLER_SCHEDULER_RUN)

# if !defined(ASIO_ENABLE_HANDLER_TRACKING)
#  define ASIO_ENABLE_HANDLER_TRACKING 1
# endif // !defined(ASIO_ENABLE_HANDLER_TRACKING)

#elif defined(ASIO_PERFETTO_HANDLER_TRACKING)

# include "asio/detail/perfetto_handler_tracking.hpp"

#elif defined(ASIO_ENABLE_HANDLER_TRACKING)

# include "asio/detail/stderr_handler_tracking.hpp"

#else // defined(ASIO_ENABLE_HANDLER_TRACKING)

# define ASIO_INHERIT_TRACKED_HANDLER
# define ASIO_ALSO_INHERIT_TRACKED_HANDLER
# define ASIO_HANDLER_TRACKING_INIT (void)0
# define ASIO_HANDLER_LOCATION(loc) (void)0
# define ASIO_HANDLER_CREATION(args) (void)0
# define ASIO_HANDLER_COMPLETION(args) (void)0
# define ASIO_HANDLER_INVOCATION_BEGIN(args) (void)0
# define ASIO_HANDLER_INVOCATION_END (void)0
# define ASIO_HANDLER_OPERATION(args) (void)0
# define ASIO_HANDLER_REACTOR_REGISTRATION(args) (void)0
# define ASIO_HANDLER_REACTOR_DEREGISTRATION(args) (void)0
# define ASIO_HANDLER_REACTOR_READ_EVENT 0
# define ASIO_HANDLER_REACTOR_WRITE_EVENT 0
# define ASIO_HANDLER_REACTOR_ERROR_EVENT 0
# define ASIO_HANDLER_REACTOR_EVENTS(args) (void)0
# define ASIO_HANDLER_REACTOR_OPERATION(args) (void)0
# define ASIO_HANDLER_SCHEDULER_RUN(args) (void)0

#endif // defined(ASIO_ENABLE_HANDLER_TRACKING)

#endif // ASIO_DETAIL_HANDLER_TRACKING_HPP
