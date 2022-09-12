//
// prepend.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2003-2022 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_PREPEND_HPP
#define ASIO_PREPEND_HPP

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
/// arguments should be passed additional values before the results of the
/// operation.
template <typename CompletionToken, typename... Values>
class prepend_t
{
public:
  /// Constructor.
  template <typename T, typename... V>
  ASIO_CONSTEXPR explicit prepend_t(
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
/// arguments should be passed additional values before the results of the
/// operation.
template <typename CompletionToken, typename... Values>
ASIO_NODISCARD inline ASIO_CONSTEXPR prepend_t<
  typename decay<CompletionToken>::type, typename decay<Values>::type...>
prepend(ASIO_MOVE_ARG(CompletionToken) completion_token,
    ASIO_MOVE_ARG(Values)... values)
{
  return prepend_t<
    typename decay<CompletionToken>::type, typename decay<Values>::type...>(
      ASIO_MOVE_CAST(CompletionToken)(completion_token),
      ASIO_MOVE_CAST(Values)(values)...);
}

} // namespace asio

#include "asio/detail/pop_options.hpp"

#include "asio/impl/prepend.hpp"

#endif // (defined(ASIO_HAS_STD_TUPLE)
       //     && defined(ASIO_HAS_VARIADIC_TEMPLATES))
       //   || defined(GENERATING_DOCUMENTATION)

#endif // ASIO_PREPEND_HPP
