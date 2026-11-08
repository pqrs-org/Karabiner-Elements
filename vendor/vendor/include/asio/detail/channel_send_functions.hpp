//
// detail/channel_send_functions.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_CHANNEL_SEND_FUNCTIONS_HPP
#define ASIO_DETAIL_CHANNEL_SEND_FUNCTIONS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/async_result.hpp"
#include "asio/detail/completion_message.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/error_code.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

template <typename Derived, typename Executor, typename... Signatures>
class channel_send_functions;

template <typename Derived, typename Executor, typename R, typename... Args>
class channel_send_functions<Derived, Executor, R(Args...)>
{
public:
  template <typename... Args2>
  enable_if_t<
    is_constructible<completion_message<R(Args...)>,
      int, Args2...>::value,
    bool
  > try_send(Args2&&... args)
  {
    typedef completion_message<R(Args...)> message_type;
    Derived* self = static_cast<Derived*>(this);
    return self->service_->template try_send<message_type>(
        self->impl_, false, static_cast<Args2&&>(args)...);
  }

  template <typename... Args2>
  enable_if_t<
    is_constructible<completion_message<R(Args...)>,
      int, Args2...>::value,
    bool
  > try_send_via_dispatch(Args2&&... args)
  {
    typedef completion_message<R(Args...)> message_type;
    Derived* self = static_cast<Derived*>(this);
    return self->service_->template try_send<message_type>(
        self->impl_, true, static_cast<Args2&&>(args)...);
  }

  template <typename... Args2>
  enable_if_t<
    is_constructible<completion_message<R(Args...)>,
      int, Args2...>::value,
    std::size_t
  > try_send_n(std::size_t count, Args2&&... args)
  {
    typedef completion_message<R(Args...)> message_type;
    Derived* self = static_cast<Derived*>(this);
    return self->service_->template try_send_n<message_type>(
        self->impl_, count, false, static_cast<Args2&&>(args)...);
  }

  template <typename... Args2>
  enable_if_t<
    is_constructible<completion_message<R(Args...)>,
      int, Args2...>::value,
    std::size_t
  > try_send_n_via_dispatch(std::size_t count, Args2&&... args)
  {
    typedef completion_message<R(Args...)> message_type;
    Derived* self = static_cast<Derived*>(this);
    return self->service_->template try_send_n<message_type>(
        self->impl_, count, true, static_cast<Args2&&>(args)...);
  }

  template <
      ASIO_COMPLETION_TOKEN_FOR(void (asio::error_code))
        CompletionToken ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(Executor)>
  auto async_send(Args... args,
      CompletionToken&& token
        ASIO_DEFAULT_COMPLETION_TOKEN(Executor))
    -> decltype(
        async_initiate<CompletionToken, void (asio::error_code)>(
          declval<typename conditional_t<false, CompletionToken,
            Derived>::initiate_async_send>(), token,
          declval<typename conditional_t<false, CompletionToken,
            Derived>::payload_type>()))
  {
    typedef typename Derived::payload_type payload_type;
    typedef completion_message<R(Args...)> message_type;
    Derived* self = static_cast<Derived*>(this);
    ASIO_INVOKE_INLINE return async_initiate<CompletionToken,
      void (asio::error_code)>(
        typename Derived::initiate_async_send(self), token,
        payload_type(message_type(0, static_cast<Args&&>(args)...)));
  }
};

template <typename Derived, typename Executor,
    typename R, typename... Args, typename... Signatures>
class channel_send_functions<Derived, Executor, R(Args...), Signatures...> :
  public channel_send_functions<Derived, Executor, Signatures...>
{
  typedef channel_send_functions<Derived, Executor, Signatures...> base_type;

public:
  using base_type::async_send;

  template <typename... Args2>
  enable_if_t<
    is_constructible<completion_message<R(Args...)>,
      int, Args2...>::value,
    bool
  > try_send(Args2&&... args)
  {
    typedef completion_message<R(Args...)> message_type;
    Derived* self = static_cast<Derived*>(this);
    return self->service_->template try_send<message_type>(
        self->impl_, false, static_cast<Args2&&>(args)...);
  }

  template <typename... Args2>
  auto try_send(Args2&&... args)
    -> enable_if_t<
        !is_constructible<
          completion_message<R(Args...)>,
          int, Args2...>::value,
        decltype(declval<base_type&>().try_send(declval<Args2>()...))>
  {
    return this->base_type::try_send(static_cast<Args2&&>(args)...);
  }

  template <typename... Args2>
  enable_if_t<
    is_constructible<completion_message<R(Args...)>,
      int, Args2...>::value,
    bool
  > try_send_via_dispatch(Args2&&... args)
  {
    typedef completion_message<R(Args...)> message_type;
    Derived* self = static_cast<Derived*>(this);
    return self->service_->template try_send<message_type>(
        self->impl_, true, static_cast<Args2&&>(args)...);
  }

  template <typename... Args2>
  auto try_send_via_dispatch(Args2&&... args)
    -> enable_if_t<
        !is_constructible<
          completion_message<R(Args...)>,
          int, Args2...>::value,
        decltype(declval<base_type&>().try_send_via_dispatch(
          declval<Args2>()...))>
  {
    return this->base_type::try_send_via_dispatch(
        static_cast<Args2&&>(args)...);
  }

  template <typename... Args2>
  enable_if_t<
    is_constructible<completion_message<R(Args...)>,
      int, Args2...>::value,
    std::size_t
  > try_send_n(std::size_t count, Args2&&... args)
  {
    typedef completion_message<R(Args...)> message_type;
    Derived* self = static_cast<Derived*>(this);
    return self->service_->template try_send_n<message_type>(
        self->impl_, count, false, static_cast<Args2&&>(args)...);
  }

  template <typename... Args2>
  auto try_send_n(std::size_t count, Args2&&... args)
    -> enable_if_t<
        !is_constructible<
          completion_message<R(Args...)>,
          int, Args2...>::value,
        decltype(declval<base_type&>().try_send_n(
          count, declval<Args2>()...))>
  {
    return this->base_type::try_send_n(count, static_cast<Args2&&>(args)...);
  }

  template <typename... Args2>
  enable_if_t<
    is_constructible<completion_message<R(Args...)>,
      int, Args2...>::value,
    std::size_t
  > try_send_n_via_dispatch(std::size_t count, Args2&&... args)
  {
    typedef completion_message<R(Args...)> message_type;
    Derived* self = static_cast<Derived*>(this);
    return self->service_->template try_send_n<message_type>(
        self->impl_, count, true, static_cast<Args2&&>(args)...);
  }

  template <typename... Args2>
  auto try_send_n_via_dispatch(std::size_t count, Args2&&... args)
    -> enable_if_t<
        !is_constructible<
          completion_message<R(Args...)>,
          int, Args2...>::value,
        decltype(declval<base_type&>().try_send_n_via_dispatch(
          count, declval<Args2>()...))>
  {
    return this->base_type::try_send_n_via_dispatch(
        count, static_cast<Args2&&>(args)...);
  }

  template <
      ASIO_COMPLETION_TOKEN_FOR(void (asio::error_code))
        CompletionToken ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(Executor)>
  auto async_send(Args... args,
      CompletionToken&& token
        ASIO_DEFAULT_COMPLETION_TOKEN(Executor))
    -> decltype(
        async_initiate<CompletionToken, void (asio::error_code)>(
          declval<typename conditional_t<false, CompletionToken,
            Derived>::initiate_async_send>(), token,
          declval<typename conditional_t<false, CompletionToken,
            Derived>::payload_type>()))
  {
    typedef typename Derived::payload_type payload_type;
    typedef completion_message<R(Args...)> message_type;
    Derived* self = static_cast<Derived*>(this);
    ASIO_INVOKE_INLINE return async_initiate<CompletionToken,
      void (asio::error_code)>(
        typename Derived::initiate_async_send(self), token,
        payload_type(message_type(0, static_cast<Args&&>(args)...)));
  }
};

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_CHANNEL_SEND_FUNCTIONS_HPP
