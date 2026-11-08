//
// cancel_after.hpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_CANCEL_AFTER_HPP
#define ASIO_CANCEL_AFTER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/basic_waitable_timer.hpp"
#include "asio/cancellation_type.hpp"
#include "asio/detail/chrono.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/wait_traits.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN

/// A @ref completion_token adapter that cancels an operation after a timeout.
/**
 * The cancel_after_t class is used to indicate that an asynchronous operation
 * should be cancelled if not complete before the specified duration has
 * elapsed.
 *
 * @sa @ref overview_token_adapters "Completion token adapters"
 */
template <typename CompletionToken, typename Clock,
    typename WaitTraits = asio::wait_traits<Clock>>
class cancel_after_t
{
public:
  /// Constructor.
  template <typename T>
  cancel_after_t(T&& completion_token, const typename Clock::duration& timeout,
      cancellation_type_t cancel_type = cancellation_type::terminal)
    : token_(static_cast<T&&>(completion_token)),
      timeout_(timeout),
      cancel_type_(cancel_type)
  {
  }

//private:
  CompletionToken token_;
  typename Clock::duration timeout_;
  cancellation_type_t cancel_type_;
};

/// A @ref completion_token adapter that cancels an operation after a timeout.
/**
 * The cancel_after_timer class is used to indicate that an asynchronous
 * operation should be cancelled if not complete before the specified duration
 * has elapsed.
 */
template <typename CompletionToken, typename Clock,
    typename WaitTraits = asio::wait_traits<Clock>,
    typename Executor = any_io_executor>
class cancel_after_timer
{
public:
  /// Constructor.
  template <typename T>
  cancel_after_timer(T&& completion_token,
      basic_waitable_timer<Clock, WaitTraits, Executor>& timer,
      const typename Clock::duration& timeout,
      cancellation_type_t cancel_type = cancellation_type::terminal)
    : token_(static_cast<T&&>(completion_token)),
      timer_(timer),
      timeout_(timeout),
      cancel_type_(cancel_type)
  {
  }

//private:
  CompletionToken token_;
  basic_waitable_timer<Clock, WaitTraits, Executor>& timer_;
  typename Clock::duration timeout_;
  cancellation_type_t cancel_type_;
};

/// A @ref completion_token adapter that cancels an operation after another
/// "trigger" asynchronous operation completes.
/**
 * The cancel_after_trigger class is used to indicate that an asynchronous
 * operation should be cancelled if not complete before the specified trigger
 * operation has completed.
 */
template <typename CompletionToken, typename Trigger>
class cancel_after_trigger
{
public:
  /// Constructor.
  template <typename T, typename U>
  cancel_after_trigger(T&& completion_token, U&& trigger,
      cancellation_type_t cancel_type = cancellation_type::terminal)
    : token_(static_cast<T&&>(completion_token)),
      trigger_(static_cast<U&&>(trigger)),
      cancel_type_(cancel_type)
  {
  }

//private:
  CompletionToken token_;
  Trigger trigger_;
  cancellation_type_t cancel_type_;
};

/// A function object type that adapts a @ref completion_token to cancel an
/// operation after a timeout.
/**
 * May also be used directly as a completion token, in which case it adapts the
 * asynchronous operation's default completion token (or asio::deferred
 * if no default is available).
 */
template <typename Clock, typename WaitTraits = asio::wait_traits<Clock>>
class partial_cancel_after
{
public:
  /// Constructor that specifies the timeout duration and cancellation type.
  explicit partial_cancel_after(const typename Clock::duration& timeout,
      cancellation_type_t cancel_type = cancellation_type::terminal)
    : timeout_(timeout),
      cancel_type_(cancel_type)
  {
  }

  /// Adapt a @ref completion_token to specify that the asynchronous operation
  /// should be cancelled after a timeout.
  template <typename CompletionToken>
  ASIO_NODISCARD inline
  cancel_after_t<decay_t<CompletionToken>, Clock, WaitTraits>
  operator()(CompletionToken&& completion_token) const
  {
    return cancel_after_t<decay_t<CompletionToken>, Clock, WaitTraits>(
        static_cast<CompletionToken&&>(completion_token),
        timeout_, cancel_type_);
  }

//private:
  typename Clock::duration timeout_;
  cancellation_type_t cancel_type_;
};

/// A function object type that adapts a @ref completion_token to cancel an
/// operation after a timeout.
/**
 * May also be used directly as a completion token, in which case it adapts the
 * asynchronous operation's default completion token (or asio::deferred
 * if no default is available).
 */
template <typename Clock, typename WaitTraits = asio::wait_traits<Clock>,
    typename Executor = any_io_executor>
class partial_cancel_after_timer
{
public:
  /// Constructor that specifies the timeout duration and cancellation type.
  explicit partial_cancel_after_timer(
      basic_waitable_timer<Clock, WaitTraits, Executor>& timer,
      const typename Clock::duration& timeout,
      cancellation_type_t cancel_type = cancellation_type::terminal)
    : timer_(timer),
      timeout_(timeout),
      cancel_type_(cancel_type)
  {
  }

  /// Adapt a @ref completion_token to specify that the asynchronous operation
  /// should be cancelled after a timeout.
  template <typename CompletionToken>
  ASIO_NODISCARD inline
  cancel_after_timer<decay_t<CompletionToken>, Clock, WaitTraits, Executor>
  operator()(CompletionToken&& completion_token) const
  {
    return cancel_after_timer<decay_t<CompletionToken>,
      Clock, WaitTraits, Executor>(
        static_cast<CompletionToken&&>(completion_token),
        timeout_, cancel_type_);
  }

//private:
  basic_waitable_timer<Clock, WaitTraits, Executor>& timer_;
  typename Clock::duration timeout_;
  cancellation_type_t cancel_type_;
};

/// A function object type that adapts a @ref completion_token to cancel an
/// operation after a "trigger" asynchronous operation completes.
/**
 * May also be used directly as a completion token, in which case it adapts the
 * asynchronous operation's default completion token (or asio::deferred
 * if no default is available).
 */
template <typename Trigger>
class partial_cancel_after_trigger
{
public:
  /// Constructor that specifies the trigger and cancellation type.
  template <typename T>
  explicit partial_cancel_after_trigger(T&& trigger,
      cancellation_type_t cancel_type = cancellation_type::terminal)
    : trigger_(static_cast<T&&>(trigger)),
      cancel_type_(cancel_type)
  {
  }

  /// Adapt a @ref completion_token to cancel the operation if not complete
  /// before the trigger operation completes.
  template <typename CompletionToken>
  ASIO_NODISCARD inline
  cancel_after_trigger<decay_t<CompletionToken>, Trigger>
  operator()(CompletionToken&& completion_token) &&
  {
    return cancel_after_trigger<decay_t<CompletionToken>, Trigger>(
        static_cast<CompletionToken&&>(completion_token),
        static_cast<Trigger&&>(trigger_), cancel_type_);
  }

  /// Adapt a @ref completion_token to cancel the operation if not complete
  /// before the trigger operation completes.
  template <typename CompletionToken>
  ASIO_NODISCARD inline
  cancel_after_trigger<decay_t<CompletionToken>, Trigger>
  operator()(CompletionToken&& completion_token) const &
  {
    return cancel_after_trigger<decay_t<CompletionToken>, Trigger>(
        static_cast<CompletionToken&&>(completion_token), trigger_,
        cancel_type_);
  }

//private:
  Trigger trigger_;
  cancellation_type_t cancel_type_;
};

/// Create a partial completion token adapter that cancels an operation if not
/// complete before the specified relative timeout has elapsed.
/**
 * @par Thread Safety
 * When an asynchronous operation is used with cancel_after, a timer async_wait
 * operation is performed in parallel to the main operation. If this parallel
 * async_wait completes first, a cancellation request is emitted to cancel the
 * main operation. Consequently, the application must ensure that the
 * asynchronous operation is performed within an implicit or explicit strand.
 */
template <typename Rep, typename Period>
ASIO_NODISCARD inline partial_cancel_after<chrono::steady_clock>
cancel_after(const chrono::duration<Rep, Period>& timeout,
    cancellation_type_t cancel_type = cancellation_type::terminal)
{
  return partial_cancel_after<chrono::steady_clock>(timeout, cancel_type);
}

/// Create a partial completion token adapter that cancels an operation if not
/// complete before the specified relative timeout has elapsed.
/**
 * @par Thread Safety
 * When an asynchronous operation is used with cancel_after, a timer async_wait
 * operation is performed in parallel to the main operation. If this parallel
 * async_wait completes first, a cancellation request is emitted to cancel the
 * main operation. Consequently, the application must ensure that the
 * asynchronous operation is performed within an implicit or explicit strand.
 */
template <typename Clock, typename WaitTraits,
    typename Executor, typename Rep, typename Period>
ASIO_NODISCARD inline
partial_cancel_after_timer<Clock, WaitTraits, Executor>
cancel_after(basic_waitable_timer<Clock, WaitTraits, Executor>& timer,
    const chrono::duration<Rep, Period>& timeout,
    cancellation_type_t cancel_type = cancellation_type::terminal)
{
  return partial_cancel_after_timer<Clock, WaitTraits, Executor>(
      timer, timeout, cancel_type);
}

/// Create a partial completion token adapter that cancels an operation if not
/// complete before the specified asynchronous operation completes.
/**
 * @par Thread Safety
 * When an asynchronous operation is used with cancel_after, an additional
 * asynchronous operation is performed in parallel to the main operation. If
 * this parallel operation completes first, a cancellation request is emitted
 * to cancel the main operation. Consequently, the application must ensure that
 * the asynchronous operation is performed within an implicit or explicit
 * strand.
 */
template <typename Trigger>
ASIO_NODISCARD inline
constraint_t<
    is_async_operation<Trigger>::value,
    partial_cancel_after_trigger<decay_t<Trigger>>
  >
cancel_after(Trigger&& trigger,
    cancellation_type_t cancel_type = cancellation_type::terminal)
{
  return partial_cancel_after_trigger<decay_t<Trigger>>(
      static_cast<Trigger&&>(trigger), cancel_type);
}

/// Adapt a @ref completion_token to cancel an operation if not complete before
/// the specified relative timeout has elapsed.
/**
 * @par Thread Safety
 * When an asynchronous operation is used with cancel_after, a timer async_wait
 * operation is performed in parallel to the main operation. If this parallel
 * async_wait completes first, a cancellation request is emitted to cancel the
 * main operation. Consequently, the application must ensure that the
 * asynchronous operation is performed within an implicit or explicit strand.
 */
template <typename Rep, typename Period, typename CompletionToken>
ASIO_NODISCARD inline
cancel_after_t<decay_t<CompletionToken>, chrono::steady_clock>
cancel_after(const chrono::duration<Rep, Period>& timeout,
    CompletionToken&& completion_token)
{
  return cancel_after_t<decay_t<CompletionToken>, chrono::steady_clock>(
      static_cast<CompletionToken&&>(completion_token),
      timeout, cancellation_type::terminal);
}

/// Adapt a @ref completion_token to cancel an operation if not complete before
/// the specified relative timeout has elapsed.
/**
 * @par Thread Safety
 * When an asynchronous operation is used with cancel_after, a timer async_wait
 * operation is performed in parallel to the main operation. If this parallel
 * async_wait completes first, a cancellation request is emitted to cancel the
 * main operation. Consequently, the application must ensure that the
 * asynchronous operation is performed within an implicit or explicit strand.
 */
template <typename Rep, typename Period, typename CompletionToken>
ASIO_NODISCARD inline
cancel_after_t<decay_t<CompletionToken>, chrono::steady_clock>
cancel_after(const chrono::duration<Rep, Period>& timeout,
    cancellation_type_t cancel_type, CompletionToken&& completion_token)
{
  return cancel_after_t<decay_t<CompletionToken>, chrono::steady_clock>(
      static_cast<CompletionToken&&>(completion_token), timeout, cancel_type);
}

/// Adapt a @ref completion_token to cancel an operation if not complete before
/// the specified relative timeout has elapsed.
/**
 * @par Thread Safety
 * When an asynchronous operation is used with cancel_after, a timer async_wait
 * operation is performed in parallel to the main operation. If this parallel
 * async_wait completes first, a cancellation request is emitted to cancel the
 * main operation. Consequently, the application must ensure that the
 * asynchronous operation is performed within an implicit or explicit strand.
 */
template <typename Clock, typename WaitTraits, typename Executor,
    typename Rep, typename Period, typename CompletionToken>
ASIO_NODISCARD inline
cancel_after_timer<decay_t<CompletionToken>, Clock, WaitTraits, Executor>
cancel_after(basic_waitable_timer<Clock, WaitTraits, Executor>& timer,
    const chrono::duration<Rep, Period>& timeout,
    CompletionToken&& completion_token)
{
  return cancel_after_timer<decay_t<CompletionToken>,
    Clock, WaitTraits, Executor>(
      static_cast<CompletionToken&&>(completion_token),
      timer, timeout, cancellation_type::terminal);
}

/// Adapt a @ref completion_token to cancel an operation if not complete before
/// the specified relative timeout has elapsed.
/**
 * @par Thread Safety
 * When an asynchronous operation is used with cancel_after, a timer async_wait
 * operation is performed in parallel to the main operation. If this parallel
 * async_wait completes first, a cancellation request is emitted to cancel the
 * main operation. Consequently, the application must ensure that the
 * asynchronous operation is performed within an implicit or explicit strand.
 */
template <typename Clock, typename WaitTraits, typename Executor,
    typename Rep, typename Period, typename CompletionToken>
ASIO_NODISCARD inline
cancel_after_timer<decay_t<CompletionToken>, Clock, WaitTraits, Executor>
cancel_after(basic_waitable_timer<Clock, WaitTraits, Executor>& timer,
    const chrono::duration<Rep, Period>& timeout,
    cancellation_type_t cancel_type, CompletionToken&& completion_token)
{
  return cancel_after_timer<decay_t<CompletionToken>,
    Clock, WaitTraits, Executor>(
      static_cast<CompletionToken&&>(completion_token),
      timer, timeout, cancel_type);
}

/// Adapt a @ref completion_token to cancel an operation if not complete before
/// the specified asynchronous operation completes.
/**
 * @par Thread Safety
 * When an asynchronous operation is used with cancel_after, an additional
 * asynchronous operation is performed in parallel to the main operation. If
 * this parallel operation completes first, a cancellation request is emitted
 * to cancel the main operation. Consequently, the application must ensure that
 * the asynchronous operation is performed within an implicit or explicit
 * strand.
 */
template <typename Trigger, typename CompletionToken>
ASIO_NODISCARD inline
constraint_t<
    is_async_operation<Trigger>::value,
    cancel_after_trigger<decay_t<CompletionToken>, decay_t<Trigger>>
  >
cancel_after(Trigger&& trigger, CompletionToken&& completion_token)
{
  return cancel_after_trigger<decay_t<CompletionToken>, decay_t<Trigger>>(
      static_cast<CompletionToken&&>(completion_token),
      static_cast<Trigger&&>(trigger), cancellation_type::terminal);
}

/// Adapt a @ref completion_token to cancel an operation if not complete before
/// the specified asynchronous operation completes.
/**
 * @par Thread Safety
 * When an asynchronous operation is used with cancel_after, an additional
 * asynchronous operation is performed in parallel to the main operation. If
 * this parallel operation completes first, a cancellation request is emitted
 * to cancel the main operation. Consequently, the application must ensure that
 * the asynchronous operation is performed within an implicit or explicit
 * strand.
 */
template <typename Trigger, typename CompletionToken>
ASIO_NODISCARD inline
constraint_t<
    is_async_operation<Trigger>::value,
    cancel_after_trigger<decay_t<CompletionToken>, decay_t<Trigger>>
  >
cancel_after(Trigger&& trigger, cancellation_type_t cancel_type,
    CompletionToken&& completion_token)
{
  return cancel_after_trigger<decay_t<CompletionToken>, decay_t<Trigger>>(
      static_cast<CompletionToken&&>(completion_token),
      static_cast<Trigger&&>(trigger), cancel_type);
}

ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#include "asio/impl/cancel_after.hpp"

#endif // ASIO_CANCEL_AFTER_HPP
