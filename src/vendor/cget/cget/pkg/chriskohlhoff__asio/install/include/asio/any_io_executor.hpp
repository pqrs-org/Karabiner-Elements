//
// any_io_executor.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_ANY_IO_EXECUTOR_HPP
#define ASIO_ANY_IO_EXECUTOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#if defined(ASIO_USE_TS_EXECUTOR_AS_DEFAULT)
# include "asio/executor.hpp"
#else // defined(ASIO_USE_TS_EXECUTOR_AS_DEFAULT)
# include "asio/execution.hpp"
# include "asio/execution_context.hpp"
#endif // defined(ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

#include "asio/detail/push_options.hpp"

namespace asio {

#if defined(ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

typedef executor any_io_executor;

#else // defined(ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

/// Polymorphic executor type for use with I/O objects.
/**
 * The @c any_io_executor type is a polymorphic executor that supports the set
 * of properties required by I/O objects. It is defined as the
 * execution::any_executor class template parameterised as follows:
 * @code execution::any_executor<
 *   execution::context_as_t<execution_context&>,
 *   execution::blocking_t::never_t,
 *   execution::prefer_only<execution::blocking_t::possibly_t>,
 *   execution::prefer_only<execution::outstanding_work_t::tracked_t>,
 *   execution::prefer_only<execution::outstanding_work_t::untracked_t>,
 *   execution::prefer_only<execution::relationship_t::fork_t>,
 *   execution::prefer_only<execution::relationship_t::continuation_t>
 * > @endcode
 */
#if defined(GENERATING_DOCUMENTATION)
typedef execution::any_executor<...> any_io_executor;
#else // defined(GENERATING_DOCUMENTATION)
typedef execution::any_executor<
    execution::context_as_t<execution_context&>,
    execution::blocking_t::never_t,
    execution::prefer_only<execution::blocking_t::possibly_t>,
    execution::prefer_only<execution::outstanding_work_t::tracked_t>,
    execution::prefer_only<execution::outstanding_work_t::untracked_t>,
    execution::prefer_only<execution::relationship_t::fork_t>,
    execution::prefer_only<execution::relationship_t::continuation_t>
  > any_io_executor;
#endif // defined(GENERATING_DOCUMENTATION)

#endif // defined(ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_ANY_IO_EXECUTOR_HPP
