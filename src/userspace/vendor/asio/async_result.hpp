//
// async_result.hpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_ASYNC_RESULT_HPP
#define ASIO_ASYNC_RESULT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/handler_type.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {

/// An interface for customising the behaviour of an initiating function.
/**
 * This template may be specialised for user-defined handler types.
 */
template <typename Handler>
class async_result
{
public:
  /// The return type of the initiating function.
  typedef void type;

  /// Construct an async result from a given handler.
  /**
   * When using a specalised async_result, the constructor has an opportunity
   * to initialise some state associated with the handler, which is then
   * returned from the initiating function.
   */
  explicit async_result(Handler&)
  {
  }

  /// Obtain the value to be returned from the initiating function.
  type get()
  {
  }
};

namespace detail {

// Helper template to deduce the true type of a handler, capture a local copy
// of the handler, and then create an async_result for the handler.
template <typename Handler, typename Signature>
struct async_result_init
{
  explicit async_result_init(ASIO_MOVE_ARG(Handler) orig_handler)
    : handler(ASIO_MOVE_CAST(Handler)(orig_handler)),
      result(handler)
  {
  }

  typename handler_type<Handler, Signature>::type handler;
  async_result<typename handler_type<Handler, Signature>::type> result;
};

template <typename Handler, typename Signature>
struct async_result_type_helper
{
  typedef typename async_result<
      typename handler_type<Handler, Signature>::type
    >::type type;
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#if defined(GENERATING_DOCUMENTATION)
# define ASIO_INITFN_RESULT_TYPE(h, sig) \
  void_or_deduced
#elif defined(_MSC_VER) && (_MSC_VER < 1500)
# define ASIO_INITFN_RESULT_TYPE(h, sig) \
  typename ::asio::detail::async_result_type_helper<h, sig>::type
#else
# define ASIO_INITFN_RESULT_TYPE(h, sig) \
  typename ::asio::async_result< \
    typename ::asio::handler_type<h, sig>::type>::type
#endif

#endif // ASIO_ASYNC_RESULT_HPP
