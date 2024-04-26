//
// compose.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_COMPOSE_HPP
#define ASIO_COMPOSE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/associated_executor.hpp"
#include "asio/async_result.hpp"
#include "asio/detail/base_from_cancellation_state.hpp"
#include "asio/detail/composed_work.hpp"
#include "asio/detail/handler_cont_helpers.hpp"
#include "asio/detail/type_traits.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

template <typename Impl, typename Work, typename Handler, typename Signature>
class composed_op;

template <typename Impl, typename Work, typename Handler,
    typename R, typename... Args>
class composed_op<Impl, Work, Handler, R(Args...)>
  : public base_from_cancellation_state<Handler>
{
public:
  template <typename I, typename W, typename H>
  composed_op(I&& impl,
      W&& work,
      H&& handler)
    : base_from_cancellation_state<Handler>(
        handler, enable_terminal_cancellation()),
      impl_(static_cast<I&&>(impl)),
      work_(static_cast<W&&>(work)),
      handler_(static_cast<H&&>(handler)),
      invocations_(0)
  {
  }

  composed_op(composed_op&& other)
    : base_from_cancellation_state<Handler>(
        static_cast<base_from_cancellation_state<Handler>&&>(other)),
      impl_(static_cast<Impl&&>(other.impl_)),
      work_(static_cast<Work&&>(other.work_)),
      handler_(static_cast<Handler&&>(other.handler_)),
      invocations_(other.invocations_)
  {
  }

  typedef typename composed_work_guard<
    typename Work::head_type>::executor_type io_executor_type;

  io_executor_type get_io_executor() const noexcept
  {
    return work_.head_.get_executor();
  }

  typedef associated_executor_t<Handler, io_executor_type> executor_type;

  executor_type get_executor() const noexcept
  {
    return (get_associated_executor)(handler_, work_.head_.get_executor());
  }

  typedef associated_allocator_t<Handler, std::allocator<void>> allocator_type;

  allocator_type get_allocator() const noexcept
  {
    return (get_associated_allocator)(handler_, std::allocator<void>());
  }

  template<typename... T>
  void operator()(T&&... t)
  {
    if (invocations_ < ~0u)
      ++invocations_;
    this->get_cancellation_state().slot().clear();
    impl_(*this, static_cast<T&&>(t)...);
  }

  void complete(Args... args)
  {
    this->work_.reset();
    static_cast<Handler&&>(this->handler_)(static_cast<Args&&>(args)...);
  }

  void reset_cancellation_state()
  {
    base_from_cancellation_state<Handler>::reset_cancellation_state(handler_);
  }

  template <typename Filter>
  void reset_cancellation_state(Filter&& filter)
  {
    base_from_cancellation_state<Handler>::reset_cancellation_state(handler_,
        static_cast<Filter&&>(filter));
  }

  template <typename InFilter, typename OutFilter>
  void reset_cancellation_state(InFilter&& in_filter,
      OutFilter&& out_filter)
  {
    base_from_cancellation_state<Handler>::reset_cancellation_state(handler_,
        static_cast<InFilter&&>(in_filter),
        static_cast<OutFilter&&>(out_filter));
  }

  cancellation_type_t cancelled() const noexcept
  {
    return base_from_cancellation_state<Handler>::cancelled();
  }

//private:
  Impl impl_;
  Work work_;
  Handler handler_;
  unsigned invocations_;
};

template <typename Impl, typename Work, typename Handler, typename Signature>
inline bool asio_handler_is_continuation(
    composed_op<Impl, Work, Handler, Signature>* this_handler)
{
  return this_handler->invocations_ > 1 ? true
    : asio_handler_cont_helpers::is_continuation(
        this_handler->handler_);
}

template <typename Signature, typename Executors>
class initiate_composed_op
{
public:
  typedef typename composed_io_executors<Executors>::head_type executor_type;

  template <typename T>
  explicit initiate_composed_op(int, T&& executors)
    : executors_(static_cast<T&&>(executors))
  {
  }

  executor_type get_executor() const noexcept
  {
    return executors_.head_;
  }

  template <typename Handler, typename Impl>
  void operator()(Handler&& handler,
      Impl&& impl) const
  {
    composed_op<decay_t<Impl>, composed_work<Executors>,
      decay_t<Handler>, Signature>(
        static_cast<Impl&&>(impl),
        composed_work<Executors>(executors_),
        static_cast<Handler&&>(handler))();
  }

private:
  composed_io_executors<Executors> executors_;
};

template <typename Signature, typename Executors>
inline initiate_composed_op<Signature, Executors> make_initiate_composed_op(
    composed_io_executors<Executors>&& executors)
{
  return initiate_composed_op<Signature, Executors>(0,
      static_cast<composed_io_executors<Executors>&&>(executors));
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
      const detail::composed_op<Impl, Work, Handler, Signature>& h) noexcept
  {
    return Associator<Handler, DefaultCandidate>::get(h.handler_);
  }

  static auto get(const detail::composed_op<Impl, Work, Handler, Signature>& h,
      const DefaultCandidate& c) noexcept
    -> decltype(Associator<Handler, DefaultCandidate>::get(h.handler_, c))
  {
    return Associator<Handler, DefaultCandidate>::get(h.handler_, c);
  }
};

#endif // !defined(GENERATING_DOCUMENTATION)

/// Launch an asynchronous operation with a stateful implementation.
/**
 * The async_compose function simplifies the implementation of composed
 * asynchronous operations automatically wrapping a stateful function object
 * with a conforming intermediate completion handler.
 *
 * @param implementation A function object that contains the implementation of
 * the composed asynchronous operation. The first argument to the function
 * object is a non-const reference to the enclosing intermediate completion
 * handler. The remaining arguments are any arguments that originate from the
 * completion handlers of any asynchronous operations performed by the
 * implementation.
 *
 * @param token The completion token.
 *
 * @param io_objects_or_executors Zero or more I/O objects or I/O executors for
 * which outstanding work must be maintained.
 *
 * @par Per-Operation Cancellation
 * By default, terminal per-operation cancellation is enabled for
 * composed operations that are implemented using @c async_compose. To
 * disable cancellation for the composed operation, or to alter its
 * supported cancellation types, call the @c self object's @c
 * reset_cancellation_state function.
 *
 * @par Example:
 *
 * @code struct async_echo_implementation
 * {
 *   tcp::socket& socket_;
 *   asio::mutable_buffer buffer_;
 *   enum { starting, reading, writing } state_;
 *
 *   template <typename Self>
 *   void operator()(Self& self,
 *       asio::error_code error = {},
 *       std::size_t n = 0)
 *   {
 *     switch (state_)
 *     {
 *     case starting:
 *       state_ = reading;
 *       socket_.async_read_some(
 *           buffer_, std::move(self));
 *       break;
 *     case reading:
 *       if (error)
 *       {
 *         self.complete(error, 0);
 *       }
 *       else
 *       {
 *         state_ = writing;
 *         asio::async_write(socket_, buffer_,
 *             asio::transfer_exactly(n),
 *             std::move(self));
 *       }
 *       break;
 *     case writing:
 *       self.complete(error, n);
 *       break;
 *     }
 *   }
 * };
 *
 * template <typename CompletionToken>
 * auto async_echo(tcp::socket& socket,
 *     asio::mutable_buffer buffer,
 *     CompletionToken&& token) ->
 *   decltype(
 *     asio::async_compose<CompletionToken,
 *       void(asio::error_code, std::size_t)>(
 *         std::declval<async_echo_implementation>(),
 *         token, socket))
 * {
 *   return asio::async_compose<CompletionToken,
 *     void(asio::error_code, std::size_t)>(
 *       async_echo_implementation{socket, buffer,
 *         async_echo_implementation::starting},
 *       token, socket);
 * } @endcode
 */
template <typename CompletionToken, typename Signature,
    typename Implementation, typename... IoObjectsOrExecutors>
auto async_compose(Implementation&& implementation,
    type_identity_t<CompletionToken>& token,
    IoObjectsOrExecutors&&... io_objects_or_executors)
  -> decltype(
    async_initiate<CompletionToken, Signature>(
      detail::make_initiate_composed_op<Signature>(
        detail::make_composed_io_executors(
          detail::get_composed_io_executor(
            static_cast<IoObjectsOrExecutors&&>(
              io_objects_or_executors))...)),
      token, static_cast<Implementation&&>(implementation)))
{
  return async_initiate<CompletionToken, Signature>(
      detail::make_initiate_composed_op<Signature>(
        detail::make_composed_io_executors(
          detail::get_composed_io_executor(
            static_cast<IoObjectsOrExecutors&&>(
              io_objects_or_executors))...)),
      token, static_cast<Implementation&&>(implementation));
}

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_COMPOSE_HPP
