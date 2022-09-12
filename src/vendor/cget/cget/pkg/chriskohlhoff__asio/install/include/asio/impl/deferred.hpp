//
// impl/deferred.hpp
// ~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2022 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_IMPL_DEFERRED_HPP
#define ASIO_IMPL_DEFERRED_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/push_options.hpp"

namespace asio {

#if !defined(GENERATING_DOCUMENTATION)

template <typename Signature>
class async_result<deferred_t, Signature>
{
public:
  template <typename Initiation, typename... InitArgs>
  static deferred_async_operation<Signature, Initiation, InitArgs...>
  initiate(ASIO_MOVE_ARG(Initiation) initiation,
      deferred_t, ASIO_MOVE_ARG(InitArgs)... args)
  {
    return deferred_async_operation<
        Signature, Initiation, InitArgs...>(
          deferred_init_tag{},
          ASIO_MOVE_CAST(Initiation)(initiation),
          ASIO_MOVE_CAST(InitArgs)(args)...);
    }
};

template <typename Function, typename R, typename... Args>
class async_result<deferred_function<Function>, R(Args...)>
{
public:
  template <typename Initiation, typename... InitArgs>
  static auto initiate(ASIO_MOVE_ARG(Initiation) initiation,
      deferred_function<Function> token,
      ASIO_MOVE_ARG(InitArgs)... init_args)
    -> decltype(
        deferred_sequence<
          deferred_async_operation<
            R(Args...), Initiation, InitArgs...>,
          Function>(deferred_init_tag{},
            deferred_async_operation<
              R(Args...), Initiation, InitArgs...>(
                deferred_init_tag{},
                ASIO_MOVE_CAST(Initiation)(initiation),
                ASIO_MOVE_CAST(InitArgs)(init_args)...),
            ASIO_MOVE_CAST(Function)(token.function_)))
  {
    return deferred_sequence<
        deferred_async_operation<
          R(Args...), Initiation, InitArgs...>,
        Function>(deferred_init_tag{},
          deferred_async_operation<
            R(Args...), Initiation, InitArgs...>(
              deferred_init_tag{},
              ASIO_MOVE_CAST(Initiation)(initiation),
              ASIO_MOVE_CAST(InitArgs)(init_args)...),
          ASIO_MOVE_CAST(Function)(token.function_));
  }
};

template <template <typename, typename> class Associator,
    typename Handler, typename Tail, typename DefaultCandidate>
struct associator<Associator,
    detail::deferred_sequence_handler<Handler, Tail>,
    DefaultCandidate>
  : Associator<Handler, DefaultCandidate>
{
  static typename Associator<Handler, DefaultCandidate>::type get(
      const detail::deferred_sequence_handler<Handler, Tail>& h,
      const DefaultCandidate& c = DefaultCandidate()) ASIO_NOEXCEPT
  {
    return Associator<Handler, DefaultCandidate>::get(h.handler_, c);
  }
};

#endif // !defined(GENERATING_DOCUMENTATION)

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_IMPL_DEFERRED_HPP
