//
// impl/compose.hpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_IMPL_COMPOSE_HPP
#define ASIO_IMPL_COMPOSE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/associated_executor.hpp"
#include "asio/detail/base_from_cancellation_state.hpp"
#include "asio/detail/handler_alloc_helpers.hpp"
#include "asio/detail/handler_cont_helpers.hpp"
#include "asio/detail/handler_invoke_helpers.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/detail/variadic_templates.hpp"
#include "asio/execution/executor.hpp"
#include "asio/execution/outstanding_work.hpp"
#include "asio/executor_work_guard.hpp"
#include "asio/is_executor.hpp"
#include "asio/system_executor.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {

namespace detail
{
  template <typename Executor, typename = void>
  class composed_work_guard
  {
  public:
    typedef typename decay<
        typename prefer_result<Executor,
          execution::outstanding_work_t::tracked_t
        >::type
      >::type executor_type;

    composed_work_guard(const Executor& ex)
      : executor_(asio::prefer(ex, execution::outstanding_work.tracked))
    {
    }

    void reset()
    {
    }

    executor_type get_executor() const ASIO_NOEXCEPT
    {
      return executor_;
    }

  private:
    executor_type executor_;
  };

#if !defined(ASIO_NO_TS_EXECUTORS)

  template <typename Executor>
  struct composed_work_guard<Executor,
      typename enable_if<
        !execution::is_executor<Executor>::value
      >::type> : executor_work_guard<Executor>
  {
    composed_work_guard(const Executor& ex)
      : executor_work_guard<Executor>(ex)
    {
    }
  };

#endif // !defined(ASIO_NO_TS_EXECUTORS)

  template <typename>
  struct composed_io_executors;

  template <>
  struct composed_io_executors<void()>
  {
    composed_io_executors() ASIO_NOEXCEPT
      : head_(system_executor())
    {
    }

    typedef system_executor head_type;
    system_executor head_;
  };

  inline composed_io_executors<void()> make_composed_io_executors()
  {
    return composed_io_executors<void()>();
  }

  template <typename Head>
  struct composed_io_executors<void(Head)>
  {
    explicit composed_io_executors(const Head& ex) ASIO_NOEXCEPT
      : head_(ex)
    {
    }

    typedef Head head_type;
    Head head_;
  };

  template <typename Head>
  inline composed_io_executors<void(Head)>
  make_composed_io_executors(const Head& head)
  {
    return composed_io_executors<void(Head)>(head);
  }

#if defined(ASIO_HAS_VARIADIC_TEMPLATES)

  template <typename Head, typename... Tail>
  struct composed_io_executors<void(Head, Tail...)>
  {
    explicit composed_io_executors(const Head& head,
        const Tail&... tail) ASIO_NOEXCEPT
      : head_(head),
        tail_(tail...)
    {
    }

    void reset()
    {
      head_.reset();
      tail_.reset();
    }

    typedef Head head_type;
    Head head_;
    composed_io_executors<void(Tail...)> tail_;
  };

  template <typename Head, typename... Tail>
  inline composed_io_executors<void(Head, Tail...)>
  make_composed_io_executors(const Head& head, const Tail&... tail)
  {
    return composed_io_executors<void(Head, Tail...)>(head, tail...);
  }

#else // defined(ASIO_HAS_VARIADIC_TEMPLATES)

#define ASIO_PRIVATE_COMPOSED_IO_EXECUTORS_DEF(n) \
  template <typename Head, ASIO_VARIADIC_TPARAMS(n)> \
  struct composed_io_executors<void(Head, ASIO_VARIADIC_TARGS(n))> \
  { \
    explicit composed_io_executors(const Head& head, \
        ASIO_VARIADIC_CONSTREF_PARAMS(n)) ASIO_NOEXCEPT \
      : head_(head), \
        tail_(ASIO_VARIADIC_BYVAL_ARGS(n)) \
    { \
    } \
  \
    void reset() \
    { \
      head_.reset(); \
      tail_.reset(); \
    } \
  \
    typedef Head head_type; \
    Head head_; \
    composed_io_executors<void(ASIO_VARIADIC_TARGS(n))> tail_; \
  }; \
  \
  template <typename Head, ASIO_VARIADIC_TPARAMS(n)> \
  inline composed_io_executors<void(Head, ASIO_VARIADIC_TARGS(n))> \
  make_composed_io_executors(const Head& head, \
      ASIO_VARIADIC_CONSTREF_PARAMS(n)) \
  { \
    return composed_io_executors< \
      void(Head, ASIO_VARIADIC_TARGS(n))>( \
        head, ASIO_VARIADIC_BYVAL_ARGS(n)); \
  } \
  /**/
  ASIO_VARIADIC_GENERATE(ASIO_PRIVATE_COMPOSED_IO_EXECUTORS_DEF)
#undef ASIO_PRIVATE_COMPOSED_IO_EXECUTORS_DEF

#endif // defined(ASIO_HAS_VARIADIC_TEMPLATES)

  template <typename>
  struct composed_work;

  template <>
  struct composed_work<void()>
  {
    typedef composed_io_executors<void()> executors_type;

    composed_work(const executors_type&) ASIO_NOEXCEPT
      : head_(system_executor())
    {
    }

    void reset()
    {
      head_.reset();
    }

    typedef system_executor head_type;
    composed_work_guard<system_executor> head_;
  };

  template <typename Head>
  struct composed_work<void(Head)>
  {
    typedef composed_io_executors<void(Head)> executors_type;

    explicit composed_work(const executors_type& ex) ASIO_NOEXCEPT
      : head_(ex.head_)
    {
    }

    void reset()
    {
      head_.reset();
    }

    typedef Head head_type;
    composed_work_guard<Head> head_;
  };

#if defined(ASIO_HAS_VARIADIC_TEMPLATES)

  template <typename Head, typename... Tail>
  struct composed_work<void(Head, Tail...)>
  {
    typedef composed_io_executors<void(Head, Tail...)> executors_type;

    explicit composed_work(const executors_type& ex) ASIO_NOEXCEPT
      : head_(ex.head_),
        tail_(ex.tail_)
    {
    }

    void reset()
    {
      head_.reset();
      tail_.reset();
    }

    typedef Head head_type;
    composed_work_guard<Head> head_;
    composed_work<void(Tail...)> tail_;
  };

#else // defined(ASIO_HAS_VARIADIC_TEMPLATES)

#define ASIO_PRIVATE_COMPOSED_WORK_DEF(n) \
  template <typename Head, ASIO_VARIADIC_TPARAMS(n)> \
  struct composed_work<void(Head, ASIO_VARIADIC_TARGS(n))> \
  { \
    typedef composed_io_executors<void(Head, \
      ASIO_VARIADIC_TARGS(n))> executors_type; \
  \
    explicit composed_work(const executors_type& ex) ASIO_NOEXCEPT \
      : head_(ex.head_), \
        tail_(ex.tail_) \
    { \
    } \
  \
    void reset() \
    { \
      head_.reset(); \
      tail_.reset(); \
    } \
  \
    typedef Head head_type; \
    composed_work_guard<Head> head_; \
    composed_work<void(ASIO_VARIADIC_TARGS(n))> tail_; \
  }; \
  /**/
  ASIO_VARIADIC_GENERATE(ASIO_PRIVATE_COMPOSED_WORK_DEF)
#undef ASIO_PRIVATE_COMPOSED_WORK_DEF

#endif // defined(ASIO_HAS_VARIADIC_TEMPLATES)

#if defined(ASIO_HAS_VARIADIC_TEMPLATES)
  template <typename Impl, typename Work, typename Handler, typename Signature>
  class composed_op;

  template <typename Impl, typename Work, typename Handler,
      typename R, typename... Args>
  class composed_op<Impl, Work, Handler, R(Args...)>
#else // defined(ASIO_HAS_VARIADIC_TEMPLATES)
  template <typename Impl, typename Work, typename Handler, typename Signature>
  class composed_op
#endif // defined(ASIO_HAS_VARIADIC_TEMPLATES)
    : public base_from_cancellation_state<Handler>
  {
  public:
    template <typename I, typename W, typename H>
    composed_op(ASIO_MOVE_ARG(I) impl,
        ASIO_MOVE_ARG(W) work,
        ASIO_MOVE_ARG(H) handler)
      : base_from_cancellation_state<Handler>(
          handler, enable_terminal_cancellation()),
        impl_(ASIO_MOVE_CAST(I)(impl)),
        work_(ASIO_MOVE_CAST(W)(work)),
        handler_(ASIO_MOVE_CAST(H)(handler)),
        invocations_(0)
    {
    }

#if defined(ASIO_HAS_MOVE)
    composed_op(composed_op&& other)
      : base_from_cancellation_state<Handler>(
          ASIO_MOVE_CAST(base_from_cancellation_state<
            Handler>)(other)),
        impl_(ASIO_MOVE_CAST(Impl)(other.impl_)),
        work_(ASIO_MOVE_CAST(Work)(other.work_)),
        handler_(ASIO_MOVE_CAST(Handler)(other.handler_)),
        invocations_(other.invocations_)
    {
    }
#endif // defined(ASIO_HAS_MOVE)

    typedef typename associated_executor<Handler,
        typename composed_work_guard<
          typename Work::head_type
        >::executor_type
      >::type executor_type;

    executor_type get_executor() const ASIO_NOEXCEPT
    {
      return (get_associated_executor)(handler_, work_.head_.get_executor());
    }

    typedef typename associated_allocator<Handler,
      std::allocator<void> >::type allocator_type;

    allocator_type get_allocator() const ASIO_NOEXCEPT
    {
      return (get_associated_allocator)(handler_, std::allocator<void>());
    }

#if defined(ASIO_HAS_VARIADIC_TEMPLATES)

    template<typename... T>
    void operator()(ASIO_MOVE_ARG(T)... t)
    {
      if (invocations_ < ~0u)
        ++invocations_;
      this->get_cancellation_state().slot().clear();
      impl_(*this, ASIO_MOVE_CAST(T)(t)...);
    }

    void complete(Args... args)
    {
      this->work_.reset();
      ASIO_MOVE_OR_LVALUE(Handler)(this->handler_)(
          ASIO_MOVE_CAST(Args)(args)...);
    }

#else // defined(ASIO_HAS_VARIADIC_TEMPLATES)

    void operator()()
    {
      if (invocations_ < ~0u)
        ++invocations_;
      this->get_cancellation_state().slot().clear();
      impl_(*this);
    }

    void complete()
    {
      this->work_.reset();
      ASIO_MOVE_OR_LVALUE(Handler)(this->handler_)();
    }

#define ASIO_PRIVATE_COMPOSED_OP_DEF(n) \
    template<ASIO_VARIADIC_TPARAMS(n)> \
    void operator()(ASIO_VARIADIC_MOVE_PARAMS(n)) \
    { \
      if (invocations_ < ~0u) \
        ++invocations_; \
      this->get_cancellation_state().slot().clear(); \
      impl_(*this, ASIO_VARIADIC_MOVE_ARGS(n)); \
    } \
    \
    template<ASIO_VARIADIC_TPARAMS(n)> \
    void complete(ASIO_VARIADIC_MOVE_PARAMS(n)) \
    { \
      this->work_.reset(); \
      ASIO_MOVE_OR_LVALUE(Handler)(this->handler_)( \
          ASIO_VARIADIC_MOVE_ARGS(n)); \
    } \
    /**/
    ASIO_VARIADIC_GENERATE(ASIO_PRIVATE_COMPOSED_OP_DEF)
#undef ASIO_PRIVATE_COMPOSED_OP_DEF

#endif // defined(ASIO_HAS_VARIADIC_TEMPLATES)

    void reset_cancellation_state()
    {
      base_from_cancellation_state<Handler>::reset_cancellation_state(handler_);
    }

    template <typename Filter>
    void reset_cancellation_state(ASIO_MOVE_ARG(Filter) filter)
    {
      base_from_cancellation_state<Handler>::reset_cancellation_state(handler_,
          ASIO_MOVE_CAST(Filter)(filter));
    }

    template <typename InFilter, typename OutFilter>
    void reset_cancellation_state(ASIO_MOVE_ARG(InFilter) in_filter,
        ASIO_MOVE_ARG(OutFilter) out_filter)
    {
      base_from_cancellation_state<Handler>::reset_cancellation_state(handler_,
          ASIO_MOVE_CAST(InFilter)(in_filter),
          ASIO_MOVE_CAST(OutFilter)(out_filter));
    }

  //private:
    Impl impl_;
    Work work_;
    Handler handler_;
    unsigned invocations_;
  };

  template <typename Impl, typename Work, typename Handler, typename Signature>
  inline asio_handler_allocate_is_deprecated
  asio_handler_allocate(std::size_t size,
      composed_op<Impl, Work, Handler, Signature>* this_handler)
  {
#if defined(ASIO_NO_DEPRECATED)
    asio_handler_alloc_helpers::allocate(size, this_handler->handler_);
    return asio_handler_allocate_is_no_longer_used();
#else // defined(ASIO_NO_DEPRECATED)
    return asio_handler_alloc_helpers::allocate(
        size, this_handler->handler_);
#endif // defined(ASIO_NO_DEPRECATED)
  }

  template <typename Impl, typename Work, typename Handler, typename Signature>
  inline asio_handler_deallocate_is_deprecated
  asio_handler_deallocate(void* pointer, std::size_t size,
      composed_op<Impl, Work, Handler, Signature>* this_handler)
  {
    asio_handler_alloc_helpers::deallocate(
        pointer, size, this_handler->handler_);
#if defined(ASIO_NO_DEPRECATED)
    return asio_handler_deallocate_is_no_longer_used();
#endif // defined(ASIO_NO_DEPRECATED)
  }

  template <typename Impl, typename Work, typename Handler, typename Signature>
  inline bool asio_handler_is_continuation(
      composed_op<Impl, Work, Handler, Signature>* this_handler)
  {
    return this_handler->invocations_ > 1 ? true
      : asio_handler_cont_helpers::is_continuation(
          this_handler->handler_);
  }

  template <typename Function, typename Impl,
      typename Work, typename Handler, typename Signature>
  inline asio_handler_invoke_is_deprecated
  asio_handler_invoke(Function& function,
      composed_op<Impl, Work, Handler, Signature>* this_handler)
  {
    asio_handler_invoke_helpers::invoke(
        function, this_handler->handler_);
#if defined(ASIO_NO_DEPRECATED)
    return asio_handler_invoke_is_no_longer_used();
#endif // defined(ASIO_NO_DEPRECATED)
  }

  template <typename Function, typename Impl,
      typename Work, typename Handler, typename Signature>
  inline asio_handler_invoke_is_deprecated
  asio_handler_invoke(const Function& function,
      composed_op<Impl, Work, Handler, Signature>* this_handler)
  {
    asio_handler_invoke_helpers::invoke(
        function, this_handler->handler_);
#if defined(ASIO_NO_DEPRECATED)
    return asio_handler_invoke_is_no_longer_used();
#endif // defined(ASIO_NO_DEPRECATED)
  }

  template <typename Signature, typename Executors>
  class initiate_composed_op
  {
  public:
    typedef typename composed_io_executors<Executors>::head_type executor_type;

    template <typename T>
    explicit initiate_composed_op(int, ASIO_MOVE_ARG(T) executors)
      : executors_(ASIO_MOVE_CAST(T)(executors))
    {
    }

    executor_type get_executor() const ASIO_NOEXCEPT
    {
      return executors_.head_;
    }

    template <typename Handler, typename Impl>
    void operator()(ASIO_MOVE_ARG(Handler) handler,
        ASIO_MOVE_ARG(Impl) impl) const
    {
      composed_op<typename decay<Impl>::type, composed_work<Executors>,
        typename decay<Handler>::type, Signature>(
          ASIO_MOVE_CAST(Impl)(impl),
          composed_work<Executors>(executors_),
          ASIO_MOVE_CAST(Handler)(handler))();
    }

  private:
    composed_io_executors<Executors> executors_;
  };

  template <typename Signature, typename Executors>
  inline initiate_composed_op<Signature, Executors> make_initiate_composed_op(
      ASIO_MOVE_ARG(composed_io_executors<Executors>) executors)
  {
    return initiate_composed_op<Signature, Executors>(0,
        ASIO_MOVE_CAST(composed_io_executors<Executors>)(executors));
  }

  template <typename IoObject>
  inline typename IoObject::executor_type
  get_composed_io_executor(IoObject& io_object,
      typename enable_if<
        !is_executor<IoObject>::value
      >::type* = 0,
      typename enable_if<
        !execution::is_executor<IoObject>::value
      >::type* = 0)
  {
    return io_object.get_executor();
  }

  template <typename Executor>
  inline const Executor& get_composed_io_executor(const Executor& ex,
      typename enable_if<
        is_executor<Executor>::value
          || execution::is_executor<Executor>::value
      >::type* = 0)
  {
    return ex;
  }
} // namespace detail

#if !defined(GENERATING_DOCUMENTATION)

template <template <typename, typename> class Associator,
    typename Impl, typename Work, typename Handler,
    typename Signature, typename DefaultCandidate>
struct associator<Associator,
    detail::composed_op<Impl, Work, Handler, Signature>,
    DefaultCandidate>
  : Associator<Handler, DefaultCandidate>
{
  static typename Associator<Handler, DefaultCandidate>::type get(
      const detail::composed_op<Impl, Work, Handler, Signature>& h,
      const DefaultCandidate& c = DefaultCandidate()) ASIO_NOEXCEPT
  {
    return Associator<Handler, DefaultCandidate>::get(h.handler_, c);
  }
};

#if defined(ASIO_HAS_VARIADIC_TEMPLATES)

template <typename CompletionToken, typename Signature,
    typename Implementation, typename... IoObjectsOrExecutors>
ASIO_INITFN_AUTO_RESULT_TYPE(CompletionToken, Signature)
async_compose(ASIO_MOVE_ARG(Implementation) implementation,
    ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token,
    ASIO_MOVE_ARG(IoObjectsOrExecutors)... io_objects_or_executors)
{
  return async_initiate<CompletionToken, Signature>(
      detail::make_initiate_composed_op<Signature>(
        detail::make_composed_io_executors(
          detail::get_composed_io_executor(
            ASIO_MOVE_CAST(IoObjectsOrExecutors)(
              io_objects_or_executors))...)),
      token, ASIO_MOVE_CAST(Implementation)(implementation));
}

#else // defined(ASIO_HAS_VARIADIC_TEMPLATES)

template <typename CompletionToken, typename Signature, typename Implementation>
ASIO_INITFN_AUTO_RESULT_TYPE(CompletionToken, Signature)
async_compose(ASIO_MOVE_ARG(Implementation) implementation,
    ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token)
{
  return async_initiate<CompletionToken, Signature>(
      detail::make_initiate_composed_op<Signature>(
        detail::make_composed_io_executors()),
      token, ASIO_MOVE_CAST(Implementation)(implementation));
}

# define ASIO_PRIVATE_GET_COMPOSED_IO_EXECUTOR(n) \
  ASIO_PRIVATE_GET_COMPOSED_IO_EXECUTOR_##n

# define ASIO_PRIVATE_GET_COMPOSED_IO_EXECUTOR_1 \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T1)(x1))
# define ASIO_PRIVATE_GET_COMPOSED_IO_EXECUTOR_2 \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T1)(x1)), \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T2)(x2))
# define ASIO_PRIVATE_GET_COMPOSED_IO_EXECUTOR_3 \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T1)(x1)), \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T2)(x2)), \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T3)(x3))
# define ASIO_PRIVATE_GET_COMPOSED_IO_EXECUTOR_4 \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T1)(x1)), \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T2)(x2)), \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T3)(x3)), \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T4)(x4))
# define ASIO_PRIVATE_GET_COMPOSED_IO_EXECUTOR_5 \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T1)(x1)), \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T2)(x2)), \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T3)(x3)), \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T4)(x4)), \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T5)(x5))
# define ASIO_PRIVATE_GET_COMPOSED_IO_EXECUTOR_6 \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T1)(x1)), \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T2)(x2)), \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T3)(x3)), \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T4)(x4)), \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T5)(x5)), \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T6)(x6))
# define ASIO_PRIVATE_GET_COMPOSED_IO_EXECUTOR_7 \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T1)(x1)), \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T2)(x2)), \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T3)(x3)), \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T4)(x4)), \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T5)(x5)), \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T6)(x6)), \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T7)(x7))
# define ASIO_PRIVATE_GET_COMPOSED_IO_EXECUTOR_8 \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T1)(x1)), \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T2)(x2)), \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T3)(x3)), \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T4)(x4)), \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T5)(x5)), \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T6)(x6)), \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T7)(x7)), \
  detail::get_composed_io_executor(ASIO_MOVE_CAST(T8)(x8))

#define ASIO_PRIVATE_ASYNC_COMPOSE_DEF(n) \
  template <typename CompletionToken, typename Signature, \
      typename Implementation, ASIO_VARIADIC_TPARAMS(n)> \
  ASIO_INITFN_AUTO_RESULT_TYPE(CompletionToken, Signature) \
  async_compose(ASIO_MOVE_ARG(Implementation) implementation, \
      ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token, \
      ASIO_VARIADIC_MOVE_PARAMS(n)) \
  { \
    return async_initiate<CompletionToken, Signature>( \
        detail::make_initiate_composed_op<Signature>( \
          detail::make_composed_io_executors( \
            ASIO_PRIVATE_GET_COMPOSED_IO_EXECUTOR(n))), \
        token, ASIO_MOVE_CAST(Implementation)(implementation)); \
  } \
  /**/
  ASIO_VARIADIC_GENERATE(ASIO_PRIVATE_ASYNC_COMPOSE_DEF)
#undef ASIO_PRIVATE_ASYNC_COMPOSE_DEF

#undef ASIO_PRIVATE_GET_COMPOSED_IO_EXECUTOR
#undef ASIO_PRIVATE_GET_COMPOSED_IO_EXECUTOR_1
#undef ASIO_PRIVATE_GET_COMPOSED_IO_EXECUTOR_2
#undef ASIO_PRIVATE_GET_COMPOSED_IO_EXECUTOR_3
#undef ASIO_PRIVATE_GET_COMPOSED_IO_EXECUTOR_4
#undef ASIO_PRIVATE_GET_COMPOSED_IO_EXECUTOR_5
#undef ASIO_PRIVATE_GET_COMPOSED_IO_EXECUTOR_6
#undef ASIO_PRIVATE_GET_COMPOSED_IO_EXECUTOR_7
#undef ASIO_PRIVATE_GET_COMPOSED_IO_EXECUTOR_8

#endif // defined(ASIO_HAS_VARIADIC_TEMPLATES)
#endif // !defined(GENERATING_DOCUMENTATION)

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_IMPL_COMPOSE_HPP
