//
// executor_work_guard.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2022 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_EXECUTOR_WORK_GUARD_HPP
#define ASIO_EXECUTOR_WORK_GUARD_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#include "asio/associated_executor.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/execution.hpp"
#include "asio/is_executor.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {

#if !defined(ASIO_EXECUTOR_WORK_GUARD_DECL)
#define ASIO_EXECUTOR_WORK_GUARD_DECL

template <typename Executor, typename = void, typename = void>
class executor_work_guard;

#endif // !defined(ASIO_EXECUTOR_WORK_GUARD_DECL)

#if defined(GENERATING_DOCUMENTATION)

/// An object of type @c executor_work_guard controls ownership of executor work
/// within a scope.
template <typename Executor>
class executor_work_guard
{
public:
  /// The underlying executor type.
  typedef Executor executor_type;

  /// Constructs a @c executor_work_guard object for the specified executor.
  /**
   * Stores a copy of @c e and calls <tt>on_work_started()</tt> on it.
   */
  explicit executor_work_guard(const executor_type& e) ASIO_NOEXCEPT;

  /// Copy constructor.
  executor_work_guard(const executor_work_guard& other) ASIO_NOEXCEPT;

  /// Move constructor.
  executor_work_guard(executor_work_guard&& other) ASIO_NOEXCEPT;

  /// Destructor.
  /**
   * Unless the object has already been reset, or is in a moved-from state,
   * calls <tt>on_work_finished()</tt> on the stored executor.
   */
  ~executor_work_guard();

  /// Obtain the associated executor.
  executor_type get_executor() const ASIO_NOEXCEPT;

  /// Whether the executor_work_guard object owns some outstanding work.
  bool owns_work() const ASIO_NOEXCEPT;

  /// Indicate that the work is no longer outstanding.
  /**
   * Unless the object has already been reset, or is in a moved-from state,
   * calls <tt>on_work_finished()</tt> on the stored executor.
   */
  void reset() ASIO_NOEXCEPT;
};

#endif // defined(GENERATING_DOCUMENTATION)

#if !defined(GENERATING_DOCUMENTATION)

#if !defined(ASIO_NO_TS_EXECUTORS)

template <typename Executor>
class executor_work_guard<Executor,
    typename enable_if<
      is_executor<Executor>::value
    >::type>
{
public:
  typedef Executor executor_type;

  explicit executor_work_guard(const executor_type& e) ASIO_NOEXCEPT
    : executor_(e),
      owns_(true)
  {
    executor_.on_work_started();
  }

  executor_work_guard(const executor_work_guard& other) ASIO_NOEXCEPT
    : executor_(other.executor_),
      owns_(other.owns_)
  {
    if (owns_)
      executor_.on_work_started();
  }

#if defined(ASIO_HAS_MOVE)
  executor_work_guard(executor_work_guard&& other) ASIO_NOEXCEPT
    : executor_(ASIO_MOVE_CAST(Executor)(other.executor_)),
      owns_(other.owns_)
  {
    other.owns_ = false;
  }
#endif // defined(ASIO_HAS_MOVE)

  ~executor_work_guard()
  {
    if (owns_)
      executor_.on_work_finished();
  }

  executor_type get_executor() const ASIO_NOEXCEPT
  {
    return executor_;
  }

  bool owns_work() const ASIO_NOEXCEPT
  {
    return owns_;
  }

  void reset() ASIO_NOEXCEPT
  {
    if (owns_)
    {
      executor_.on_work_finished();
      owns_ = false;
    }
  }

private:
  // Disallow assignment.
  executor_work_guard& operator=(const executor_work_guard&);

  executor_type executor_;
  bool owns_;
};

#endif // !defined(ASIO_NO_TS_EXECUTORS)

template <typename Executor>
class executor_work_guard<Executor,
    typename enable_if<
      !is_executor<Executor>::value
    >::type,
    typename enable_if<
      execution::is_executor<Executor>::value
    >::type>
{
public:
  typedef Executor executor_type;

  explicit executor_work_guard(const executor_type& e) ASIO_NOEXCEPT
    : executor_(e),
      owns_(true)
  {
    new (&work_) work_type(asio::prefer(executor_,
          execution::outstanding_work.tracked));
  }

  executor_work_guard(const executor_work_guard& other) ASIO_NOEXCEPT
    : executor_(other.executor_),
      owns_(other.owns_)
  {
    if (owns_)
    {
      new (&work_) work_type(asio::prefer(executor_,
            execution::outstanding_work.tracked));
    }
  }

#if defined(ASIO_HAS_MOVE)
  executor_work_guard(executor_work_guard&& other) ASIO_NOEXCEPT
    : executor_(ASIO_MOVE_CAST(Executor)(other.executor_)),
      owns_(other.owns_)
  {
    if (owns_)
    {
      new (&work_) work_type(
          ASIO_MOVE_CAST(work_type)(
            *static_cast<work_type*>(
              static_cast<void*>(&other.work_))));
      other.owns_ = false;
    }
  }
#endif //  defined(ASIO_HAS_MOVE)

  ~executor_work_guard()
  {
    if (owns_)
      static_cast<work_type*>(static_cast<void*>(&work_))->~work_type();
  }

  executor_type get_executor() const ASIO_NOEXCEPT
  {
    return executor_;
  }

  bool owns_work() const ASIO_NOEXCEPT
  {
    return owns_;
  }

  void reset() ASIO_NOEXCEPT
  {
    if (owns_)
    {
      static_cast<work_type*>(static_cast<void*>(&work_))->~work_type();
      owns_ = false;
    }
  }

private:
  // Disallow assignment.
  executor_work_guard& operator=(const executor_work_guard&);

  typedef typename decay<
      typename prefer_result<
        const executor_type&,
        execution::outstanding_work_t::tracked_t
      >::type
    >::type work_type;

  executor_type executor_;
  typename aligned_storage<sizeof(work_type),
      alignment_of<work_type>::value>::type work_;
  bool owns_;
};

#endif // !defined(GENERATING_DOCUMENTATION)

/// Create an @ref executor_work_guard object.
template <typename Executor>
ASIO_NODISCARD inline executor_work_guard<Executor>
make_work_guard(const Executor& ex,
    typename constraint<
      is_executor<Executor>::value || execution::is_executor<Executor>::value
    >::type = 0)
{
  return executor_work_guard<Executor>(ex);
}

/// Create an @ref executor_work_guard object.
template <typename ExecutionContext>
ASIO_NODISCARD inline
executor_work_guard<typename ExecutionContext::executor_type>
make_work_guard(ExecutionContext& ctx,
    typename constraint<
      is_convertible<ExecutionContext&, execution_context&>::value
    >::type = 0)
{
  return executor_work_guard<typename ExecutionContext::executor_type>(
      ctx.get_executor());
}

/// Create an @ref executor_work_guard object.
template <typename T>
ASIO_NODISCARD inline
executor_work_guard<typename associated_executor<T>::type>
make_work_guard(const T& t,
    typename constraint<
      !is_executor<T>::value
    >::type = 0,
    typename constraint<
      !execution::is_executor<T>::value
    >::type = 0,
    typename constraint<
      !is_convertible<T&, execution_context&>::value
    >::type = 0)
{
  return executor_work_guard<typename associated_executor<T>::type>(
      associated_executor<T>::get(t));
}

/// Create an @ref executor_work_guard object.
template <typename T, typename Executor>
ASIO_NODISCARD inline
executor_work_guard<typename associated_executor<T, Executor>::type>
make_work_guard(const T& t, const Executor& ex,
    typename constraint<
      is_executor<Executor>::value || execution::is_executor<Executor>::value
    >::type = 0)
{
  return executor_work_guard<typename associated_executor<T, Executor>::type>(
      associated_executor<T, Executor>::get(t, ex));
}

/// Create an @ref executor_work_guard object.
template <typename T, typename ExecutionContext>
ASIO_NODISCARD inline executor_work_guard<typename associated_executor<T,
  typename ExecutionContext::executor_type>::type>
make_work_guard(const T& t, ExecutionContext& ctx,
    typename constraint<
      !is_executor<T>::value
    >::type = 0,
    typename constraint<
      !execution::is_executor<T>::value
    >::type = 0,
    typename constraint<
      !is_convertible<T&, execution_context&>::value
    >::type = 0,
    typename constraint<
      is_convertible<ExecutionContext&, execution_context&>::value
    >::type = 0)
{
  return executor_work_guard<typename associated_executor<T,
    typename ExecutionContext::executor_type>::type>(
      associated_executor<T, typename ExecutionContext::executor_type>::get(
        t, ctx.get_executor()));
}

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_EXECUTOR_WORK_GUARD_HPP
