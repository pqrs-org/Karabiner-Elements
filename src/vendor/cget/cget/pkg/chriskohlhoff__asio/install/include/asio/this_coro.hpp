//
// this_coro.hpp
// ~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_THIS_CORO_HPP
#define ASIO_THIS_CORO_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/type_traits.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace this_coro {

/// Awaitable type that returns the executor of the current coroutine.
struct executor_t
{
  ASIO_CONSTEXPR executor_t()
  {
  }
};

/// Awaitable object that returns the executor of the current coroutine.
#if defined(ASIO_HAS_CONSTEXPR) || defined(GENERATING_DOCUMENTATION)
constexpr executor_t executor;
#elif defined(ASIO_MSVC)
__declspec(selectany) executor_t executor;
#endif

/// Awaitable type that returns the cancellation state of the current coroutine.
struct cancellation_state_t
{
  ASIO_CONSTEXPR cancellation_state_t()
  {
  }
};

/// Awaitable object that returns the cancellation state of the current
/// coroutine.
#if defined(ASIO_HAS_CONSTEXPR) || defined(GENERATING_DOCUMENTATION)
constexpr cancellation_state_t cancellation_state;
#elif defined(ASIO_MSVC)
__declspec(selectany) cancellation_state_t cancellation_state;
#endif

#if defined(GENERATING_DOCUMENTATION)

/// Returns an awaitable object that may be used to reset the cancellation state
/// of the current coroutine.
ASIO_NODISCARD ASIO_CONSTEXPR unspecified
reset_cancellation_state();

/// Returns an awaitable object that may be used to reset the cancellation state
/// of the current coroutine.
template <typename Filter>
ASIO_NODISCARD ASIO_CONSTEXPR unspecified
reset_cancellation_state(ASIO_MOVE_ARG(Filter) filter);

/// Returns an awaitable object that may be used to reset the cancellation state
/// of the current coroutine.
template <typename InFilter, typename OutFilter>
ASIO_NODISCARD ASIO_CONSTEXPR unspecified
reset_cancellation_state(
    ASIO_MOVE_ARG(InFilter) in_filter,
    ASIO_MOVE_ARG(OutFilter) out_filter);

/// Returns an awaitable object that may be used to determine whether the
/// coroutine throws if trying to suspend when it has been cancelled.
ASIO_NODISCARD ASIO_CONSTEXPR unspecified
throw_if_cancelled();

/// Returns an awaitable object that may be used to specify whether the
/// coroutine throws if trying to suspend when it has been cancelled.
ASIO_NODISCARD ASIO_CONSTEXPR unspecified
throw_if_cancelled(bool value);

#else // defined(GENERATING_DOCUMENTATION)

struct reset_cancellation_state_0_t
{
  ASIO_CONSTEXPR reset_cancellation_state_0_t()
  {
  }
};

ASIO_NODISCARD inline ASIO_CONSTEXPR reset_cancellation_state_0_t
reset_cancellation_state()
{
  return reset_cancellation_state_0_t();
}

template <typename Filter>
struct reset_cancellation_state_1_t
{
  template <typename F>
  ASIO_CONSTEXPR reset_cancellation_state_1_t(
      ASIO_MOVE_ARG(F) filter)
    : filter(ASIO_MOVE_CAST(F)(filter))
  {
  }

  Filter filter;
};

template <typename Filter>
ASIO_NODISCARD inline ASIO_CONSTEXPR reset_cancellation_state_1_t<
    typename decay<Filter>::type>
reset_cancellation_state(ASIO_MOVE_ARG(Filter) filter)
{
  return reset_cancellation_state_1_t<typename decay<Filter>::type>(
      ASIO_MOVE_CAST(Filter)(filter));
}

template <typename InFilter, typename OutFilter>
struct reset_cancellation_state_2_t
{
  template <typename F1, typename F2>
  ASIO_CONSTEXPR reset_cancellation_state_2_t(
      ASIO_MOVE_ARG(F1) in_filter, ASIO_MOVE_ARG(F2) out_filter)
    : in_filter(ASIO_MOVE_CAST(F1)(in_filter)),
      out_filter(ASIO_MOVE_CAST(F2)(out_filter))
  {
  }

  InFilter in_filter;
  OutFilter out_filter;
};

template <typename InFilter, typename OutFilter>
ASIO_NODISCARD inline ASIO_CONSTEXPR reset_cancellation_state_2_t<
    typename decay<InFilter>::type,
    typename decay<OutFilter>::type>
reset_cancellation_state(
    ASIO_MOVE_ARG(InFilter) in_filter,
    ASIO_MOVE_ARG(OutFilter) out_filter)
{
  return reset_cancellation_state_2_t<
      typename decay<InFilter>::type,
      typename decay<OutFilter>::type>(
        ASIO_MOVE_CAST(InFilter)(in_filter),
        ASIO_MOVE_CAST(OutFilter)(out_filter));
}

struct throw_if_cancelled_0_t
{
  ASIO_CONSTEXPR throw_if_cancelled_0_t()
  {
  }
};

ASIO_NODISCARD inline ASIO_CONSTEXPR throw_if_cancelled_0_t
throw_if_cancelled()
{
  return throw_if_cancelled_0_t();
}

struct throw_if_cancelled_1_t
{
  ASIO_CONSTEXPR throw_if_cancelled_1_t(bool value)
    : value(value)
  {
  }

  bool value;
};

ASIO_NODISCARD inline ASIO_CONSTEXPR throw_if_cancelled_1_t
throw_if_cancelled(bool value)
{
  return throw_if_cancelled_1_t(value);
}

#endif // defined(GENERATING_DOCUMENTATION)

} // namespace this_coro
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_THIS_CORO_HPP
