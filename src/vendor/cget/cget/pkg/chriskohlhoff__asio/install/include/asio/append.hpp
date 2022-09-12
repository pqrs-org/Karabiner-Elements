//
// append.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2022 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_APPEND_HPP
#define ASIO_APPEND_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if (defined(ASIO_HAS_STD_TUPLE) \
    && defined(ASIO_HAS_VARIADIC_TEMPLATES)) \
  || defined(GENERATING_DOCUMENTATION)

#include <tuple>
#include "asio/detail/type_traits.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {

/// Completion token type used to specify that the completion handler
/// arguments should be passed additional values after the results of the
/// operation.
template <typename CompletionToken, typename... Values>
class append_t
{
public:
  /// Constructor.
  template <typename T, typename... V>
  ASIO_CONSTEXPR explicit append_t(
      ASIO_MOVE_ARG(T) completion_token,
      ASIO_MOVE_ARG(V)... values)
    : token_(ASIO_MOVE_CAST(T)(completion_token)),
      values_(ASIO_MOVE_CAST(V)(values)...)
  {
  }

//private:
  CompletionToken token_;
  std::tuple<Values...> values_;
};

/// Completion token type used to specify that the completion handler
/// arguments should be passed additional values after the results of the
/// operation.
template <typename CompletionToken, typename... Values>
ASIO_NODISCARD inline ASIO_CONSTEXPR append_t<
  typename decay<CompletionToken>::type, typename decay<Values>::type...>
append(ASIO_MOVE_ARG(CompletionToken) completion_token,
    ASIO_MOVE_ARG(Values)... values)
{
  return append_t<
    typename decay<CompletionToken>::type, typename decay<Values>::type...>(
      ASIO_MOVE_CAST(CompletionToken)(completion_token),
      ASIO_MOVE_CAST(Values)(values)...);
}

} // namespace asio

#include "asio/detail/pop_options.hpp"

#include "asio/impl/append.hpp"

#endif // (defined(ASIO_HAS_STD_TUPLE)
       //     && defined(ASIO_HAS_VARIADIC_TEMPLATES))
       //   || defined(GENERATING_DOCUMENTATION)

#endif // ASIO_APPEND_HPP
