//
// impl/spawn.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2023 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_IMPL_SPAWN_HPP
#define ASIO_IMPL_SPAWN_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/associated_allocator.hpp"
#include "asio/associated_cancellation_slot.hpp"
#include "asio/associated_executor.hpp"
#include "asio/async_result.hpp"
#include "asio/bind_executor.hpp"
#include "asio/detail/atomic_count.hpp"
#include "asio/detail/bind_handler.hpp"
#include "asio/detail/handler_alloc_helpers.hpp"
#include "asio/detail/handler_cont_helpers.hpp"
#include "asio/detail/handler_invoke_helpers.hpp"
#include "asio/detail/memory.hpp"
#include "asio/detail/noncopyable.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/detail/utility.hpp"
#include "asio/detail/variadic_templates.hpp"
#include "asio/system_error.hpp"

#if defined(ASIO_HAS_STD_TUPLE)
# include <tuple>
#endif // defined(ASIO_HAS_STD_TUPLE)

#if defined(ASIO_HAS_BOOST_CONTEXT_FIBER)
# include <boost/context/fiber.hpp>
#endif // defined(ASIO_HAS_BOOST_CONTEXT_FIBER)

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

#if !defined(ASIO_NO_EXCEPTIONS)
inline void spawned_thread_rethrow(void* ex)
{
  if (*static_cast<exception_ptr*>(ex))
    rethrow_exception(*static_cast<exception_ptr*>(ex));
}
#endif // !defined(ASIO_NO_EXCEPTIONS)

#if defined(ASIO_HAS_BOOST_COROUTINE)

// Spawned thread implementation using Boost.Coroutine.
class spawned_coroutine_thread : public spawned_thread_base
{
public:
#if defined(BOOST_COROUTINES_UNIDIRECT) || defined(BOOST_COROUTINES_V2)
  typedef boost::coroutines::pull_coroutine<void> callee_type;
  typedef boost::coroutines::push_coroutine<void> caller_type;
#else
  typedef boost::coroutines::coroutine<void()> callee_type;
  typedef boost::coroutines::coroutine<void()> caller_type;
#endif

  spawned_coroutine_thread(caller_type& caller)
    : caller_(caller),
      on_suspend_fn_(0),
      on_suspend_arg_(0)
  {
  }

  template <typename F>
  static spawned_thread_base* spawn(ASIO_MOVE_ARG(F) f,
      const boost::coroutines::attributes& attributes,
      cancellation_slot parent_cancel_slot = cancellation_slot(),
      cancellation_state cancel_state = cancellation_state())
  {
    spawned_coroutine_thread* spawned_thread = 0;
    callee_type callee(entry_point<typename decay<F>::type>(
          ASIO_MOVE_CAST(F)(f), &spawned_thread), attributes);
    spawned_thread->callee_.swap(callee);
    spawned_thread->parent_cancellation_slot_ = parent_cancel_slot;
    spawned_thread->cancellation_state_ = cancel_state;
    return spawned_thread;
  }

  template <typename F>
  static spawned_thread_base* spawn(ASIO_MOVE_ARG(F) f,
      cancellation_slot parent_cancel_slot = cancellation_slot(),
      cancellation_state cancel_state = cancellation_state())
  {
    return spawn(ASIO_MOVE_CAST(F)(f), boost::coroutines::attributes(),
        parent_cancel_slot, cancel_state);
  }

  void resume()
  {
    callee_();
    if (on_suspend_fn_)
    {
      void (*fn)(void*) = on_suspend_fn_;
      void* arg = on_suspend_arg_;
      on_suspend_fn_ = 0;
      fn(arg);
    }
  }

  void suspend_with(void (*fn)(void*), void* arg)
  {
    if (throw_if_cancelled_)
      if (!!cancellation_state_.cancelled())
        throw_error(asio::error::operation_aborted, "yield");
    has_context_switched_ = true;
    on_suspend_fn_ = fn;
    on_suspend_arg_ = arg;
    caller_();
  }

  void destroy()
  {
    callee_type callee;
    callee.swap(callee_);
    if (terminal_)
      callee();
  }

private:
  template <typename Function>
  class entry_point
  {
  public:
    template <typename F>
    entry_point(ASIO_MOVE_ARG(F) f,
        spawned_coroutine_thread** spawned_thread_out)
      : function_(ASIO_MOVE_CAST(F)(f)),
        spawned_thread_out_(spawned_thread_out)
    {
    }

    void operator()(caller_type& caller)
    {
      Function function(ASIO_MOVE_CAST(Function)(function_));
      spawned_coroutine_thread spawned_thread(caller);
      *spawned_thread_out_ = &spawned_thread;
      spawned_thread_out_ = 0;
      spawned_thread.suspend();
#if !defined(ASIO_NO_EXCEPTIONS)
      try
#endif // !defined(ASIO_NO_EXCEPTIONS)
      {
        function(&spawned_thread);
        spawned_thread.terminal_ = true;
        spawned_thread.suspend();
      }
#if !defined(ASIO_NO_EXCEPTIONS)
      catch (const boost::coroutines::detail::forced_unwind&)
      {
        throw;
      }
      catch (...)
      {
        exception_ptr ex = current_exception();
        spawned_thread.terminal_ = true;
        spawned_thread.suspend_with(spawned_thread_rethrow, &ex);
      }
#endif // !defined(ASIO_NO_EXCEPTIONS)
    }

  private:
    Function function_;
    spawned_coroutine_thread** spawned_thread_out_;
  };

  caller_type& caller_;
  callee_type callee_;
  void (*on_suspend_fn_)(void*);
  void* on_suspend_arg_;
};

#endif // defined(ASIO_HAS_BOOST_COROUTINE)

#if defined(ASIO_HAS_BOOST_CONTEXT_FIBER)

// Spawned thread implementation using Boost.Context's fiber.
class spawned_fiber_thread : public spawned_thread_base
{
public:
  typedef boost::context::fiber fiber_type;

  spawned_fiber_thread(ASIO_MOVE_ARG(fiber_type) caller)
    : caller_(ASIO_MOVE_CAST(fiber_type)(caller)),
      on_suspend_fn_(0),
      on_suspend_arg_(0)
  {
  }

  template <typename StackAllocator, typename F>
  static spawned_thread_base* spawn(allocator_arg_t,
      ASIO_MOVE_ARG(StackAllocator) stack_allocator,
      ASIO_MOVE_ARG(F) f,
      cancellation_slot parent_cancel_slot = cancellation_slot(),
      cancellation_state cancel_state = cancellation_state())
  {
    spawned_fiber_thread* spawned_thread = 0;
    fiber_type callee(allocator_arg_t(),
        ASIO_MOVE_CAST(StackAllocator)(stack_allocator),
        entry_point<typename decay<F>::type>(
          ASIO_MOVE_CAST(F)(f), &spawned_thread));
    callee = fiber_type(ASIO_MOVE_CAST(fiber_type)(callee)).resume();
    spawned_thread->callee_ = ASIO_MOVE_CAST(fiber_type)(callee);
    spawned_thread->parent_cancellation_slot_ = parent_cancel_slot;
    spawned_thread->cancellation_state_ = cancel_state;
    return spawned_thread;
  }

  template <typename F>
  static spawned_thread_base* spawn(ASIO_MOVE_ARG(F) f,
      cancellation_slot parent_cancel_slot = cancellation_slot(),
      cancellation_state cancel_state = cancellation_state())
  {
    return spawn(allocator_arg_t(), boost::context::fixedsize_stack(),
        ASIO_MOVE_CAST(F)(f), parent_cancel_slot, cancel_state);
  }

  void resume()
  {
    callee_ = fiber_type(ASIO_MOVE_CAST(fiber_type)(callee_)).resume();
    if (on_suspend_fn_)
    {
      void (*fn)(void*) = on_suspend_fn_;
      void* arg = on_suspend_arg_;
      on_suspend_fn_ = 0;
      fn(arg);
    }
  }

  void suspend_with(void (*fn)(void*), void* arg)
  {
    if (throw_if_cancelled_)
      if (!!cancellation_state_.cancelled())
        throw_error(asio::error::operation_aborted, "yield");
    has_context_switched_ = true;
    on_suspend_fn_ = fn;
    on_suspend_arg_ = arg;
    caller_ = fiber_type(ASIO_MOVE_CAST(fiber_type)(caller_)).resume();
  }

  void destroy()
  {
    fiber_type callee = ASIO_MOVE_CAST(fiber_type)(callee_);
    if (terminal_)
      fiber_type(ASIO_MOVE_CAST(fiber_type)(callee)).resume();
  }

private:
  template <typename Function>
  class entry_point
  {
  public:
    template <typename F>
    entry_point(ASIO_MOVE_ARG(F) f,
        spawned_fiber_thread** spawned_thread_out)
      : function_(ASIO_MOVE_CAST(F)(f)),
        spawned_thread_out_(spawned_thread_out)
    {
    }

    fiber_type operator()(ASIO_MOVE_ARG(fiber_type) caller)
    {
      Function function(ASIO_MOVE_CAST(Function)(function_));
      spawned_fiber_thread spawned_thread(
          ASIO_MOVE_CAST(fiber_type)(caller));
      *spawned_thread_out_ = &spawned_thread;
      spawned_thread_out_ = 0;
      spawned_thread.suspend();
#if !defined(ASIO_NO_EXCEPTIONS)
      try
#endif // !defined(ASIO_NO_EXCEPTIONS)
      {
        function(&spawned_thread);
        spawned_thread.terminal_ = true;
        spawned_thread.suspend();
      }
#if !defined(ASIO_NO_EXCEPTIONS)
      catch (const boost::context::detail::forced_unwind&)
      {
        throw;
      }
      catch (...)
      {
        exception_ptr ex = current_exception();
        spawned_thread.terminal_ = true;
        spawned_thread.suspend_with(spawned_thread_rethrow, &ex);
      }
#endif // !defined(ASIO_NO_EXCEPTIONS)
      return ASIO_MOVE_CAST(fiber_type)(spawned_thread.caller_);
    }

  private:
    Function function_;
    spawned_fiber_thread** spawned_thread_out_;
  };

  fiber_type caller_;
  fiber_type callee_;
  void (*on_suspend_fn_)(void*);
  void* on_suspend_arg_;
};

#endif // defined(ASIO_HAS_BOOST_CONTEXT_FIBER)

#if defined(ASIO_HAS_BOOST_CONTEXT_FIBER)
typedef spawned_fiber_thread default_spawned_thread_type;
#elif defined(ASIO_HAS_BOOST_COROUTINE)
typedef spawned_coroutine_thread default_spawned_thread_type;
#else
# error No spawn() implementation available
#endif

// Helper class to perform the initial resume on the correct executor.
class spawned_thread_resumer
{
public:
  explicit spawned_thread_resumer(spawned_thread_base* spawned_thread)
    : spawned_thread_(spawned_thread)
  {
#if !defined(ASIO_HAS_MOVE)
    spawned_thread->detach();
    spawned_thread->attach(&spawned_thread_);
#endif // !defined(ASIO_HAS_MOVE)
  }

#if defined(ASIO_HAS_MOVE)

  spawned_thread_resumer(spawned_thread_resumer&& other) ASIO_NOEXCEPT
    : spawned_thread_(other.spawned_thread_)
  {
    other.spawned_thread_ = 0;
  }

#else // defined(ASIO_HAS_MOVE)

  spawned_thread_resumer(
      const spawned_thread_resumer& other) ASIO_NOEXCEPT
    : spawned_thread_(other.spawned_thread_)
  {
    spawned_thread_->detach();
    spawned_thread_->attach(&spawned_thread_);
  }

#endif // defined(ASIO_HAS_MOVE)

  ~spawned_thread_resumer()
  {
    if (spawned_thread_)
      spawned_thread_->destroy();
  }

  void operator()()
  {
#if defined(ASIO_HAS_MOVE)
    spawned_thread_->attach(&spawned_thread_);
#endif // defined(ASIO_HAS_MOVE)
    spawned_thread_->resume();
  }

private:
  spawned_thread_base* spawned_thread_;
};

// Helper class to ensure spawned threads are destroyed on the correct executor.
class spawned_thread_destroyer
{
public:
  explicit spawned_thread_destroyer(spawned_thread_base* spawned_thread)
    : spawned_thread_(spawned_thread)
  {
    spawned_thread->detach();
#if !defined(ASIO_HAS_MOVE)
    spawned_thread->attach(&spawned_thread_);
#endif // !defined(ASIO_HAS_MOVE)
  }

#if defined(ASIO_HAS_MOVE)

  spawned_thread_destroyer(spawned_thread_destroyer&& other) ASIO_NOEXCEPT
    : spawned_thread_(other.spawned_thread_)
  {
    other.spawned_thread_ = 0;
  }

#else // defined(ASIO_HAS_MOVE)

  spawned_thread_destroyer(
      const spawned_thread_destroyer& other) ASIO_NOEXCEPT
    : spawned_thread_(other.spawned_thread_)
  {
    spawned_thread_->detach();
    spawned_thread_->attach(&spawned_thread_);
  }

#endif // defined(ASIO_HAS_MOVE)

  ~spawned_thread_destroyer()
  {
    if (spawned_thread_)
      spawned_thread_->destroy();
  }

  void operator()()
  {
    if (spawned_thread_)
    {
      spawned_thread_->destroy();
      spawned_thread_ = 0;
    }
  }

private:
  spawned_thread_base* spawned_thread_;
};

// Base class for all completion handlers associated with a spawned thread.
template <typename Executor>
class spawn_handler_base
{
public:
  typedef Executor executor_type;
  typedef cancellation_slot cancellation_slot_type;

  spawn_handler_base(const basic_yield_context<Executor>& yield)
    : yield_(yield),
      spawned_thread_(yield.spawned_thread_)
  {
    spawned_thread_->detach();
#if !defined(ASIO_HAS_MOVE)
    spawned_thread_->attach(&spawned_thread_);
#endif // !defined(ASIO_HAS_MOVE)
  }

#if defined(ASIO_HAS_MOVE)

  spawn_handler_base(spawn_handler_base&& other) ASIO_NOEXCEPT
    : yield_(other.yield_),
      spawned_thread_(other.spawned_thread_)

  {
    other.spawned_thread_ = 0;
  }

#else // defined(ASIO_HAS_MOVE)

  spawn_handler_base(const spawn_handler_base& other) ASIO_NOEXCEPT
    : yield_(other.yield_),
      spawned_thread_(other.spawned_thread_)
  {
    spawned_thread_->detach();
    spawned_thread_->attach(&spawned_thread_);
  }

#endif // defined(ASIO_HAS_MOVE)

  ~spawn_handler_base()
  {
    if (spawned_thread_)
      (post)(yield_.executor_, spawned_thread_destroyer(spawned_thread_));
  }

  executor_type get_executor() const ASIO_NOEXCEPT
  {
    return yield_.executor_;
  }

  cancellation_slot_type get_cancellation_slot() const ASIO_NOEXCEPT
  {
    return spawned_thread_->get_cancellation_slot();
  }

  void resume()
  {
    spawned_thread_resumer resumer(spawned_thread_);
    spawned_thread_ = 0;
    resumer();
  }

protected:
  const basic_yield_context<Executor>& yield_;
  spawned_thread_base* spawned_thread_;
};

// Completion handlers for when basic_yield_context is used as a token.
template <typename Executor, typename Signature>
class spawn_handler;

template <typename Executor, typename R>
class spawn_handler<Executor, R()>
  : public spawn_handler_base<Executor>
{
public:
  typedef void return_type;

  struct result_type {};

  spawn_handler(const basic_yield_context<Executor>& yield, result_type&)
    : spawn_handler_base<Executor>(yield)
  {
  }

  void operator()()
  {
    this->resume();
  }

  static return_type on_resume(result_type&)
  {
  }
};

template <typename Executor, typename R>
class spawn_handler<Executor, R(asio::error_code)>
  : public spawn_handler_base<Executor>
{
public:
  typedef void return_type;
  typedef asio::error_code* result_type;

  spawn_handler(const basic_yield_context<Executor>& yield, result_type& result)
    : spawn_handler_base<Executor>(yield),
      result_(result)
  {
  }

  void operator()(asio::error_code ec)
  {
    if (this->yield_.ec_)
    {
      *this->yield_.ec_ = ec;
      result_ = 0;
    }
    else
      result_ = &ec;
    this->resume();
  }

  static return_type on_resume(result_type& result)
  {
    if (result)
      throw_error(*result);
  }

private:
  result_type& result_;
};

template <typename Executor, typename R>
class spawn_handler<Executor, R(exception_ptr)>
  : public spawn_handler_base<Executor>
{
public:
  typedef void return_type;
  typedef exception_ptr* result_type;

  spawn_handler(const basic_yield_context<Executor>& yield, result_type& result)
    : spawn_handler_base<Executor>(yield),
      result_(result)
  {
  }

  void operator()(exception_ptr ex)
  {
    result_ = &ex;
    this->resume();
  }

  static return_type on_resume(result_type& result)
  {
    if (result)
      rethrow_exception(*result);
  }

private:
  result_type& result_;
};

template <typename Executor, typename R, typename T>
class spawn_handler<Executor, R(T)>
  : public spawn_handler_base<Executor>
{
public:
  typedef T return_type;
  typedef return_type* result_type;

  spawn_handler(const basic_yield_context<Executor>& yield, result_type& result)
    : spawn_handler_base<Executor>(yield),
      result_(result)
  {
  }

  void operator()(T value)
  {
    result_ = &value;
    this->resume();
  }

  static return_type on_resume(result_type& result)
  {
    return ASIO_MOVE_CAST(return_type)(*result);
  }

private:
  result_type& result_;
};

template <typename Executor, typename R, typename T>
class spawn_handler<Executor, R(asio::error_code, T)>
  : public spawn_handler_base<Executor>
{
public:
  typedef T return_type;

  struct result_type
  {
    asio::error_code* ec_;
    return_type* value_;
  };

  spawn_handler(const basic_yield_context<Executor>& yield, result_type& result)
    : spawn_handler_base<Executor>(yield),
      result_(result)
  {
  }

  void operator()(asio::error_code ec, T value)
  {
    if (this->yield_.ec_)
    {
      *this->yield_.ec_ = ec;
      result_.ec_ = 0;
    }
    else
      result_.ec_ = &ec;
    result_.value_ = &value;
    this->resume();
  }

  static return_type on_resume(result_type& result)
  {
    if (result.ec_)
      throw_error(*result.ec_);
    return ASIO_MOVE_CAST(return_type)(*result.value_);
  }

private:
  result_type& result_;
};

template <typename Executor, typename R, typename T>
class spawn_handler<Executor, R(exception_ptr, T)>
  : public spawn_handler_base<Executor>
{
public:
  typedef T return_type;

  struct result_type
  {
    exception_ptr ex_;
    return_type* value_;
  };

  spawn_handler(const basic_yield_context<Executor>& yield, result_type& result)
    : spawn_handler_base<Executor>(yield),
      result_(result)
  {
  }

  void operator()(exception_ptr ex, T value)
  {
    result_.ex_ = &ex;
    result_.value_ = &value;
    this->resume();
  }

  static return_type on_resume(result_type& result)
  {
    if (result.ex_)
      rethrow_exception(*result.ex_);
    return ASIO_MOVE_CAST(return_type)(*result.value_);
  }

private:
  result_type& result_;
};

#if defined(ASIO_HAS_VARIADIC_TEMPLATES) \
  && defined(ASIO_HAS_STD_TUPLE)

template <typename Executor, typename R, typename... Ts>
class spawn_handler<Executor, R(Ts...)>
  : public spawn_handler_base<Executor>
{
public:
  typedef std::tuple<Ts...> return_type;

  typedef return_type* result_type;

  spawn_handler(const basic_yield_context<Executor>& yield, result_type& result)
    : spawn_handler_base<Executor>(yield),
      result_(result)
  {
  }

  template <typename... Args>
  void operator()(ASIO_MOVE_ARG(Args)... args)
  {
    return_type value(ASIO_MOVE_CAST(Args)(args)...);
    result_ = &value;
    this->resume();
  }

  static return_type on_resume(result_type& result)
  {
    return ASIO_MOVE_CAST(return_type)(*result);
  }

private:
  result_type& result_;
};

template <typename Executor, typename R, typename... Ts>
class spawn_handler<Executor, R(asio::error_code, Ts...)>
  : public spawn_handler_base<Executor>
{
public:
  typedef std::tuple<Ts...> return_type;

  struct result_type
  {
    asio::error_code* ec_;
    return_type* value_;
  };

  spawn_handler(const basic_yield_context<Executor>& yield, result_type& result)
    : spawn_handler_base<Executor>(yield),
      result_(result)
  {
  }

  template <typename... Args>
  void operator()(asio::error_code ec,
      ASIO_MOVE_ARG(Args)... args)
  {
    return_type value(ASIO_MOVE_CAST(Args)(args)...);
    if (this->yield_.ec_)
    {
      *this->yield_.ec_ = ec;
      result_.ec_ = 0;
    }
    else
      result_.ec_ = &ec;
    result_.value_ = &value;
    this->resume();
  }

  static return_type on_resume(result_type& result)
  {
    if (result.ec_)
      throw_error(*result.ec_);
    return ASIO_MOVE_CAST(return_type)(*result.value_);
  }

private:
  result_type& result_;
};

template <typename Executor, typename R, typename... Ts>
class spawn_handler<Executor, R(exception_ptr, Ts...)>
  : public spawn_handler_base<Executor>
{
public:
  typedef std::tuple<Ts...> return_type;

  struct result_type
  {
    exception_ptr ex_;
    return_type* value_;
  };

  spawn_handler(const basic_yield_context<Executor>& yield, result_type& result)
    : spawn_handler_base<Executor>(yield),
      result_(result)
  {
  }

  template <typename... Args>
  void operator()(exception_ptr ex, ASIO_MOVE_ARG(Args)... args)
  {
    return_type value(ASIO_MOVE_CAST(Args)(args)...);
    result_.ex_ = &ex;
    result_.value_ = &value;
    this->resume();
  }

  static return_type on_resume(result_type& result)
  {
    if (result.ex_)
      rethrow_exception(*result.ex_);
    return ASIO_MOVE_CAST(return_type)(*result.value_);
  }

private:
  result_type& result_;
};

#endif // defined(ASIO_HAS_VARIADIC_TEMPLATES)
       //   && defined(ASIO_HAS_STD_TUPLE)

template <typename Executor, typename Signature>
inline bool asio_handler_is_continuation(spawn_handler<Executor, Signature>*)
{
  return true;
}

} // namespace detail

template <typename Executor, typename Signature>
class async_result<basic_yield_context<Executor>, Signature>
{
public:
  typedef typename detail::spawn_handler<Executor, Signature> handler_type;
  typedef typename handler_type::return_type return_type;

#if defined(ASIO_HAS_VARIADIC_TEMPLATES)
# if defined(ASIO_HAS_VARIADIC_LAMBDA_CAPTURES)

  template <typename Initiation, typename... InitArgs>
  static return_type initiate(ASIO_MOVE_ARG(Initiation) init,
      const basic_yield_context<Executor>& yield,
      ASIO_MOVE_ARG(InitArgs)... init_args)
  {
    typename handler_type::result_type result
      = typename handler_type::result_type();

    yield.spawned_thread_->suspend_with(
        [&]()
        {
          ASIO_MOVE_CAST(Initiation)(init)(
              handler_type(yield, result),
              ASIO_MOVE_CAST(InitArgs)(init_args)...);
        });

    return handler_type::on_resume(result);
  }

# else // defined(ASIO_HAS_VARIADIC_LAMBDA_CAPTURES)

  template <typename Initiation, typename... InitArgs>
  struct suspend_with_helper
  {
    typename handler_type::result_type& result_;
    ASIO_MOVE_ARG(Initiation) init_;
    const basic_yield_context<Executor>& yield_;
    std::tuple<ASIO_MOVE_ARG(InitArgs)...> init_args_;

    template <std::size_t... I>
    void do_invoke(detail::index_sequence<I...>)
    {
      ASIO_MOVE_CAST(Initiation)(init_)(
          handler_type(yield_, result_),
          ASIO_MOVE_CAST(InitArgs)(std::get<I>(init_args_))...);
    }

    void operator()()
    {
      this->do_invoke(detail::make_index_sequence<sizeof...(InitArgs)>());
    }
  };

  template <typename Initiation, typename... InitArgs>
  static return_type initiate(ASIO_MOVE_ARG(Initiation) init,
      const basic_yield_context<Executor>& yield,
      ASIO_MOVE_ARG(InitArgs)... init_args)
  {
    typename handler_type::result_type result
      = typename handler_type::result_type();

    yield.spawned_thread_->suspend_with(
      suspend_with_helper<Initiation, InitArgs...>{
          result, ASIO_MOVE_CAST(Initiation)(init), yield,
          std::tuple<ASIO_MOVE_ARG(InitArgs)...>(
            ASIO_MOVE_CAST(InitArgs)(init_args)...)});

    return handler_type::on_resume(result);
  }

# endif // defined(ASIO_HAS_VARIADIC_LAMBDA_CAPTURES)
#else // defined(ASIO_HAS_VARIADIC_TEMPLATES)

  template <typename Initiation>
  static return_type initiate(Initiation init,
      const basic_yield_context<Executor>& yield)
  {
    typename handler_type::result_type result
      = typename handler_type::result_type();

    struct on_suspend
    {
      Initiation& init_;
      const basic_yield_context<Executor>& yield_;
      typename handler_type::result_type& result_;

      void do_call()
      {
        ASIO_MOVE_CAST(Initiation)(init_)(
            handler_type(yield_, result_));
      }

      static void call(void* arg)
      {
        static_cast<on_suspend*>(arg)->do_call();
      }
    } o = { init, yield, result };

    yield.spawned_thread_->suspend_with(&on_suspend::call, &o);

    return handler_type::on_resume(result);
  }

#define ASIO_PRIVATE_ON_SUSPEND_MEMBERS(n) \
  ASIO_PRIVATE_ON_SUSPEND_MEMBERS_##n
#define ASIO_PRIVATE_ON_SUSPEND_MEMBERS_1 \
  T1& x1;
#define ASIO_PRIVATE_ON_SUSPEND_MEMBERS_2 \
  T1& x1; T2& x2;
#define ASIO_PRIVATE_ON_SUSPEND_MEMBERS_3 \
  T1& x1; T2& x2; T3& x3;
#define ASIO_PRIVATE_ON_SUSPEND_MEMBERS_4 \
  T1& x1; T2& x2; T3& x3; T4& x4;
#define ASIO_PRIVATE_ON_SUSPEND_MEMBERS_5 \
  T1& x1; T2& x2; T3& x3; T4& x4; T5& x5;
#define ASIO_PRIVATE_ON_SUSPEND_MEMBERS_6 \
  T1& x1; T2& x2; T3& x3; T4& x4; T5& x5; T6& x6;
#define ASIO_PRIVATE_ON_SUSPEND_MEMBERS_7 \
  T1& x1; T2& x2; T3& x3; T4& x4; T5& x5; T6& x6; T7& x7;
#define ASIO_PRIVATE_ON_SUSPEND_MEMBERS_8 \
  T1& x1; T2& x2; T3& x3; T4& x4; T5& x5; T6& x6; T7& x7; T8& x8;

#define ASIO_PRIVATE_INITIATE_DEF(n) \
  template <typename Initiation, ASIO_VARIADIC_TPARAMS(n)> \
  static return_type initiate(Initiation init, \
      const basic_yield_context<Executor>& yield, \
      ASIO_VARIADIC_BYVAL_PARAMS(n)) \
  { \
    typename handler_type::result_type result \
      = typename handler_type::result_type(); \
  \
    struct on_suspend \
    { \
      Initiation& init; \
      const basic_yield_context<Executor>& yield; \
      typename handler_type::result_type& result; \
      ASIO_PRIVATE_ON_SUSPEND_MEMBERS(n) \
  \
      void do_call() \
      { \
        ASIO_MOVE_CAST(Initiation)(init)( \
            handler_type(yield, result), \
            ASIO_VARIADIC_MOVE_ARGS(n)); \
      } \
  \
      static void call(void* arg) \
      { \
        static_cast<on_suspend*>(arg)->do_call(); \
      } \
    } o = { init, yield, result, ASIO_VARIADIC_BYVAL_ARGS(n) }; \
  \
    yield.spawned_thread_->suspend_with(&on_suspend::call, &o); \
  \
    return handler_type::on_resume(result); \
  } \
  /**/
  ASIO_VARIADIC_GENERATE(ASIO_PRIVATE_INITIATE_DEF)
#undef ASIO_PRIVATE_INITIATE_DEF
#undef ASIO_PRIVATE_ON_SUSPEND_MEMBERS
#undef ASIO_PRIVATE_ON_SUSPEND_MEMBERS_1
#undef ASIO_PRIVATE_ON_SUSPEND_MEMBERS_2
#undef ASIO_PRIVATE_ON_SUSPEND_MEMBERS_3
#undef ASIO_PRIVATE_ON_SUSPEND_MEMBERS_4
#undef ASIO_PRIVATE_ON_SUSPEND_MEMBERS_5
#undef ASIO_PRIVATE_ON_SUSPEND_MEMBERS_6
#undef ASIO_PRIVATE_ON_SUSPEND_MEMBERS_7
#undef ASIO_PRIVATE_ON_SUSPEND_MEMBERS_8

#endif // defined(ASIO_HAS_VARIADIC_TEMPLATES)
};

namespace detail {

template <typename Executor, typename Function, typename Handler>
class spawn_entry_point
{
public:
  template <typename F, typename H>
  spawn_entry_point(const Executor& ex,
      ASIO_MOVE_ARG(F) f, ASIO_MOVE_ARG(H) h)
    : executor_(ex),
      function_(ASIO_MOVE_CAST(F)(f)),
      handler_(ASIO_MOVE_CAST(H)(h)),
      work_(handler_, executor_)
  {
  }

  void operator()(spawned_thread_base* spawned_thread)
  {
    const basic_yield_context<Executor> yield(spawned_thread, executor_);
    this->call(yield,
        void_type<typename result_of<Function(
          basic_yield_context<Executor>)>::type>());
  }

private:
  void call(const basic_yield_context<Executor>& yield, void_type<void>)
  {
#if !defined(ASIO_NO_EXCEPTIONS)
    try
#endif // !defined(ASIO_NO_EXCEPTIONS)
    {
      function_(yield);
      if (!yield.spawned_thread_->has_context_switched())
        (post)(yield);
      detail::binder1<Handler, exception_ptr>
        handler(handler_, exception_ptr());
      work_.complete(handler, handler.handler_);
    }
#if !defined(ASIO_NO_EXCEPTIONS)
# if defined(ASIO_HAS_BOOST_CONTEXT_FIBER)
    catch (const boost::context::detail::forced_unwind&)
    {
      throw;
    }
# endif // defined(ASIO_HAS_BOOST_CONTEXT_FIBER)
# if defined(ASIO_HAS_BOOST_COROUTINE)
    catch (const boost::coroutines::detail::forced_unwind&)
    {
      throw;
    }
# endif // defined(ASIO_HAS_BOOST_COROUTINE)
    catch (...)
    {
      exception_ptr ex = current_exception();
      if (!yield.spawned_thread_->has_context_switched())
        (post)(yield);
      detail::binder1<Handler, exception_ptr> handler(handler_, ex);
      work_.complete(handler, handler.handler_);
    }
#endif // !defined(ASIO_NO_EXCEPTIONS)
  }

  template <typename T>
  void call(const basic_yield_context<Executor>& yield, void_type<T>)
  {
#if !defined(ASIO_NO_EXCEPTIONS)
    try
#endif // !defined(ASIO_NO_EXCEPTIONS)
    {
      T result(function_(yield));
      if (!yield.spawned_thread_->has_context_switched())
        (post)(yield);
      detail::binder2<Handler, exception_ptr, T>
        handler(handler_, exception_ptr(), ASIO_MOVE_CAST(T)(result));
      work_.complete(handler, handler.handler_);
    }
#if !defined(ASIO_NO_EXCEPTIONS)
# if defined(ASIO_HAS_BOOST_CONTEXT_FIBER)
    catch (const boost::context::detail::forced_unwind&)
    {
      throw;
    }
# endif // defined(ASIO_HAS_BOOST_CONTEXT_FIBER)
# if defined(ASIO_HAS_BOOST_COROUTINE)
    catch (const boost::coroutines::detail::forced_unwind&)
    {
      throw;
    }
# endif // defined(ASIO_HAS_BOOST_COROUTINE)
    catch (...)
    {
      exception_ptr ex = current_exception();
      if (!yield.spawned_thread_->has_context_switched())
        (post)(yield);
      detail::binder2<Handler, exception_ptr, T> handler(handler_, ex, T());
      work_.complete(handler, handler.handler_);
    }
#endif // !defined(ASIO_NO_EXCEPTIONS)
  }

  Executor executor_;
  Function function_;
  Handler handler_;
  handler_work<Handler, Executor> work_;
};

struct spawn_cancellation_signal_emitter
{
  cancellation_signal* signal_;
  cancellation_type_t type_;

  void operator()()
  {
    signal_->emit(type_);
  }
};

template <typename Handler, typename Executor, typename = void>
class spawn_cancellation_handler
{
public:
  spawn_cancellation_handler(const Handler&, const Executor& ex)
    : ex_(ex)
  {
  }

  cancellation_slot slot()
  {
    return signal_.slot();
  }

  void operator()(cancellation_type_t type)
  {
    spawn_cancellation_signal_emitter emitter = { &signal_, type };
    (dispatch)(ex_, emitter);
  }

private:
  cancellation_signal signal_;
  Executor ex_;
};


template <typename Handler, typename Executor>
class spawn_cancellation_handler<Handler, Executor,
    typename enable_if<
      is_same<
        typename associated_executor<Handler,
          Executor>::asio_associated_executor_is_unspecialised,
        void
      >::value
    >::type>
{
public:
  spawn_cancellation_handler(const Handler&, const Executor&)
  {
  }

  cancellation_slot slot()
  {
    return signal_.slot();
  }

  void operator()(cancellation_type_t type)
  {
    signal_.emit(type);
  }

private:
  cancellation_signal signal_;
};

template <typename Executor>
class initiate_spawn
{
public:
  typedef Executor executor_type;

  explicit initiate_spawn(const executor_type& ex)
    : executor_(ex)
  {
  }

  executor_type get_executor() const ASIO_NOEXCEPT
  {
    return executor_;
  }

  template <typename Handler, typename F>
  void operator()(ASIO_MOVE_ARG(Handler) handler,
      ASIO_MOVE_ARG(F) f) const
  {
    typedef typename decay<Handler>::type handler_type;
    typedef typename decay<F>::type function_type;
    typedef spawn_cancellation_handler<
      handler_type, Executor> cancel_handler_type;

    typename associated_cancellation_slot<handler_type>::type slot
      = asio::get_associated_cancellation_slot(handler);

    cancel_handler_type* cancel_handler = slot.is_connected()
      ? &slot.template emplace<cancel_handler_type>(handler, executor_)
      : 0;

    cancellation_slot proxy_slot(
        cancel_handler
          ? cancel_handler->slot()
          : cancellation_slot());

    cancellation_state cancel_state(proxy_slot);

    (dispatch)(executor_,
        spawned_thread_resumer(
          default_spawned_thread_type::spawn(
            spawn_entry_point<Executor, function_type, handler_type>(
              executor_, ASIO_MOVE_CAST(F)(f),
              ASIO_MOVE_CAST(Handler)(handler)),
            proxy_slot, cancel_state)));
  }

#if defined(ASIO_HAS_BOOST_CONTEXT_FIBER)

  template <typename Handler, typename StackAllocator, typename F>
  void operator()(ASIO_MOVE_ARG(Handler) handler, allocator_arg_t,
      ASIO_MOVE_ARG(StackAllocator) stack_allocator,
      ASIO_MOVE_ARG(F) f) const
  {
    typedef typename decay<Handler>::type handler_type;
    typedef typename decay<F>::type function_type;
    typedef spawn_cancellation_handler<
      handler_type, Executor> cancel_handler_type;

    typename associated_cancellation_slot<handler_type>::type slot
      = asio::get_associated_cancellation_slot(handler);

    cancel_handler_type* cancel_handler = slot.is_connected()
      ? &slot.template emplace<cancel_handler_type>(handler, executor_)
      : 0;

    cancellation_slot proxy_slot(
        cancel_handler
          ? cancel_handler->slot()
          : cancellation_slot());

    cancellation_state cancel_state(proxy_slot);

    (dispatch)(executor_,
        spawned_thread_resumer(
          spawned_fiber_thread::spawn(allocator_arg_t(),
            ASIO_MOVE_CAST(StackAllocator)(stack_allocator),
            spawn_entry_point<Executor, function_type, handler_type>(
              executor_, ASIO_MOVE_CAST(F)(f),
              ASIO_MOVE_CAST(Handler)(handler)),
            proxy_slot, cancel_state)));
  }

#endif // defined(ASIO_HAS_BOOST_CONTEXT_FIBER)

private:
  executor_type executor_;
};

} // namespace detail

template <typename Executor, typename F,
    ASIO_COMPLETION_TOKEN_FOR(typename detail::spawn_signature<
      typename result_of<F(basic_yield_context<Executor>)>::type>::type)
        CompletionToken>
inline ASIO_INITFN_AUTO_RESULT_TYPE_PREFIX(CompletionToken,
    typename detail::spawn_signature<
      typename result_of<F(basic_yield_context<Executor>)>::type>::type)
spawn(const Executor& ex, ASIO_MOVE_ARG(F) function,
    ASIO_MOVE_ARG(CompletionToken) token,
#if defined(ASIO_HAS_BOOST_COROUTINE)
    typename constraint<
      !is_same<
        typename decay<CompletionToken>::type,
        boost::coroutines::attributes
      >::value
    >::type,
#endif // defined(ASIO_HAS_BOOST_COROUTINE)
    typename constraint<
      is_executor<Executor>::value || execution::is_executor<Executor>::value
    >::type)
  ASIO_INITFN_AUTO_RESULT_TYPE_SUFFIX((
    async_initiate<CompletionToken,
      typename detail::spawn_signature<
        typename result_of<F(basic_yield_context<Executor>)>::type>::type>(
          declval<detail::initiate_spawn<Executor> >(),
          token, ASIO_MOVE_CAST(F)(function))))
{
  return async_initiate<CompletionToken,
    typename detail::spawn_signature<
      typename result_of<F(basic_yield_context<Executor>)>::type>::type>(
        detail::initiate_spawn<Executor>(ex),
        token, ASIO_MOVE_CAST(F)(function));
}

template <typename ExecutionContext, typename F,
    ASIO_COMPLETION_TOKEN_FOR(typename detail::spawn_signature<
      typename result_of<F(basic_yield_context<
        typename ExecutionContext::executor_type>)>::type>::type)
          CompletionToken>
inline ASIO_INITFN_AUTO_RESULT_TYPE_PREFIX(CompletionToken,
    typename detail::spawn_signature<
      typename result_of<F(basic_yield_context<
        typename ExecutionContext::executor_type>)>::type>::type)
spawn(ExecutionContext& ctx, ASIO_MOVE_ARG(F) function,
    ASIO_MOVE_ARG(CompletionToken) token,
#if defined(ASIO_HAS_BOOST_COROUTINE)
    typename constraint<
      !is_same<
        typename decay<CompletionToken>::type,
        boost::coroutines::attributes
      >::value
    >::type,
#endif // defined(ASIO_HAS_BOOST_COROUTINE)
    typename constraint<
      is_convertible<ExecutionContext&, execution_context&>::value
    >::type)
  ASIO_INITFN_AUTO_RESULT_TYPE_SUFFIX((
    async_initiate<CompletionToken,
      typename detail::spawn_signature<
        typename result_of<F(basic_yield_context<
          typename ExecutionContext::executor_type>)>::type>::type>(
            declval<detail::initiate_spawn<
              typename ExecutionContext::executor_type> >(),
            token, ASIO_MOVE_CAST(F)(function))))
{
  return (spawn)(ctx.get_executor(), ASIO_MOVE_CAST(F)(function),
      ASIO_MOVE_CAST(CompletionToken)(token));
}

template <typename Executor, typename F,
    ASIO_COMPLETION_TOKEN_FOR(typename detail::spawn_signature<
      typename result_of<F(basic_yield_context<Executor>)>::type>::type)
        CompletionToken>
inline ASIO_INITFN_AUTO_RESULT_TYPE_PREFIX(CompletionToken,
    typename detail::spawn_signature<
      typename result_of<F(basic_yield_context<Executor>)>::type>::type)
spawn(const basic_yield_context<Executor>& ctx,
    ASIO_MOVE_ARG(F) function,
    ASIO_MOVE_ARG(CompletionToken) token,
#if defined(ASIO_HAS_BOOST_COROUTINE)
    typename constraint<
      !is_same<
        typename decay<CompletionToken>::type,
        boost::coroutines::attributes
      >::value
    >::type,
#endif // defined(ASIO_HAS_BOOST_COROUTINE)
    typename constraint<
      is_executor<Executor>::value || execution::is_executor<Executor>::value
    >::type)
  ASIO_INITFN_AUTO_RESULT_TYPE_SUFFIX((
    async_initiate<CompletionToken,
      typename detail::spawn_signature<
        typename result_of<F(basic_yield_context<Executor>)>::type>::type>(
          declval<detail::initiate_spawn<Executor> >(),
          token, ASIO_MOVE_CAST(F)(function))))
{
  return (spawn)(ctx.get_executor(), ASIO_MOVE_CAST(F)(function),
      ASIO_MOVE_CAST(CompletionToken)(token));
}

#if defined(ASIO_HAS_BOOST_CONTEXT_FIBER)

template <typename Executor, typename StackAllocator, typename F,
    ASIO_COMPLETION_TOKEN_FOR(typename detail::spawn_signature<
      typename result_of<F(basic_yield_context<Executor>)>::type>::type)
        CompletionToken>
inline ASIO_INITFN_AUTO_RESULT_TYPE_PREFIX(CompletionToken,
    typename detail::spawn_signature<
      typename result_of<F(basic_yield_context<Executor>)>::type>::type)
spawn(const Executor& ex, allocator_arg_t,
    ASIO_MOVE_ARG(StackAllocator) stack_allocator,
    ASIO_MOVE_ARG(F) function,
    ASIO_MOVE_ARG(CompletionToken) token,
    typename constraint<
      is_executor<Executor>::value || execution::is_executor<Executor>::value
    >::type)
  ASIO_INITFN_AUTO_RESULT_TYPE_SUFFIX((
    async_initiate<CompletionToken,
      typename detail::spawn_signature<
        typename result_of<F(basic_yield_context<Executor>)>::type>::type>(
          declval<detail::initiate_spawn<Executor> >(),
          token, allocator_arg_t(),
          ASIO_MOVE_CAST(StackAllocator)(stack_allocator),
          ASIO_MOVE_CAST(F)(function))))
{
  return async_initiate<CompletionToken,
    typename detail::spawn_signature<
      typename result_of<F(basic_yield_context<Executor>)>::type>::type>(
        detail::initiate_spawn<Executor>(ex), token, allocator_arg_t(),
        ASIO_MOVE_CAST(StackAllocator)(stack_allocator),
        ASIO_MOVE_CAST(F)(function));
}

template <typename ExecutionContext, typename StackAllocator, typename F,
    ASIO_COMPLETION_TOKEN_FOR(typename detail::spawn_signature<
      typename result_of<F(basic_yield_context<
        typename ExecutionContext::executor_type>)>::type>::type)
          CompletionToken>
inline ASIO_INITFN_AUTO_RESULT_TYPE_PREFIX(CompletionToken,
    typename detail::spawn_signature<
      typename result_of<F(basic_yield_context<
        typename ExecutionContext::executor_type>)>::type>::type)
spawn(ExecutionContext& ctx, allocator_arg_t,
    ASIO_MOVE_ARG(StackAllocator) stack_allocator,
    ASIO_MOVE_ARG(F) function,
    ASIO_MOVE_ARG(CompletionToken) token,
    typename constraint<
      is_convertible<ExecutionContext&, execution_context&>::value
    >::type)
  ASIO_INITFN_AUTO_RESULT_TYPE_SUFFIX((
    async_initiate<CompletionToken,
      typename detail::spawn_signature<
        typename result_of<F(basic_yield_context<
          typename ExecutionContext::executor_type>)>::type>::type>(
            declval<detail::initiate_spawn<
              typename ExecutionContext::executor_type> >(),
            token, allocator_arg_t(),
            ASIO_MOVE_CAST(StackAllocator)(stack_allocator),
            ASIO_MOVE_CAST(F)(function))))
{
  return (spawn)(ctx.get_executor(), allocator_arg_t(),
      ASIO_MOVE_CAST(StackAllocator)(stack_allocator),
      ASIO_MOVE_CAST(F)(function),
      ASIO_MOVE_CAST(CompletionToken)(token));
}

template <typename Executor, typename StackAllocator, typename F,
    ASIO_COMPLETION_TOKEN_FOR(typename detail::spawn_signature<
      typename result_of<F(basic_yield_context<Executor>)>::type>::type)
        CompletionToken>
inline ASIO_INITFN_AUTO_RESULT_TYPE_PREFIX(CompletionToken,
    typename detail::spawn_signature<
      typename result_of<F(basic_yield_context<Executor>)>::type>::type)
spawn(const basic_yield_context<Executor>& ctx, allocator_arg_t,
    ASIO_MOVE_ARG(StackAllocator) stack_allocator,
    ASIO_MOVE_ARG(F) function,
    ASIO_MOVE_ARG(CompletionToken) token,
    typename constraint<
      is_executor<Executor>::value || execution::is_executor<Executor>::value
    >::type)
  ASIO_INITFN_AUTO_RESULT_TYPE_SUFFIX((
    async_initiate<CompletionToken,
      typename detail::spawn_signature<
        typename result_of<F(basic_yield_context<Executor>)>::type>::type>(
          declval<detail::initiate_spawn<Executor> >(),
          token, allocator_arg_t(),
          ASIO_MOVE_CAST(StackAllocator)(stack_allocator),
          ASIO_MOVE_CAST(F)(function))))
{
  return (spawn)(ctx.get_executor(), allocator_arg_t(),
      ASIO_MOVE_CAST(StackAllocator)(stack_allocator),
      ASIO_MOVE_CAST(F)(function),
      ASIO_MOVE_CAST(CompletionToken)(token));
}

#endif // defined(ASIO_HAS_BOOST_CONTEXT_FIBER)

#if defined(ASIO_HAS_BOOST_COROUTINE)

namespace detail {

template <typename Executor, typename Function, typename Handler>
class old_spawn_entry_point
{
public:
  template <typename F, typename H>
  old_spawn_entry_point(const Executor& ex,
      ASIO_MOVE_ARG(F) f, ASIO_MOVE_ARG(H) h)
    : executor_(ex),
      function_(ASIO_MOVE_CAST(F)(f)),
      handler_(ASIO_MOVE_CAST(H)(h))
  {
  }

  void operator()(spawned_thread_base* spawned_thread)
  {
    const basic_yield_context<Executor> yield(spawned_thread, executor_);
    this->call(yield,
        void_type<typename result_of<Function(
          basic_yield_context<Executor>)>::type>());
  }

private:
  void call(const basic_yield_context<Executor>& yield, void_type<void>)
  {
    function_(yield);
    ASIO_MOVE_OR_LVALUE(Handler)(handler_)();
  }

  template <typename T>
  void call(const basic_yield_context<Executor>& yield, void_type<T>)
  {
    ASIO_MOVE_OR_LVALUE(Handler)(handler_)(function_(yield));
  }

  Executor executor_;
  Function function_;
  Handler handler_;
};

inline void default_spawn_handler() {}

} // namespace detail

template <typename Function>
inline void spawn(ASIO_MOVE_ARG(Function) function,
    const boost::coroutines::attributes& attributes)
{
  typedef typename decay<Function>::type function_type;

  typename associated_executor<function_type>::type ex(
      (get_associated_executor)(function));

  asio::spawn(ex, ASIO_MOVE_CAST(Function)(function), attributes);
}

template <typename Handler, typename Function>
void spawn(ASIO_MOVE_ARG(Handler) handler,
    ASIO_MOVE_ARG(Function) function,
    const boost::coroutines::attributes& attributes,
    typename constraint<
      !is_executor<typename decay<Handler>::type>::value &&
      !execution::is_executor<typename decay<Handler>::type>::value &&
      !is_convertible<Handler&, execution_context&>::value>::type)
{
  typedef typename decay<Handler>::type handler_type;
  typedef typename decay<Function>::type function_type;
  typedef typename associated_executor<handler_type>::type executor_type;

  executor_type ex((get_associated_executor)(handler));

  (dispatch)(ex,
      detail::spawned_thread_resumer(
        detail::spawned_coroutine_thread::spawn(
          detail::old_spawn_entry_point<executor_type,
            function_type, void (*)()>(
              ex, ASIO_MOVE_CAST(Function)(function),
              &detail::default_spawn_handler), attributes)));
}

template <typename Executor, typename Function>
void spawn(basic_yield_context<Executor> ctx,
    ASIO_MOVE_ARG(Function) function,
    const boost::coroutines::attributes& attributes)
{
  typedef typename decay<Function>::type function_type;

  (dispatch)(ctx.get_executor(),
      detail::spawned_thread_resumer(
        detail::spawned_coroutine_thread::spawn(
          detail::old_spawn_entry_point<Executor,
            function_type, void (*)()>(ctx.get_executor(),
              ASIO_MOVE_CAST(Function)(function),
              &detail::default_spawn_handler), attributes)));
}

template <typename Function, typename Executor>
inline void spawn(const Executor& ex,
    ASIO_MOVE_ARG(Function) function,
    const boost::coroutines::attributes& attributes,
    typename constraint<
      is_executor<Executor>::value || execution::is_executor<Executor>::value
    >::type)
{
  asio::spawn(asio::strand<Executor>(ex),
      ASIO_MOVE_CAST(Function)(function), attributes);
}

template <typename Function, typename Executor>
inline void spawn(const strand<Executor>& ex,
    ASIO_MOVE_ARG(Function) function,
    const boost::coroutines::attributes& attributes)
{
  asio::spawn(asio::bind_executor(
        ex, &detail::default_spawn_handler),
      ASIO_MOVE_CAST(Function)(function), attributes);
}

#if !defined(ASIO_NO_TS_EXECUTORS)

template <typename Function>
inline void spawn(const asio::io_context::strand& s,
    ASIO_MOVE_ARG(Function) function,
    const boost::coroutines::attributes& attributes)
{
  asio::spawn(asio::bind_executor(
        s, &detail::default_spawn_handler),
      ASIO_MOVE_CAST(Function)(function), attributes);
}

#endif // !defined(ASIO_NO_TS_EXECUTORS)

template <typename Function, typename ExecutionContext>
inline void spawn(ExecutionContext& ctx,
    ASIO_MOVE_ARG(Function) function,
    const boost::coroutines::attributes& attributes,
    typename constraint<is_convertible<
      ExecutionContext&, execution_context&>::value>::type)
{
  asio::spawn(ctx.get_executor(),
      ASIO_MOVE_CAST(Function)(function), attributes);
}

#endif // defined(ASIO_HAS_BOOST_COROUTINE)

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_IMPL_SPAWN_HPP
