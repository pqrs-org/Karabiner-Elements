//
// experimental/basic_concurrent_channel.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2022 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_EXPERIMENTAL_BASIC_CONCURRENT_CHANNEL_HPP
#define ASIO_EXPERIMENTAL_BASIC_CONCURRENT_CHANNEL_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/non_const_lvalue.hpp"
#include "asio/detail/mutex.hpp"
#include "asio/execution/executor.hpp"
#include "asio/execution_context.hpp"
#include "asio/experimental/detail/channel_send_functions.hpp"
#include "asio/experimental/detail/channel_service.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace experimental {
namespace detail {

} // namespace detail

/// A channel for messages.
template <typename Executor, typename Traits, typename... Signatures>
class basic_concurrent_channel
#if !defined(GENERATING_DOCUMENTATION)
  : public detail::channel_send_functions<
      basic_concurrent_channel<Executor, Traits, Signatures...>,
      Executor, Signatures...>
#endif // !defined(GENERATING_DOCUMENTATION)
{
private:
  class initiate_async_send;
  class initiate_async_receive;
  typedef detail::channel_service<asio::detail::mutex> service_type;
  typedef typename service_type::template implementation_type<
      Traits, Signatures...>::payload_type payload_type;

  template <typename... PayloadSignatures,
      ASIO_COMPLETION_TOKEN_FOR(PayloadSignatures...) CompletionToken>
  auto do_async_receive(detail::channel_payload<PayloadSignatures...>*,
      ASIO_MOVE_ARG(CompletionToken) token)
    -> decltype(
        async_initiate<CompletionToken, PayloadSignatures...>(
          declval<initiate_async_receive>(), token))
  {
    return async_initiate<CompletionToken, PayloadSignatures...>(
        initiate_async_receive(this), token);
  }

public:
  /// The type of the executor associated with the channel.
  typedef Executor executor_type;

  /// Rebinds the channel type to another executor.
  template <typename Executor1>
  struct rebind_executor
  {
    /// The channel type when rebound to the specified executor.
    typedef basic_concurrent_channel<Executor1, Traits, Signatures...> other;
  };

  /// The traits type associated with the channel.
  typedef typename Traits::template rebind<Signatures...>::other traits_type;

  /// Construct a basic_concurrent_channel.
  /**
   * This constructor creates and channel.
   *
   * @param ex The I/O executor that the channel will use, by default, to
   * dispatch handlers for any asynchronous operations performed on the channel.
   *
   * @param max_buffer_size The maximum number of messages that may be buffered
   * in the channel.
   */
  basic_concurrent_channel(const executor_type& ex,
      std::size_t max_buffer_size = 0)
    : service_(&asio::use_service<service_type>(
            basic_concurrent_channel::get_context(ex))),
      impl_(),
      executor_(ex)
  {
    service_->construct(impl_, max_buffer_size);
  }

  /// Construct and open a basic_concurrent_channel.
  /**
   * This constructor creates and opens a channel.
   *
   * @param context An execution context which provides the I/O executor that
   * the channel will use, by default, to dispatch handlers for any asynchronous
   * operations performed on the channel.
   *
   * @param max_buffer_size The maximum number of messages that may be buffered
   * in the channel.
   */
  template <typename ExecutionContext>
  basic_concurrent_channel(ExecutionContext& context,
      std::size_t max_buffer_size = 0,
      typename constraint<
        is_convertible<ExecutionContext&, execution_context&>::value,
        defaulted_constraint
      >::type = defaulted_constraint())
    : service_(&asio::use_service<service_type>(context)),
      impl_(),
      executor_(context.get_executor())
  {
    service_->construct(impl_, max_buffer_size);
  }

#if defined(ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)
  /// Move-construct a basic_concurrent_channel from another.
  /**
   * This constructor moves a channel from one object to another.
   *
   * @param other The other basic_concurrent_channel object from which the move
   * will occur.
   *
   * @note Following the move, the moved-from object is in the same state as if
   * constructed using the @c basic_concurrent_channel(const executor_type&)
   * constructor.
   */
  basic_concurrent_channel(basic_concurrent_channel&& other)
    : service_(other.service_),
      executor_(other.executor_)
  {
    service_->move_construct(impl_, other.impl_);
  }

  /// Move-assign a basic_concurrent_channel from another.
  /**
   * This assignment operator moves a channel from one object to another.
   * Cancels any outstanding asynchronous operations associated with the target
   * object.
   *
   * @param other The other basic_concurrent_channel object from which the move
   * will occur.
   *
   * @note Following the move, the moved-from object is in the same state as if
   * constructed using the @c basic_concurrent_channel(const executor_type&)
   * constructor.
   */
  basic_concurrent_channel& operator=(basic_concurrent_channel&& other)
  {
    if (this != &other)
    {
      service_->move_assign(impl_, *other.service_, other.impl_);
      executor_.~executor_type();
      new (&executor_) executor_type(other.executor_);
      service_ = other.service_;
    }
    return *this;
  }

  // All channels have access to each other's implementations.
  template <typename, typename, typename...>
  friend class basic_concurrent_channel;

  /// Move-construct a basic_concurrent_channel from another.
  /**
   * This constructor moves a channel from one object to another.
   *
   * @param other The other basic_concurrent_channel object from which the move
   * will occur.
   *
   * @note Following the move, the moved-from object is in the same state as if
   * constructed using the @c basic_concurrent_channel(const executor_type&)
   * constructor.
   */
  template <typename Executor1>
  basic_concurrent_channel(
      basic_concurrent_channel<Executor1, Traits, Signatures...>&& other,
      typename constraint<
          is_convertible<Executor1, Executor>::value
      >::type = 0)
    : service_(other.service_),
      executor_(other.executor_)
  {
    service_->move_construct(impl_, *other.service_, other.impl_);
  }

  /// Move-assign a basic_concurrent_channel from another.
  /**
   * This assignment operator moves a channel from one object to another.
   * Cancels any outstanding asynchronous operations associated with the target
   * object.
   *
   * @param other The other basic_concurrent_channel object from which the move
   * will occur.
   *
   * @note Following the move, the moved-from object is in the same state as if
   * constructed using the @c basic_concurrent_channel(const executor_type&)
   * constructor.
   */
  template <typename Executor1>
  typename constraint<
    is_convertible<Executor1, Executor>::value,
    basic_concurrent_channel&
  >::type operator=(
      basic_concurrent_channel<Executor1, Traits, Signatures...>&& other)
  {
    if (this != &other)
    {
      service_->move_assign(impl_, *other.service_, other.impl_);
      executor_.~executor_type();
      new (&executor_) executor_type(other.executor_);
      service_ = other.service_;
    }
    return *this;
  }
#endif // defined(ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)

  /// Destructor.
  ~basic_concurrent_channel()
  {
    service_->destroy(impl_);
  }

  /// Get the executor associated with the object.
  executor_type get_executor() ASIO_NOEXCEPT
  {
    return executor_;
  }

  /// Get the capacity of the channel's buffer.
  std::size_t capacity() ASIO_NOEXCEPT
  {
    return service_->capacity(impl_);
  }

  /// Determine whether the channel is open.
  bool is_open() const ASIO_NOEXCEPT
  {
    return service_->is_open(impl_);
  }

  /// Reset the channel to its initial state.
  void reset()
  {
    service_->reset(impl_);
  }

  /// Close the channel.
  void close()
  {
    service_->close(impl_);
  }

  /// Cancel all asynchronous operations waiting on the channel.
  /**
   * All outstanding send operations will complete with the error
   * @c asio::experimental::error::channel_canceld. Outstanding receive
   * operations complete with the result as determined by the channel traits.
   */
  void cancel()
  {
    service_->cancel(impl_);
  }

  /// Determine whether a message can be received without blocking.
  bool ready() const ASIO_NOEXCEPT
  {
    return service_->ready(impl_);
  }

#if defined(GENERATING_DOCUMENTATION)

  /// Try to send a message without blocking.
  /**
   * Fails if the buffer is full and there are no waiting receive operations.
   *
   * @returns @c true on success, @c false on failure.
   */
  template <typename... Args>
  bool try_send(ASIO_MOVE_ARG(Args)... args);

  /// Try to send a number of messages without blocking.
  /**
   * @returns The number of messages that were sent.
   */
  template <typename... Args>
  std::size_t try_send_n(std::size_t count, ASIO_MOVE_ARG(Args)... args);

  /// Asynchronously send a message.
  template <typename... Args,
      ASIO_COMPLETION_TOKEN_FOR(void (asio::error_code))
        CompletionToken ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(executor_type)>
  auto async_send(ASIO_MOVE_ARG(Args)... args,
      ASIO_MOVE_ARG(CompletionToken) token);

#endif // defined(GENERATING_DOCUMENTATION)

  /// Try to receive a message without blocking.
  /**
   * Fails if the buffer is full and there are no waiting receive operations.
   *
   * @returns @c true on success, @c false on failure.
   */
  template <typename Handler>
  bool try_receive(ASIO_MOVE_ARG(Handler) handler)
  {
    return service_->try_receive(impl_, ASIO_MOVE_CAST(Handler)(handler));
  }

  /// Asynchronously receive a message.
  template <typename CompletionToken
      ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(executor_type)>
  auto async_receive(
      ASIO_MOVE_ARG(CompletionToken) token
        ASIO_DEFAULT_COMPLETION_TOKEN(Executor))
#if !defined(GENERATING_DOCUMENTATION)
    -> decltype(
        this->do_async_receive(static_cast<payload_type*>(0),
          ASIO_MOVE_CAST(CompletionToken)(token)))
#endif // !defined(GENERATING_DOCUMENTATION)
  {
    return this->do_async_receive(static_cast<payload_type*>(0),
        ASIO_MOVE_CAST(CompletionToken)(token));
  }

private:
  // Disallow copying and assignment.
  basic_concurrent_channel(
      const basic_concurrent_channel&) ASIO_DELETED;
  basic_concurrent_channel& operator=(
      const basic_concurrent_channel&) ASIO_DELETED;

  template <typename, typename, typename...>
  friend class detail::channel_send_functions;

  // Helper function to get an executor's context.
  template <typename T>
  static execution_context& get_context(const T& t,
      typename enable_if<execution::is_executor<T>::value>::type* = 0)
  {
    return asio::query(t, execution::context);
  }

  // Helper function to get an executor's context.
  template <typename T>
  static execution_context& get_context(const T& t,
      typename enable_if<!execution::is_executor<T>::value>::type* = 0)
  {
    return t.context();
  }

  class initiate_async_send
  {
  public:
    typedef Executor executor_type;

    explicit initiate_async_send(basic_concurrent_channel* self)
      : self_(self)
    {
    }

    executor_type get_executor() const ASIO_NOEXCEPT
    {
      return self_->get_executor();
    }

    template <typename SendHandler>
    void operator()(ASIO_MOVE_ARG(SendHandler) handler,
        ASIO_MOVE_ARG(payload_type) payload) const
    {
      asio::detail::non_const_lvalue<SendHandler> handler2(handler);
      self_->service_->async_send(self_->impl_,
          ASIO_MOVE_CAST(payload_type)(payload),
          handler2.value, self_->get_executor());
    }

  private:
    basic_concurrent_channel* self_;
  };

  class initiate_async_receive
  {
  public:
    typedef Executor executor_type;

    explicit initiate_async_receive(basic_concurrent_channel* self)
      : self_(self)
    {
    }

    executor_type get_executor() const ASIO_NOEXCEPT
    {
      return self_->get_executor();
    }

    template <typename ReceiveHandler>
    void operator()(ASIO_MOVE_ARG(ReceiveHandler) handler) const
    {
      asio::detail::non_const_lvalue<ReceiveHandler> handler2(handler);
      self_->service_->async_receive(self_->impl_,
          handler2.value, self_->get_executor());
    }

  private:
    basic_concurrent_channel* self_;
  };

  // The service associated with the I/O object.
  service_type* service_;

  // The underlying implementation of the I/O object.
  typename service_type::template implementation_type<
      Traits, Signatures...> impl_;

  // The associated executor.
  Executor executor_;
};

} // namespace experimental
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_EXPERIMENTAL_BASIC_CONCURRENT_CHANNEL_HPP
