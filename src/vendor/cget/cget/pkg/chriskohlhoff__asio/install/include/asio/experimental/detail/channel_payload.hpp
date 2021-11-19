//
// experimental/detail/channel_payload.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_EXPERIMENTAL_DETAIL_CHANNEL_PAYLOAD_HPP
#define ASIO_EXPERIMENTAL_DETAIL_CHANNEL_PAYLOAD_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include <variant>
#include "asio/detail/type_traits.hpp"
#include "asio/experimental/detail/channel_message.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace experimental {
namespace detail {

template <typename... Signatures>
class channel_payload
{
public:
  template <typename Signature>
  channel_payload(ASIO_MOVE_ARG(channel_message<Signature>) m)
    : message_(ASIO_MOVE_CAST(channel_message<Signature>)(m))
  {
  }

  template <typename Handler>
  void receive(Handler& handler)
  {
    std::visit(
        [&](auto& message)
        {
          message.receive(handler);
        }, message_);
  }

private:
  std::variant<channel_message<Signatures>...> message_;
};

template <typename R>
class channel_payload<R()>
{
public:
  explicit channel_payload(channel_message<R()>)
  {
  }

  template <typename Handler>
  void receive(Handler& handler)
  {
    ASIO_MOVE_OR_LVALUE(Handler)(handler)();
  }
};

template <typename Signature>
class channel_payload<Signature>
{
public:
  channel_payload(ASIO_MOVE_ARG(channel_message<Signature>) m)
    : message_(ASIO_MOVE_CAST(channel_message<Signature>)(m))
  {
  }

  template <typename Handler>
  void receive(Handler& handler)
  {
    message_.receive(handler);
  }

private:
  channel_message<Signature> message_;
};

} // namespace detail
} // namespace experimental
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_EXPERIMENTAL_DETAIL_CHANNEL_PAYLOAD_HPP
