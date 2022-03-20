//
// detail/work_dispatcher.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2022 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_WORK_DISPATCHER_HPP
#define ASIO_DETAIL_WORK_DISPATCHER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/bind_handler.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/associated_executor.hpp"
#include "asio/associated_allocator.hpp"
#include "asio/executor_work_guard.hpp"
#include "asio/execution/executor.hpp"
#include "asio/execution/allocator.hpp"
#include "asio/execution/blocking.hpp"
#include "asio/execution/outstanding_work.hpp"
#include "asio/prefer.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

template <typename Handler, typename Executor, typename = void>
struct is_work_dispatcher_required : true_type
{
};

template <typename Handler, typename Executor>
struct is_work_dispatcher_required<Handler, Executor,
    typename enable_if<
      is_same<
        typename associated_executor<Handler,
          Executor>::asio_associated_executor_is_unspecialised,
        void
      >::value
    >::type> : false_type
{
};

template <typename Handler, typename Executor, typename = void>
class work_dispatcher
{
public:
  template <typename CompletionHandler>
  work_dispatcher(ASIO_MOVE_ARG(CompletionHandler) handler,
      const Executor& handler_ex)
    : handler_(ASIO_MOVE_CAST(CompletionHandler)(handler)),
      executor_(asio::prefer(handler_ex,
          execution::outstanding_work.tracked))
  {
  }

#if defined(ASIO_HAS_MOVE)
  work_dispatcher(const work_dispatcher& other)
    : handler_(other.handler_),
      executor_(other.executor_)
  {
  }

  work_dispatcher(work_dispatcher&& other)
    : handler_(ASIO_MOVE_CAST(Handler)(other.handler_)),
      executor_(ASIO_MOVE_CAST(work_executor_type)(other.executor_))
  {
  }
#endif // defined(ASIO_HAS_MOVE)

  void operator()()
  {
    execution::execute(
        asio::prefer(executor_,
          execution::blocking.possibly,
          execution::allocator((get_associated_allocator)(handler_))),
        asio::detail::bind_handler(
          ASIO_MOVE_CAST(Handler)(handler_)));
  }

private:
  typedef typename decay<
      typename prefer_result<const Executor&,
        execution::outstanding_work_t::tracked_t
      >::type
    >::type work_executor_type;

  Handler handler_;
  work_executor_type executor_;
};

#if !defined(ASIO_NO_TS_EXECUTORS)

template <typename Handler, typename Executor>
class work_dispatcher<Handler, Executor,
    typename enable_if<!execution::is_executor<Executor>::value>::type>
{
public:
  template <typename CompletionHandler>
  work_dispatcher(ASIO_MOVE_ARG(CompletionHandler) handler,
      const Executor& handler_ex)
    : work_(handler_ex),
      handler_(ASIO_MOVE_CAST(CompletionHandler)(handler))
  {
  }

#if defined(ASIO_HAS_MOVE)
  work_dispatcher(const work_dispatcher& other)
    : work_(other.work_),
      handler_(other.handler_)
  {
  }

  work_dispatcher(work_dispatcher&& other)
    : work_(ASIO_MOVE_CAST(executor_work_guard<Executor>)(other.work_)),
      handler_(ASIO_MOVE_CAST(Handler)(other.handler_))
  {
  }
#endif // defined(ASIO_HAS_MOVE)

  void operator()()
  {
    typename associated_allocator<Handler>::type alloc(
        (get_associated_allocator)(handler_));
    work_.get_executor().dispatch(
        asio::detail::bind_handler(
          ASIO_MOVE_CAST(Handler)(handler_)), alloc);
    work_.reset();
  }

private:
  executor_work_guard<Executor> work_;
  Handler handler_;
};

#endif // !defined(ASIO_NO_TS_EXECUTORS)

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_WORK_DISPATCHER_HPP
