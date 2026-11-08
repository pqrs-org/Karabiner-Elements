//
// impl/io_context.ipp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_IMPL_IO_CONTEXT_IPP
#define ASIO_IMPL_IO_CONTEXT_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/config.hpp"
#include "asio/io_context.hpp"
#include "asio/detail/concurrency_hint.hpp"
#include "asio/detail/limits.hpp"
#include "asio/detail/service_registry.hpp"

#if defined(ASIO_HAS_IOCP)
# include "asio/detail/win_iocp_io_context.hpp"
#else
# include "asio/detail/scheduler.hpp"
#endif

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN

io_context::io_context()
  : execution_context(config_from_concurrency_hint()),
    impl_(asio::make_service<impl_type>(*this, false))
{
}

io_context::io_context(int concurrency_hint)
  : execution_context(config_from_concurrency_hint(concurrency_hint)),
    impl_(asio::make_service<impl_type>(*this, false))
{
}

io_context::io_context(const execution_context::service_maker& initial_services)
  : execution_context(initial_services),
    impl_(asio::make_service<impl_type>(*this, false))
{
}

io_context::~io_context()
{
  shutdown();
}

io_context::count_type io_context::run()
{
  return impl_.run();
}

io_context::count_type io_context::run_one()
{
  return impl_.run_one();
}

io_context::count_type io_context::poll()
{
  return impl_.poll();
}

io_context::count_type io_context::poll_one()
{
  return impl_.poll_one();
}

void io_context::stop()
{
  impl_.stop();
}

bool io_context::stopped() const
{
  return impl_.stopped();
}

void io_context::restart()
{
  impl_.restart();
}

io_context::service::service(asio::io_context& owner)
  : execution_context::service(owner)
{
}

io_context::service::~service()
{
}

void io_context::service::shutdown()
{
}

void io_context::service::notify_fork(io_context::fork_event)
{
}

ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_IMPL_IO_CONTEXT_IPP
