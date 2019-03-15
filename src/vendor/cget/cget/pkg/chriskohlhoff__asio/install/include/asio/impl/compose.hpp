//
// impl/compose.hpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2019 Christopher M. Kohlhoff (chris at kohlhoff dot com)
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
#include "asio/detail/handler_alloc_helpers.hpp"
#include "asio/detail/handler_cont_helpers.hpp"
#include "asio/detail/handler_invoke_helpers.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/detail/variadic_templates.hpp"
#include "asio/executor_work_guard.hpp"
#include "asio/is_executor.hpp"
#include "asio/system_executor.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {

namespace detail
{
  template <typename>
  struct composed_work;

  template <>
  struct composed_work<void()>
  {
    composed_work() ASIO_NOEXCEPT
      : head_(system_executor())
    {
    }

    void reset()
    {
      head_.reset();
    }

    typedef system_executor head_type;
    executor_work_guard<system_executor> head_;
  };

  inline composed_work<void()> make_composed_work()
  {
    return composed_work<void()>();
  }

  template <typename Head>
  struct composed_work<void(Head)>
  {
    explicit composed_work(const Head& ex) ASIO_NOEXCEPT
      : head_(ex)
    {
    }

    void reset()
    {
      head_.reset();
    }

    typedef Head head_type;
    executor_work_guard<Head> head_;
  };

  template <typename Head>
  inline composed_work<void(Head)> make_composed_work(const Head& head)
  {
    return composed_work<void(Head)>(head);
  }

#if defined(ASIO_HAS_VARIADIC_TEMPLATES)

  template <typename Head, typename... Tail>
  struct composed_work<void(Head, Tail...)>
  {
    explicit composed_work(const Head& head,
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
    executor_work_guard<Head> head_;
    composed_work<void(Tail...)> tail_;
  };

  template <typename Head, typename... Tail>
  inline composed_work<void(Head, Tail...)>
  make_composed_work(const Head& head, const Tail&... tail)
  {
    return composed_work<void(Head, Tail...)>(head, tail...);
  }

#else // defined(ASIO_HAS_VARIADIC_TEMPLATES)

#define ASIO_PRIVATE_COMPOSED_WORK_DEF(n) \
  template <typename Head, ASIO_VARIADIC_TPARAMS(n)> \
  struct composed_work<void(Head, ASIO_VARIADIC_TARGS(n))> \
  { \
    explicit composed_work(const Head& head, \
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
    executor_work_guard<Head> head_; \
    composed_work<void(ASIO_VARIADIC_TARGS(n))> tail_; \
  }; \
  \
  template <typename Head, ASIO_VARIADIC_TPARAMS(n)> \
  inline composed_work<void(Head, ASIO_VARIADIC_TARGS(n))> \
  make_composed_work(const Head& head, ASIO_VARIADIC_CONSTREF_PARAMS(n)) \
  { \
    return composed_work< \
      void(Head, ASIO_VARIADIC_TARGS(n))>( \
        head, ASIO_VARIADIC_BYVAL_ARGS(n)); \
  } \
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
  {
  public:
    composed_op(ASIO_MOVE_ARG(Impl) impl,
        ASIO_MOVE_ARG(Work) work,
        ASIO_MOVE_ARG(Handler) handler)
      : impl_(ASIO_MOVE_CAST(Impl)(impl)),
        work_(ASIO_MOVE_CAST(Work)(work)),
        handler_(ASIO_MOVE_CAST(Handler)(handler)),
        invocations_(0)
    {
    }

#if defined(ASIO_HAS_MOVE)
    composed_op(composed_op&& other)
      : impl_(ASIO_MOVE_CAST(Impl)(other.impl_)),
        work_(ASIO_MOVE_CAST(Work)(other.work_)),
        handler_(ASIO_MOVE_CAST(Handler)(other.handler_)),
        invocations_(other.invocations_)
    {
    }
#endif // defined(ASIO_HAS_MOVE)

    typedef typename associated_executor<Handler,
        typename Work::head_type>::type executor_type;

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
      if (invocations_ < ~unsigned(0))
        ++invocations_;
      impl_(*this, ASIO_MOVE_CAST(T)(t)...);
    }

    void complete(Args... args)
    {
      this->work_.reset();
      this->handler_(ASIO_MOVE_CAST(Args)(args)...);
    }

#else // defined(ASIO_HAS_VARIADIC_TEMPLATES)

    void operator()()
    {
      if (invocations_ < ~unsigned(0))
        ++invocations_;
      impl_(*this);
    }

    void complete()
    {
      this->work_.reset();
      this->handler_();
    }

#define ASIO_PRIVATE_COMPOSED_OP_DEF(n) \
    template<ASIO_VARIADIC_TPARAMS(n)> \
    void operator()(ASIO_VARIADIC_MOVE_PARAMS(n)) \
    { \
      if (invocations_ < ~unsigned(0)) \
        ++invocations_; \
      impl_(*this, ASIO_VARIADIC_MOVE_ARGS(n)); \
    } \
    \
    template<ASIO_VARIADIC_TPARAMS(n)> \
    void complete(ASIO_VARIADIC_MOVE_PARAMS(n)) \
    { \
      this->work_.reset(); \
      this->handler_(ASIO_VARIADIC_MOVE_ARGS(n)); \
    } \
    /**/
    ASIO_VARIADIC_GENERATE(ASIO_PRIVATE_COMPOSED_OP_DEF)
#undef ASIO_PRIVATE_COMPOSED_OP_DEF

#endif // defined(ASIO_HAS_VARIADIC_TEMPLATES)

  //private:
    Impl impl_;
    Work work_;
    Handler handler_;
    unsigned invocations_;
  };

  template <typename Impl, typename Work, typename Handler, typename Signature>
  inline void* asio_handler_allocate(std::size_t size,
      composed_op<Impl, Work, Handler, Signature>* this_handler)
  {
    return asio_handler_alloc_helpers::allocate(
        size, this_handler->handler_);
  }

  template <typename Impl, typename Work, typename Handler, typename Signature>
  inline void asio_handler_deallocate(void* pointer, std::size_t size,
      composed_op<Impl, Work, Handler, Signature>* this_handler)
  {
    asio_handler_alloc_helpers::deallocate(
        pointer, size, this_handler->handler_);
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
  inline void asio_handler_invoke(Function& function,
      composed_op<Impl, Work, Handler, Signature>* this_handler)
  {
    asio_handler_invoke_helpers::invoke(
        function, this_handler->handler_);
  }

  template <typename Function, typename Impl,
      typename Work, typename Handler, typename Signature>
  inline void asio_handler_invoke(const Function& function,
      composed_op<Impl, Work, Handler, Signature>* this_handler)
  {
    asio_handler_invoke_helpers::invoke(
        function, this_handler->handler_);
  }

  template <typename Signature>
  struct initiate_composed_op
  {
    template <typename Handler, typename Impl, typename Work>
    void operator()(ASIO_MOVE_ARG(Handler) handler,
        ASIO_MOVE_ARG(Impl) impl,
        ASIO_MOVE_ARG(Work) work) const
    {
      composed_op<typename decay<Impl>::type, typename decay<Work>::type,
        typename decay<Handler>::type, Signature>(
          ASIO_MOVE_CAST(Impl)(impl), ASIO_MOVE_CAST(Work)(work),
          ASIO_MOVE_CAST(Handler)(handler))();
    }
  };

  template <typename IoObject>
  inline typename IoObject::executor_type
  get_composed_io_executor(IoObject& io_object)
  {
    return io_object.get_executor();
  }

  template <typename Executor>
  inline const Executor& get_composed_io_executor(const Executor& ex,
      typename enable_if<is_executor<Executor>::value>::type* = 0)
  {
    return ex;
  }
} // namespace detail

#if !defined(GENERATING_DOCUMENTATION)
#if defined(ASIO_HAS_VARIADIC_TEMPLATES)

template <typename CompletionToken, typename Signature,
    typename Implementation, typename... IoObjectsOrExecutors>
ASIO_INITFN_RESULT_TYPE(CompletionToken, Signature)
async_compose(ASIO_MOVE_ARG(Implementation) implementation,
    ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token,
    ASIO_MOVE_ARG(IoObjectsOrExecutors)... io_objects_or_executors)
{
  return async_initiate<CompletionToken, Signature>(
      detail::initiate_composed_op<Signature>(), token,
      ASIO_MOVE_CAST(Implementation)(implementation),
      detail::make_composed_work(
        detail::get_composed_io_executor(
          ASIO_MOVE_CAST(IoObjectsOrExecutors)(
            io_objects_or_executors))...));
}

#else // defined(ASIO_HAS_VARIADIC_TEMPLATES)

template <typename CompletionToken, typename Signature, typename Implementation>
ASIO_INITFN_RESULT_TYPE(CompletionToken, Signature)
async_compose(ASIO_MOVE_ARG(Implementation) implementation,
    ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token)
{
  return async_initiate<CompletionToken, Signature>(
      detail::initiate_composed_op<Signature>(), token,
      ASIO_MOVE_CAST(Implementation)(implementation),
      detail::make_composed_work());
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

#define ASIO_PRIVATE_ASYNC_COMPOSE_DEF(n) \
  template <typename CompletionToken, typename Signature, \
      typename Implementation, ASIO_VARIADIC_TPARAMS(n)> \
  ASIO_INITFN_RESULT_TYPE(CompletionToken, Signature) \
  async_compose(ASIO_MOVE_ARG(Implementation) implementation, \
      ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token, \
      ASIO_VARIADIC_MOVE_PARAMS(n)) \
  { \
    return async_initiate<CompletionToken, Signature>( \
        detail::initiate_composed_op<Signature>(), token, \
        ASIO_MOVE_CAST(Implementation)(implementation), \
        detail::make_composed_work( \
          ASIO_PRIVATE_GET_COMPOSED_IO_EXECUTOR(n))); \
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

#endif // defined(ASIO_HAS_VARIADIC_TEMPLATES)
#endif // !defined(GENERATING_DOCUMENTATION)

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_IMPL_COMPOSE_HPP
