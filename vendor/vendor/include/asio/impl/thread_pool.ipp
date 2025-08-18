//
// impl/thread_pool.ipp
// ~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2025 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_IMPL_THREAD_POOL_IPP
#define ASIO_IMPL_THREAD_POOL_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include <stdexcept>
#include "asio/thread_pool.hpp"
#include "asio/detail/throw_exception.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {

struct thread_pool::thread_function
{
  detail::scheduler* scheduler_;

  void operator()()
  {
#if !defined(ASIO_NO_EXCEPTIONS)
    try
    {
#endif// !defined(ASIO_NO_EXCEPTIONS)
      asio::error_code ec;
      scheduler_->run(ec);
#if !defined(ASIO_NO_EXCEPTIONS)
    }
    catch (...)
    {
      std::terminate();
    }
#endif// !defined(ASIO_NO_EXCEPTIONS)
  }
};

#if !defined(ASIO_NO_TS_EXECUTORS)

long thread_pool::default_thread_pool_size()
{
  std::size_t num_threads = detail::thread::hardware_concurrency() * 2;
  num_threads = num_threads == 0 ? 2 : num_threads;
  return static_cast<long>(num_threads);
}

thread_pool::thread_pool()
  : scheduler_(asio::make_service<detail::scheduler>(*this, false)),
    threads_(allocator<void>(*this)),
    num_threads_(default_thread_pool_size()),
    joinable_(true)
{
  start();
}

#endif // !defined(ASIO_NO_TS_EXECUTORS)

long thread_pool::clamp_thread_pool_size(std::size_t n)
{
  if (n > 0x7FFFFFFF)
  {
    std::out_of_range ex("thread pool size");
    asio::detail::throw_exception(ex);
  }
  return static_cast<long>(n & 0x7FFFFFFF);
}

thread_pool::thread_pool(std::size_t num_threads)
  : execution_context(config_from_concurrency_hint(num_threads == 1 ? 1 : 0)),
    scheduler_(asio::make_service<detail::scheduler>(*this, false)),
    threads_(allocator<void>(*this)),
    num_threads_(clamp_thread_pool_size(num_threads)),
    joinable_(true)
{
  start();
}

thread_pool::thread_pool(std::size_t num_threads,
    const execution_context::service_maker& initial_services)
  : execution_context(initial_services),
    scheduler_(asio::make_service<detail::scheduler>(*this, false)),
    threads_(allocator<void>(*this)),
    num_threads_(clamp_thread_pool_size(num_threads)),
    joinable_(true)
{
  start();
}

thread_pool::~thread_pool()
{
  stop();
  join();
  shutdown();
}

void thread_pool::start()
{
  scheduler_.work_started();
  thread_function f = { &scheduler_ };
  threads_.create_threads(f, static_cast<std::size_t>(num_threads_));
}

void thread_pool::stop()
{
  scheduler_.stop();
}

void thread_pool::attach()
{
  ++num_threads_;
  thread_function f = { &scheduler_ };
  f();
}

void thread_pool::join()
{
  if (joinable_)
  {
    joinable_ = false;
    scheduler_.work_finished();
    threads_.join();
  }
}

void thread_pool::wait()
{
  join();
}

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_IMPL_THREAD_POOL_IPP
