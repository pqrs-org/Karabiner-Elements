//
// detail/handler_work.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_HANDLER_WORK_HPP
#define ASIO_DETAIL_HANDLER_WORK_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/associated_allocator.hpp"
#include "asio/associated_executor.hpp"
#include "asio/associated_immediate_executor.hpp"
#include "asio/detail/candidate_executors.hpp"
#include "asio/detail/initiate_dispatch.hpp"
#include "asio/detail/native_executors.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/detail/work_dispatcher.hpp"
#include "asio/execution/allocator.hpp"
#include "asio/execution/any_executor.hpp"
#include "asio/execution/blocking.hpp"
#include "asio/execution/executor.hpp"
#include "asio/execution/outstanding_work.hpp"
#include "asio/execution/staging_executor.hpp"
#include "asio/executor_work_guard.hpp"
#include "asio/is_executor.hpp"
#include "asio/prefer.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN

class executor;

#if !defined(ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

class any_completion_executor;
class any_io_executor;

#endif // !defined(ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

namespace execution {

template <typename...> class any_executor;

} // namespace execution
namespace detail {

struct handler_work_ownership
{
  struct never {};
  struct maybe {};
  struct always {};
};

template <typename Executor, typename CandidateExecutor = void,
    typename PolymorphicExecutor = executor, typename = void>
class handler_work_base
{
public:
  typedef handler_work_ownership::always work_ownership;

  explicit handler_work_base(int, int, const Executor& ex) noexcept
    : executor_(asio::prefer(ex, execution::outstanding_work.tracked))
  {
  }

  template <typename OtherExecutor>
  handler_work_base(const Executor& ex,
      const OtherExecutor& /*candidate*/) noexcept
    : executor_(asio::prefer(ex, execution::outstanding_work.tracked))
  {
  }

  handler_work_base(const handler_work_base& other) noexcept
    : executor_(other.executor_)
  {
  }

  handler_work_base(handler_work_base&& other) noexcept
    : executor_(static_cast<executor_type&&>(other.executor_))
  {
  }

  bool owns_work() const noexcept
  {
    return true;
  }

  template <typename Function, typename Handler>
  void dispatch(Function& function, Handler& handler)
  {
    asio::prefer(executor_,
        execution::allocator((get_associated_allocator)(handler))
      ).execute(static_cast<Function&&>(function));
  }

private:
  typedef decay_t<
      prefer_result_t<Executor, execution::outstanding_work_t::tracked_t>
    > executor_type;

  executor_type executor_;
};

template <typename Executor, typename CandidateExecutor,
    typename PolymorphicExecutor>
class handler_work_base<Executor, CandidateExecutor, PolymorphicExecutor,
    enable_if_t<
      !execution::is_executor<Executor>::value
        && (!is_same<Executor, PolymorphicExecutor>::value
          || (!is_same<CandidateExecutor, void>::value
            && !is_executor<CandidateExecutor>::value))
    >
  >
{
public:
  typedef conditional_t<
      is_same<Executor, CandidateExecutor>::value,
      handler_work_ownership::maybe,
      handler_work_ownership::always
    > work_ownership;

  explicit handler_work_base(int, int, const Executor& ex) noexcept
    : executor_(ex),
      owns_work_(true)
  {
    executor_.on_work_started();
  }

  handler_work_base(const Executor& ex,
      const Executor& candidate) noexcept
    : executor_(ex),
      owns_work_(ex != candidate)
  {
    if (owns_work_)
      executor_.on_work_started();
  }

  template <typename OtherExecutor>
  handler_work_base(const Executor& ex,
      const OtherExecutor& /*candidate*/) noexcept
    : executor_(ex),
      owns_work_(true)
  {
    executor_.on_work_started();
  }

  handler_work_base(const handler_work_base& other) noexcept
    : executor_(other.executor_),
      owns_work_(other.owns_work_)
  {
    if (owns_work_)
      executor_.on_work_started();
  }

  handler_work_base(handler_work_base&& other) noexcept
    : executor_(static_cast<Executor&&>(other.executor_)),
      owns_work_(other.owns_work_)
  {
    other.owns_work_ = false;
  }

  ~handler_work_base()
  {
    if (owns_work_)
      executor_.on_work_finished();
  }

  bool owns_work() const noexcept
  {
    return owns_work_;
  }

  template <typename Function, typename Handler>
  void dispatch(Function& function, Handler& handler)
  {
    executor_.dispatch(static_cast<Function&&>(function),
        asio::get_associated_allocator(handler));
  }

private:
  Executor executor_;
  bool owns_work_;
};

template <typename Executor, typename PolymorphicExecutor>
class handler_work_base<Executor, void, PolymorphicExecutor,
    enable_if_t<
      is_same<
        native_execute_kind_t<Executor>,
        native_execute_kinds::native
      >::value
    >
  >
{
public:
  typedef handler_work_ownership::never work_ownership;

  explicit handler_work_base(int, int, const Executor&)
  {
  }

  bool owns_work() const noexcept
  {
    return false;
  }

  template <typename Function, typename Handler>
  void dispatch(Function& function, Handler&)
  {
    // When using a native implementation, I/O completion handlers are
    // already dispatched according to the execution context's executor's
    // rules. We can call the function directly.
    static_cast<Function&&>(function)();
  }
};

template <typename Executor, typename CandidateExecutor,
    typename PolymorphicExecutor>
class handler_work_base<Executor, CandidateExecutor, PolymorphicExecutor,
    enable_if_t<
      execution::is_executor<Executor>::value
        && is_same<
            native_execute_kind_t<Executor>,
            native_execute_kinds::none
          >::value
        && is_same<
            native_outstanding_work_kind_t<Executor>,
            native_outstanding_work_kinds::native
          >::value
    >
  >
{
public:
  typedef handler_work_ownership::always work_ownership;

  explicit handler_work_base(int, int, const Executor& ex) noexcept
    : executor_(ex)
  {
  }

  template <typename OtherExecutor>
  handler_work_base(const Executor& ex,
      const OtherExecutor& /*candidate*/) noexcept
    : executor_(ex)
  {
  }

  handler_work_base(const handler_work_base& other) noexcept
    : executor_(other.executor_)
  {
  }

  handler_work_base(handler_work_base&& other) noexcept
    : executor_(static_cast<Executor&&>(other.executor_))
  {
  }

  bool owns_work() const noexcept
  {
    return true;
  }

  template <typename Function, typename Handler>
  void dispatch(Function& function, Handler& handler)
  {
    asio::prefer(executor_,
        execution::allocator((get_associated_allocator)(handler))
      ).execute(static_cast<Function&&>(function));
  }

private:
  Executor executor_;
};

template <typename Executor, typename CandidateExecutor>
class handler_work_base<Executor, CandidateExecutor, Executor,
    enable_if_t<
      is_same<CandidateExecutor, void>::value
        || is_executor<CandidateExecutor>::value
    >
  >
{
public:
  typedef handler_work_ownership::maybe work_ownership;

  explicit handler_work_base(int, int, const Executor& ex) noexcept
#if !defined(ASIO_NO_TYPEID)
    : executor_((has_native_execute)(ex) ? Executor() : ex)
#else // !defined(ASIO_NO_TYPEID)
    : executor_(ex)
#endif // !defined(ASIO_NO_TYPEID)
  {
    if (executor_)
      executor_.on_work_started();
  }

  handler_work_base(const Executor& ex,
      const Executor& candidate) noexcept
    : executor_(ex != candidate ? ex : Executor())
  {
    if (executor_)
      executor_.on_work_started();
  }

  template <typename OtherExecutor>
  handler_work_base(const Executor& ex,
      const OtherExecutor& candidate) noexcept
    : executor_(init(ex, candidate))
  {
    if (executor_)
      executor_.on_work_started();
  }

  handler_work_base(const handler_work_base& other) noexcept
    : executor_(other.executor_)
  {
    if (executor_)
      executor_.on_work_started();
  }

  handler_work_base(handler_work_base&& other) noexcept
    : executor_(static_cast<Executor&&>(other.executor_))
  {
  }

  ~handler_work_base()
  {
    if (executor_)
      executor_.on_work_finished();
  }

  bool owns_work() const noexcept
  {
    return !!executor_;
  }

  template <typename Function, typename Handler>
  void dispatch(Function& function, Handler& handler)
  {
    executor_.dispatch(static_cast<Function&&>(function),
        asio::get_associated_allocator(handler));
  }

private:
  template <typename OtherExecutor>
  static Executor init(const Executor& ex, const OtherExecutor& candidate)
  {
    const OtherExecutor* target = ex.template target<OtherExecutor>();
    return (target && *target == candidate) ? Executor() : ex;
  }

  Executor executor_;
};

template <typename... SupportableProperties, typename CandidateExecutor,
    typename PolymorphicExecutor>
class handler_work_base<execution::any_executor<SupportableProperties...>,
    CandidateExecutor, PolymorphicExecutor>
{
public:
  typedef execution::any_executor<SupportableProperties...> executor_type;

  typedef handler_work_ownership::maybe work_ownership;

  explicit handler_work_base(int, int, const executor_type& ex) noexcept
#if !defined(ASIO_NO_TYPEID)
    : executor_(
        (has_native_execute)(ex)
          ? executor_type()
          : (has_native_outstanding_work)(ex)
            ? ex
            : handler_work_base::tracked(ex))
#else // !defined(ASIO_NO_TYPEID)
    : executor_(handler_work_base::tracked(ex))
#endif // !defined(ASIO_NO_TYPEID)
  {
  }

  handler_work_base(const executor_type& ex,
      const executor_type& candidate) noexcept
    : executor_(
        ex == candidate
          ? executor_type()
          : handler_work_base::tracked(ex))
  {
  }

  template <typename OtherExecutor>
  handler_work_base(const executor_type& ex,
      const OtherExecutor& candidate) noexcept
    : executor_(
        execution::detail::executor_targets_equal(ex, candidate)
          ? executor_type()
          : handler_work_base::tracked(ex))
  {
  }

  handler_work_base(const handler_work_base& other) noexcept
    : executor_(other.executor_)
  {
  }

  handler_work_base(handler_work_base&& other) noexcept
    : executor_(static_cast<executor_type&&>(other.executor_))
  {
  }

  bool owns_work() const noexcept
  {
    return !!executor_;
  }

  template <typename Function, typename Handler>
  void dispatch(Function& function, Handler&)
  {
    executor_.execute(static_cast<Function&&>(function));
  }

  friend handler_work_base& handler_work_release_untracked(
      handler_work_base& w) noexcept
  {
    if (target_has_native_outstanding_work(w.executor_))
      w.executor_ = nullptr;
    return w;
  }

private:
  ASIO_NEVER_INLINE static executor_type tracked(
      const executor_type& ex) noexcept
  {
    return asio::prefer(ex, execution::outstanding_work.tracked);
  }

  executor_type executor_;
};

#if !defined(ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

template <typename Executor, typename CandidateExecutor,
    typename PolymorphicExecutor>
class handler_work_base<
    Executor, CandidateExecutor, PolymorphicExecutor,
    enable_if_t<
      is_same<Executor, any_completion_executor>::value
        || is_same<Executor, any_io_executor>::value
    >
  >
{
public:
  typedef Executor executor_type;

  typedef handler_work_ownership::maybe work_ownership;

  explicit handler_work_base(int, int,
      const executor_type& ex) noexcept
#if !defined(ASIO_NO_TYPEID)
    : executor_(
        (has_native_execute)(ex)
          ? executor_type()
          : (has_native_outstanding_work)(ex)
            ? ex
            : handler_work_base::tracked(ex))
#else // !defined(ASIO_NO_TYPEID)
    : executor_(handler_work_base::tracked(ex))
#endif // !defined(ASIO_NO_TYPEID)
  {
  }

  handler_work_base(const executor_type& ex,
      const executor_type& candidate) noexcept
    : executor_(
        ex == candidate
          ? executor_type()
          : handler_work_base::tracked(ex))
  {
  }

  template <typename OtherExecutor>
  handler_work_base(const executor_type& ex,
      const OtherExecutor& candidate) noexcept
    : executor_(
        execution::detail::executor_targets_equal(ex, candidate)
          ? executor_type()
          : handler_work_base::tracked(ex))
  {
  }

  explicit handler_work_base(nullptr_t) noexcept
    : executor_()
  {
  }

  handler_work_base(const handler_work_base& other) noexcept
    : executor_(other.executor_)
  {
  }

  handler_work_base(handler_work_base&& other) noexcept
    : executor_(static_cast<executor_type&&>(other.executor_))
  {
  }

  bool owns_work() const noexcept
  {
    return !!executor_;
  }

  template <typename Function, typename Handler>
  void dispatch(Function& function, Handler&)
  {
    executor_.execute(static_cast<Function&&>(function));
  }

  friend handler_work_base& handler_work_release_untracked(
      handler_work_base& w) noexcept
  {
    if (target_has_native_outstanding_work(w.executor_))
      w.executor_ = nullptr;
    return w;
  }

private:
  ASIO_NEVER_INLINE static executor_type tracked(
      const executor_type& ex) noexcept
  {
    return asio::prefer(ex, execution::outstanding_work.tracked);
  }

  executor_type executor_;
};

#endif // !defined(ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

template <typename Base>
inline Base& handler_work_release_untracked(Base& b) noexcept
{
  return b;
}

template <typename Handler, typename IoExecutor, typename = void>
struct handler_work_can_skip_associated_executor : false_type
{
};

#if !defined(ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

template <typename Handler, typename IoExecutor>
struct handler_work_can_skip_associated_executor<Handler, IoExecutor,
    enable_if_t<
      executor_is_candidate_query<Handler, IoExecutor>::is_known
        && (is_same<associated_executor_t<Handler, IoExecutor>,
              any_completion_executor>::value
          || is_same<associated_executor_t<Handler, IoExecutor>,
              any_io_executor>::value)
    >> : true_type
{
};

#endif // !defined(ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

template <typename Handler, typename IoExecutor, typename = void>
class handler_work :
  handler_work_base<IoExecutor>,
  handler_work_base<associated_executor_t<Handler, IoExecutor>, IoExecutor>
{
public:
  typedef handler_work_base<IoExecutor> base1_type;
  typedef handler_work_base<associated_executor_t<Handler, IoExecutor>,
      IoExecutor> base2_type;

  handler_work(Handler& handler, const IoExecutor& io_ex) noexcept
    : base1_type(0, 0, io_ex),
      base2_type(handler_work::make_base2(handler, io_ex,
          handler_work_can_skip_associated_executor<Handler, IoExecutor>()))
  {
  }

  handler_work& release_untracked_io_executor() noexcept
  {
    handler_work_release_untracked(static_cast<base1_type&>(*this));
    return *this;
  }

  template <typename Function>
  void complete(Function& function, Handler& handler)
  {
    complete(function, handler,
        typename base1_type::work_ownership(),
        typename base2_type::work_ownership());
  }

  bool associated_executor_owns_work() const noexcept
  {
    return base2_type::owns_work();
  }

private:
  static base2_type make_base2(Handler& handler,
      const IoExecutor& io_ex, true_type) noexcept
  {
    if (executor_is_candidate(handler, io_ex))
      return base2_type(nullptr);
    return base2_type(
        asio::get_associated_executor(handler, io_ex), io_ex);
  }

  static base2_type make_base2(Handler& handler,
      const IoExecutor& io_ex, false_type) noexcept
  {
    return base2_type(
        asio::get_associated_executor(handler, io_ex), io_ex);
  }

  template <typename Function, typename Base1Ownership>
  void complete(Function& function, Handler& handler,
      Base1Ownership, handler_work_ownership::always)
  {
    base2_type::dispatch(function, handler);
  }

  template <typename Function>
  void complete(Function& function, Handler& handler,
      handler_work_ownership::never, handler_work_ownership::maybe)
  {
    if (base2_type::owns_work())
    {
      base2_type::dispatch(function, handler);
    }
    else
    {
      // When using a native implementation, I/O completion handlers are
      // already dispatched according to the execution context's executor's
      // rules. We can call the function directly.
      static_cast<Function&&>(function)();
    }
  }

  template <typename Function>
  void complete(Function& function, Handler& handler,
      handler_work_ownership::always, handler_work_ownership::maybe)
  {
    if (base2_type::owns_work())
    {
      base2_type::dispatch(function, handler);
    }
    else
    {
      base1_type::dispatch(function, handler);
    }
  }

  template <typename Function>
  void complete(Function& function, Handler& handler,
      handler_work_ownership::maybe, handler_work_ownership::maybe)
  {
    if (base2_type::owns_work())
    {
      base2_type::dispatch(function, handler);
    }
    else if (base1_type::owns_work())
    {
      base1_type::dispatch(function, handler);
    }
    else
    {
      // When using a native implementation, I/O completion handlers are
      // already dispatched according to the execution context's executor's
      // rules. We can call the function directly.
      static_cast<Function&&>(function)();
    }
  }
};

template <typename Handler, typename IoExecutor>
class handler_work<
    Handler, IoExecutor,
    enable_if_t<
      !has_associated_executor<Handler, IoExecutor>::value
    >
  > : handler_work_base<IoExecutor>
{
public:
  typedef handler_work_base<IoExecutor> base1_type;

  handler_work(Handler&, const IoExecutor& io_ex) noexcept
    : base1_type(0, 0, io_ex)
  {
  }

  handler_work& release_untracked_io_executor() noexcept
  {
    handler_work_release_untracked(static_cast<base1_type&>(*this));
    return *this;
  }

  template <typename Function>
  void complete(Function& function, Handler& handler)
  {
    if (!base1_type::owns_work())
    {
      // When using a native implementation, I/O completion handlers are
      // already dispatched according to the execution context's executor's
      // rules. We can call the function directly.
      static_cast<Function&&>(function)();
    }
    else
    {
      base1_type::dispatch(function, handler);
    }
  }
};

template <typename Handler, typename IoExecutor,
    typename = void, typename = void, typename = void, typename = void>
class immediate_handler_work
{
public:
  typedef handler_work<Handler, IoExecutor> handler_work_type;

  ASIO_ALWAYS_INLINE
  explicit immediate_handler_work(handler_work_type&& w)
    : handler_work_(static_cast<handler_work_type&&>(
          w.release_untracked_io_executor()))
  {
  }

  template <typename Function>
  void complete(Function& function, Handler& handler, const void* io_ex)
  {
    this->complete_impl(function, handler,
        *static_cast<const IoExecutor*>(io_ex),
        integral_constant<bool,
          !has_associated_immediate_executor<Handler, IoExecutor>::value
            && execution::is_executor<IoExecutor>::value>());
  }

private:
  template <typename Function>
  void complete_impl(Function& function, Handler& handler,
      const IoExecutor& io_ex, false_type)
  {
    if (immediate_executor_is_candidate(handler, io_ex)
        && this->io_executor_is_completion_executor(handler, io_ex))
    {
      this->complete_default(function, handler, io_ex,
          integral_constant<bool,
            execution::is_executor<IoExecutor>::value>());
    }
    else
    {
      typedef associated_immediate_executor_t<Handler, IoExecutor>
        immediate_ex_type;

      immediate_ex_type immediate_ex = (get_associated_immediate_executor)(
          handler, io_ex);

      (initiate_dispatch_with_executor<const immediate_ex_type&>(immediate_ex))(
          static_cast<Function&&>(function), empty_work_function());
    }
  }

  template <typename Function>
  void complete_default(Function& function, Handler&,
      const IoExecutor& io_ex, true_type)
  {
    typedef decay_t<
        require_result_t<
          execution::staging_executor<const IoExecutor&>,
          const execution::blocking_t::never_t&
        >
      > immediate_ex_type;

    immediate_ex_type immediate_ex = asio::require(
        execution::stage(io_ex), execution::blocking.never);

    associated_allocator_t<Function> alloc(
        (get_associated_allocator)(function));

    asio::prefer(immediate_ex, execution::allocator(alloc)).execute(
        asio::detail::bind_handler(static_cast<Function&&>(function)));
  }

  template <typename Function>
  void complete_default(Function& function, Handler& handler,
      const IoExecutor& io_ex, false_type)
  {
    typedef associated_immediate_executor_t<Handler, IoExecutor>
      immediate_ex_type;

    immediate_ex_type immediate_ex = (get_associated_immediate_executor)(
        handler, io_ex);

    (initiate_dispatch_with_executor<const immediate_ex_type&>(immediate_ex))(
        static_cast<Function&&>(function), empty_work_function());
  }

  template <typename Function>
  void complete_impl(Function& function, Handler&,
      const IoExecutor& io_ex, true_type)
  {
    typedef decay_t<
        require_result_t<
          execution::staging_executor<const IoExecutor&>,
          const execution::blocking_t::never_t&
        >
      > immediate_ex_type;

    immediate_ex_type immediate_ex = asio::require(
        execution::stage(io_ex), execution::blocking.never);

    (initiate_dispatch_with_executor<const immediate_ex_type&>(immediate_ex))(
        static_cast<Function&&>(function), empty_work_function());
  }

  bool io_executor_is_completion_executor(
      Handler& handler, const IoExecutor& io_ex)
  {
    return io_executor_is_completion_executor(handler, io_ex,
        has_associated_executor<Handler, IoExecutor>());
  }

  bool io_executor_is_completion_executor(Handler&,
      const IoExecutor&, false_type)
  {
    return true;
  }

  bool io_executor_is_completion_executor(Handler& handler,
      const IoExecutor& io_ex, true_type)
  {
    return io_executor_is_completion_executor(handler, io_ex,
        typename handler_work_type::base2_type::work_ownership());
  }

  bool io_executor_is_completion_executor(Handler&,
      const IoExecutor&, handler_work_ownership::maybe)
  {
    return !handler_work_.associated_executor_owns_work();
  }

  bool io_executor_is_completion_executor(Handler& handler,
      const IoExecutor& io_ex, handler_work_ownership::always)
  {
    return execution::detail::executor_targets_equal(
        (asio::get_associated_executor)(handler, io_ex), io_ex);
  }

  handler_work_type handler_work_;
};

template <typename Handler, typename IoExecutor>
class immediate_handler_work<Handler, IoExecutor,
    enable_if_t<
      execution::is_executor<IoExecutor>::value
    >,
    enable_if_t<
      is_same<IoExecutor, associated_executor_t<Handler, IoExecutor>>::value
        || (is_base_of<execution::detail::any_executor_base,
              IoExecutor>::value
          && is_base_of<execution::detail::any_executor_base,
              associated_executor_t<Handler, IoExecutor>>::value)
    >,
    enable_if_t<
      !has_associated_immediate_executor<Handler, IoExecutor>::value
    >,
    enable_if_t<
      has_associated_executor<Handler, IoExecutor>::value
    >
  >
{
public:
  typedef handler_work<Handler, IoExecutor> handler_work_type;

  ASIO_ALWAYS_INLINE
  explicit immediate_handler_work(handler_work_type&& w)
    : handler_work_(static_cast<handler_work_type&&>(
          w.release_untracked_io_executor()))
  {
  }

  template <typename Function>
  void complete(Function& function, Handler& handler, const void* io_ex)
  {
    this->complete_impl(function, handler,
        *static_cast<const IoExecutor*>(io_ex),
        integral_constant<bool,
          !has_associated_immediate_executor<Handler, IoExecutor>::value
            && execution::is_executor<IoExecutor>::value>());
  }

private:
  template <typename Function>
  void complete_impl(Function& function, Handler& handler,
      const IoExecutor& io_ex, false_type)
  {
    typedef associated_immediate_executor_t<Handler, IoExecutor>
      immediate_ex_type;

    immediate_ex_type immediate_ex = (get_associated_immediate_executor)(
        handler, io_ex);

    if (this->io_executor_is_completion_executor(handler, io_ex))
    {
      associated_allocator_t<Function> alloc(
          (get_associated_allocator)(function));

      asio::prefer(immediate_ex, execution::allocator(alloc)).execute(
          asio::detail::bind_handler(static_cast<Function&&>(function)));
    }
    else
    {
      (initiate_dispatch_with_executor<const immediate_ex_type&>(immediate_ex))(
          static_cast<Function&&>(function), empty_work_function());
    }
  }

  template <typename Function>
  void complete_impl(Function& function, Handler& handler,
      const IoExecutor& io_ex, true_type)
  {
    typedef decay_t<
        require_result_t<
          execution::staging_executor<const IoExecutor&>,
          const execution::blocking_t::never_t&
        >
      > immediate_ex_type;

    immediate_ex_type immediate_ex = asio::require(
        execution::stage(io_ex), execution::blocking.never);

    if (this->io_executor_is_completion_executor(handler, io_ex))
    {
      associated_allocator_t<Function> alloc(
          (get_associated_allocator)(function));

      asio::prefer(immediate_ex, execution::allocator(alloc)).execute(
          asio::detail::bind_handler(static_cast<Function&&>(function)));
    }
    else
    {
      (initiate_dispatch_with_executor<const immediate_ex_type&>(immediate_ex))(
          static_cast<Function&&>(function), empty_work_function());
    }
  }

  bool io_executor_is_completion_executor(
      Handler& handler, const IoExecutor& io_ex)
  {
    return io_executor_is_completion_executor(handler, io_ex,
        typename handler_work_type::base2_type::work_ownership());
  }

  bool io_executor_is_completion_executor(Handler&,
      const IoExecutor&, handler_work_ownership::maybe)
  {
    return !handler_work_.associated_executor_owns_work();
  }

  bool io_executor_is_completion_executor(Handler& handler,
      const IoExecutor& io_ex, handler_work_ownership::always)
  {
    return execution::detail::executor_targets_equal(
        (asio::get_associated_executor)(handler, io_ex), io_ex);
  }

  handler_work_type handler_work_;
};

template <typename Handler, typename IoExecutor>
class immediate_handler_work<Handler, IoExecutor,
    enable_if_t<
      execution::is_executor<IoExecutor>::value
    >,
    enable_if_t<
      is_same<IoExecutor, associated_executor_t<Handler, IoExecutor>>::value
    >,
    enable_if_t<
      !has_associated_immediate_executor<Handler, IoExecutor>::value
    >,
    enable_if_t<
      !has_associated_executor<Handler, IoExecutor>::value
    >
  >
{
public:
  typedef handler_work<Handler, IoExecutor> handler_work_type;

  ASIO_ALWAYS_INLINE
  explicit immediate_handler_work(handler_work_type&& w)
    : handler_work_(static_cast<handler_work_type&&>(
          w.release_untracked_io_executor()))
  {
  }

  template <typename Function>
  void complete(Function& function, Handler& handler, const void* io_ex)
  {
    this->complete_impl(function, handler,
        *static_cast<const IoExecutor*>(io_ex),
        integral_constant<bool,
          !has_associated_immediate_executor<Handler, IoExecutor>::value
            && execution::is_executor<IoExecutor>::value>());
  }

private:
  template <typename Function>
  void complete_impl(Function& function, Handler& handler,
      const IoExecutor& io_ex, false_type)
  {
    typedef associated_immediate_executor_t<Handler, IoExecutor>
      immediate_ex_type;

    immediate_ex_type immediate_ex = (get_associated_immediate_executor)(
        handler, io_ex);

    associated_allocator_t<Function> alloc(
        (get_associated_allocator)(function));

    asio::prefer(immediate_ex, execution::allocator(alloc)).execute(
        asio::detail::bind_handler(static_cast<Function&&>(function)));
  }

  template <typename Function>
  void complete_impl(Function& function, Handler&,
      const IoExecutor& io_ex, true_type)
  {
    typedef decay_t<
        require_result_t<
          execution::staging_executor<const IoExecutor&>,
          const execution::blocking_t::never_t&
        >
      > immediate_ex_type;

    immediate_ex_type immediate_ex = asio::require(
        execution::stage(io_ex), execution::blocking.never);

    associated_allocator_t<Function> alloc(
        (get_associated_allocator)(function));

    asio::prefer(immediate_ex, execution::allocator(alloc)).execute(
        asio::detail::bind_handler(static_cast<Function&&>(function)));
  }

  handler_work_type handler_work_;
};

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_HANDLER_WORK_HPP
