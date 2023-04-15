//
// experimental/detail/channel_handler.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2023 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_EXPERIMENTAL_DETAIL_CHANNEL_HANDLER_HPP
#define ASIO_EXPERIMENTAL_DETAIL_CHANNEL_HANDLER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/associator.hpp"
#include "asio/experimental/detail/channel_payload.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace experimental {
namespace detail {

template <typename Payload, typename Handler>
class channel_handler
{
public:
  channel_handler(ASIO_MOVE_ARG(Payload) p, Handler& h)
    : payload_(ASIO_MOVE_CAST(Payload)(p)),
      handler_(ASIO_MOVE_CAST(Handler)(h))
  {
  }

  void operator()()
  {
    payload_.receive(handler_);
  }

//private:
  Payload payload_;
  Handler handler_;
};

} // namespace detail
} // namespace experimental

template <template <typename, typename> class Associator,
    typename Payload, typename Handler, typename DefaultCandidate>
struct associator<Associator,
    experimental::detail::channel_handler<Payload, Handler>,
    DefaultCandidate>
  : Associator<Handler, DefaultCandidate>
{
  static typename Associator<Handler, DefaultCandidate>::type
  get(const experimental::detail::channel_handler<Payload, Handler>& h)
    ASIO_NOEXCEPT
  {
    return Associator<Handler, DefaultCandidate>::get(h.handler_);
  }

  static ASIO_AUTO_RETURN_TYPE_PREFIX2(
      typename Associator<Handler, DefaultCandidate>::type)
  get(const experimental::detail::channel_handler<Payload, Handler>& h,
      const DefaultCandidate& c) ASIO_NOEXCEPT
    ASIO_AUTO_RETURN_TYPE_SUFFIX((
      Associator<Handler, DefaultCandidate>::get(h.handler_, c)))
  {
    return Associator<Handler, DefaultCandidate>::get(h.handler_, c);
  }
};

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_EXPERIMENTAL_DETAIL_CHANNEL_HANDLER_HPP
