//
// experimental/detail/channel_message.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2023 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_EXPERIMENTAL_DETAIL_CHANNEL_MESSAGE_HPP
#define ASIO_EXPERIMENTAL_DETAIL_CHANNEL_MESSAGE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include <tuple>
#include "asio/detail/type_traits.hpp"
#include "asio/detail/utility.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace experimental {
namespace detail {

template <typename Signature>
class channel_message;

template <typename R>
class channel_message<R()>
{
public:
  channel_message(int)
  {
  }

  template <typename Handler>
  void receive(Handler& handler)
  {
    ASIO_MOVE_OR_LVALUE(Handler)(handler)();
  }
};

template <typename R, typename Arg0>
class channel_message<R(Arg0)>
{
public:
  template <typename T0>
  channel_message(int, ASIO_MOVE_ARG(T0) t0)
    : arg0_(ASIO_MOVE_CAST(T0)(t0))
  {
  }

  template <typename Handler>
  void receive(Handler& handler)
  {
    ASIO_MOVE_OR_LVALUE(Handler)(handler)(
        ASIO_MOVE_CAST(arg0_type)(arg0_));
  }

private:
  typedef typename decay<Arg0>::type arg0_type;
  arg0_type arg0_;
};

template <typename R, typename Arg0, typename Arg1>
class channel_message<R(Arg0, Arg1)>
{
public:
  template <typename T0, typename T1>
  channel_message(int, ASIO_MOVE_ARG(T0) t0, ASIO_MOVE_ARG(T1) t1)
    : arg0_(ASIO_MOVE_CAST(T0)(t0)),
      arg1_(ASIO_MOVE_CAST(T1)(t1))
  {
  }

  template <typename Handler>
  void receive(Handler& handler)
  {
    ASIO_MOVE_OR_LVALUE(Handler)(handler)(
        ASIO_MOVE_CAST(arg0_type)(arg0_),
        ASIO_MOVE_CAST(arg1_type)(arg1_));
  }

private:
  typedef typename decay<Arg0>::type arg0_type;
  arg0_type arg0_;
  typedef typename decay<Arg1>::type arg1_type;
  arg1_type arg1_;
};

template <typename R, typename... Args>
class channel_message<R(Args...)>
{
public:
  template <typename... T>
  channel_message(int, ASIO_MOVE_ARG(T)... t)
    : args_(ASIO_MOVE_CAST(T)(t)...)
  {
  }

  template <typename Handler>
  void receive(Handler& h)
  {
    this->do_receive(h, asio::detail::index_sequence_for<Args...>());
  }

private:
  template <typename Handler, std::size_t... I>
  void do_receive(Handler& h, asio::detail::index_sequence<I...>)
  {
    ASIO_MOVE_OR_LVALUE(Handler)(h)(
        std::get<I>(ASIO_MOVE_CAST(args_type)(args_))...);
  }

  typedef std::tuple<typename decay<Args>::type...> args_type;
  args_type args_;
};

} // namespace detail
} // namespace experimental
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_EXPERIMENTAL_DETAIL_CHANNEL_MESSAGE_HPP
