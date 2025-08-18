//
// detail/impl/resolver_thread_pool.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2025 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_IMPL_RESOLVER_THREAD_POOL_IPP
#define ASIO_DETAIL_IMPL_RESOLVER_THREAD_POOL_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/config.hpp"
#include "asio/detail/memory.hpp"
#include "asio/detail/resolver_thread_pool.hpp"
#include "asio/error.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

class resolver_thread_pool::work_scheduler_runner
{
public:
  work_scheduler_runner(scheduler_impl& work_scheduler)
    : work_scheduler_(work_scheduler)
  {
  }

  void operator()()
  {
    asio::error_code ec;
    work_scheduler_.run(ec);
  }

private:
  scheduler_impl& work_scheduler_;
};

resolver_thread_pool::resolver_thread_pool(execution_context& context)
  : execution_context_service_base<resolver_thread_pool>(context),
    scheduler_(asio::use_service<scheduler_impl>(context)),
    work_scheduler_(scheduler_impl::internal(), context),
    work_threads_(execution_context::allocator<void>(context)),
    num_work_threads_(config(context).get("resolver", "threads", 0U)),
    scheduler_locking_(config(context).get("scheduler", "locking", true)),
    shutdown_(false)
{
  work_scheduler_.work_started();
  if (num_work_threads_ > 0)
    start_work_threads();
  else
    num_work_threads_ = 1;
}

resolver_thread_pool::~resolver_thread_pool()
{
  shutdown();
}

void resolver_thread_pool::shutdown()
{
  if (!shutdown_)
  {
    work_scheduler_.work_finished();
    work_scheduler_.stop();
    work_threads_.join();
    work_scheduler_.shutdown();
    shutdown_ = true;
  }
}

void resolver_thread_pool::notify_fork(execution_context::fork_event fork_ev)
{
  if (!work_threads_.empty())
  {
    if (fork_ev == execution_context::fork_prepare)
    {
      work_scheduler_.stop();
      work_threads_.join();
    }
  }
  else if (fork_ev != execution_context::fork_prepare)
  {
    work_scheduler_.restart();
  }
}

void resolver_thread_pool::start_resolve_op(resolve_op* op)
{
  if (scheduler_locking_)
  {
    start_work_threads();
    scheduler_.work_started();
    work_scheduler_.post_immediate_completion(op, false);
  }
  else
  {
    op->ec_ = asio::error::operation_not_supported;
    scheduler_.post_immediate_completion(op, false);
  }
}

void resolver_thread_pool::start_work_threads()
{
  asio::detail::mutex::scoped_lock lock(mutex_);
  if (work_threads_.empty())
    for (unsigned int i = 0; i < num_work_threads_; ++i)
      work_threads_.create_thread(work_scheduler_runner(work_scheduler_));
}

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_IMPL_RESOLVER_THREAD_POOL_IPP
