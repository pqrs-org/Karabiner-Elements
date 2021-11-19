//
// experimental/detail/channel_send_functions.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_EXPERIMENTAL_DETAIL_CHANNEL_SEND_FUNCTIONS_HPP
#define ASIO_EXPERIMENTAL_DETAIL_CHANNEL_SEND_FUNCTIONS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/async_result.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/experimental/detail/channel_message.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace experimental {
namespace detail {

template <typename Derived, typename Executor, typename... Signatures>
class channel_send_functions;

template <typename Derived, typename Executor, typename R, typename... Args>
class channel_send_functions<Derived, Executor, R(Args...)>
{
public:
  template <typename... Args2>
  typename enable_if<
    is_constructible<detail::channel_message<R(Args...)>, int, Args2...>::value,
    bool
  >::type try_send(ASIO_MOVE_ARG(Args2)... args)
  {
    typedef typename detail::channel_message<R(Args...)> message_type;
    Derived* self = static_cast<Derived*>(this);
    return self->service_->template try_send<message_type>(
        self->impl_, ASIO_MOVE_CAST(Args2)(args)...);
  }

  template <typename... Args2>
  typename enable_if<
    is_constructible<detail::channel_message<R(Args...)>, int, Args2...>::value,
    std::size_t
  >::type try_send_n(std::size_t count, ASIO_MOVE_ARG(Args2)... args)
  {
    typedef typename detail::channel_message<R(Args...)> message_type;
    Derived* self = static_cast<Derived*>(this);
    return self->service_->template try_send_n<message_type>(
        self->impl_, count, ASIO_MOVE_CAST(Args2)(args)...);
  }

  template <
      ASIO_COMPLETION_TOKEN_FOR(void (asio::error_code))
        CompletionToken ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(Executor)>
  auto async_send(Args... args,
      ASIO_MOVE_ARG(CompletionToken) token
        ASIO_DEFAULT_COMPLETION_TOKEN(Executor))
  {
    typedef typename Derived::payload_type payload_type;
    typedef typename detail::channel_message<R(Args...)> message_type;
    Derived* self = static_cast<Derived*>(this);
    return async_initiate<CompletionToken, void (asio::error_code)>(
        typename Derived::initiate_async_send(self), token,
        payload_type(message_type(0, ASIO_MOVE_CAST(Args)(args)...)));
  }
};

template <typename Derived, typename Executor,
    typename R, typename... Args, typename... Signatures>
class channel_send_functions<Derived, Executor, R(Args...), Signatures...> :
  public channel_send_functions<Derived, Executor, Signatures...>
{
public:
  using channel_send_functions<Derived, Executor, Signatures...>::try_send;
  using channel_send_functions<Derived, Executor, Signatures...>::async_send;

  template <typename... Args2>
  typename enable_if<
    is_constructible<detail::channel_message<R(Args...)>, int, Args2...>::value,
    bool
  >::type try_send(ASIO_MOVE_ARG(Args2)... args)
  {
    typedef typename detail::channel_message<R(Args...)> message_type;
    Derived* self = static_cast<Derived*>(this);
    return self->service_->template try_send<message_type>(
        self->impl_, ASIO_MOVE_CAST(Args2)(args)...);
  }

  template <typename... Args2>
  typename enable_if<
    is_constructible<detail::channel_message<R(Args...)>, int, Args2...>::value,
    std::size_t
  >::type try_send_n(std::size_t count, ASIO_MOVE_ARG(Args2)... args)
  {
    typedef typename detail::channel_message<R(Args...)> message_type;
    Derived* self = static_cast<Derived*>(this);
    return self->service_->template try_send_n<message_type>(
        self->impl_, count, ASIO_MOVE_CAST(Args2)(args)...);
  }

  template <
      ASIO_COMPLETION_TOKEN_FOR(void (asio::error_code))
        CompletionToken ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(Executor)>
  auto async_send(Args... args,
      ASIO_MOVE_ARG(CompletionToken) token
        ASIO_DEFAULT_COMPLETION_TOKEN(Executor))
  {
    typedef typename Derived::payload_type payload_type;
    typedef typename detail::channel_message<R(Args...)> message_type;
    Derived* self = static_cast<Derived*>(this);
    return async_initiate<CompletionToken, void (asio::error_code)>(
        typename Derived::initiate_async_send(self), token,
        payload_type(message_type(0, ASIO_MOVE_CAST(Args)(args)...)));
  }
};

} // namespace detail
} // namespace experimental
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_EXPERIMENTAL_DETAIL_CHANNEL_SEND_FUNCTIONS_HPP
