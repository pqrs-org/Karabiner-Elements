//
// impl/dispatch.hpp
// ~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2019 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_IMPL_DISPATCH_HPP
#define ASIO_IMPL_DISPATCH_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/associated_allocator.hpp"
#include "asio/associated_executor.hpp"
#include "asio/detail/work_dispatcher.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

struct initiate_dispatch
{
  template <typename CompletionHandler>
  void operator()(ASIO_MOVE_ARG(CompletionHandler) handler) const
  {
    typedef typename decay<CompletionHandler>::type DecayedHandler;

    typename associated_executor<DecayedHandler>::type ex(
        (get_associated_executor)(handler));

    typename associated_allocator<DecayedHandler>::type alloc(
        (get_associated_allocator)(handler));

    ex.dispatch(ASIO_MOVE_CAST(CompletionHandler)(handler), alloc);
  }

  template <typename CompletionHandler, typename Executor>
  void operator()(ASIO_MOVE_ARG(CompletionHandler) handler,
      ASIO_MOVE_ARG(Executor) ex) const
  {
    typedef typename decay<CompletionHandler>::type DecayedHandler;

    typename associated_allocator<DecayedHandler>::type alloc(
        (get_associated_allocator)(handler));

    ex.dispatch(detail::work_dispatcher<DecayedHandler>(
          ASIO_MOVE_CAST(CompletionHandler)(handler)), alloc);
  }
};

} // namespace detail

template <typename CompletionToken>
ASIO_INITFN_RESULT_TYPE(CompletionToken, void()) dispatch(
    ASIO_MOVE_ARG(CompletionToken) token)
{
  return async_initiate<CompletionToken, void()>(
      detail::initiate_dispatch(), token);
}

template <typename Executor, typename CompletionToken>
ASIO_INITFN_RESULT_TYPE(CompletionToken, void()) dispatch(
    const Executor& ex, ASIO_MOVE_ARG(CompletionToken) token,
    typename enable_if<is_executor<Executor>::value>::type*)
{
  return async_initiate<CompletionToken, void()>(
      detail::initiate_dispatch(), token, ex);
}

template <typename ExecutionContext, typename CompletionToken>
inline ASIO_INITFN_RESULT_TYPE(CompletionToken, void()) dispatch(
    ExecutionContext& ctx, ASIO_MOVE_ARG(CompletionToken) token,
    typename enable_if<is_convertible<
      ExecutionContext&, execution_context&>::value>::type*)
{
  return (dispatch)(ctx.get_executor(),
      ASIO_MOVE_CAST(CompletionToken)(token));
}

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_IMPL_DISPATCH_HPP
