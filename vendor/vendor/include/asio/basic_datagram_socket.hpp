//
// basic_datagram_socket.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_BASIC_DATAGRAM_SOCKET_HPP
#define ASIO_BASIC_DATAGRAM_SOCKET_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include <cstddef>
#include "asio/basic_socket.hpp"
#include "asio/detail/handler_type_requirements.hpp"
#include "asio/detail/non_const_lvalue.hpp"
#include "asio/detail/throw_error.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/error.hpp"
#include "asio/is_shallow_mutable_sequence.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN

#if !defined(ASIO_BASIC_DATAGRAM_SOCKET_FWD_DECL)
#define ASIO_BASIC_DATAGRAM_SOCKET_FWD_DECL

// Forward declaration with defaulted arguments.
template <typename Protocol, typename Executor = any_io_executor>
class basic_datagram_socket;

#endif // !defined(ASIO_BASIC_DATAGRAM_SOCKET_FWD_DECL)

/// Provides datagram-oriented socket functionality.
/**
 * The basic_datagram_socket class template provides asynchronous and blocking
 * datagram-oriented socket functionality.
 *
 * @par Thread Safety
 * @e Distinct @e objects: Safe.@n
 * @e Shared @e objects: Unsafe.
 *
 * Synchronous @c send, @c send_to, @c receive, @c receive_from, @c connect,
 * and @c shutdown operations are thread safe with respect to each other, if
 * the underlying operating system calls are also thread safe. This means that
 * it is permitted to perform concurrent calls to these synchronous operations
 * on a single socket object. Other synchronous operations, such as @c open or
 * @c close, are not thread safe.
 */
template <typename Protocol, typename Executor>
class basic_datagram_socket
  : public basic_socket<Protocol, Executor>
{
private:
  class initiate_async_send;
  class initiate_async_send_to;
  class initiate_async_receive;
  class initiate_async_receive_from;
  class initiate_async_send_multiple;
  class initiate_async_send_to_multiple;
  class initiate_async_receive_multiple;
  class initiate_async_receive_from_multiple;

public:
  /// The type of the executor associated with the object.
  typedef Executor executor_type;

  /// Rebinds the socket type to another executor.
  template <typename Executor1>
  struct rebind_executor
  {
    /// The socket type when rebound to the specified executor.
    typedef basic_datagram_socket<Protocol, Executor1> other;
  };

  /// The native representation of a socket.
#if defined(GENERATING_DOCUMENTATION)
  typedef implementation_defined native_handle_type;
#else
  typedef typename basic_socket<Protocol,
    Executor>::native_handle_type native_handle_type;
#endif

  /// The protocol type.
  typedef Protocol protocol_type;

  /// The endpoint type.
  typedef typename Protocol::endpoint endpoint_type;

  /// Construct a basic_datagram_socket without opening it.
  /**
   * This constructor creates a datagram socket without opening it. The open()
   * function must be called before data can be sent or received on the socket.
   *
   * @param ex The I/O executor that the socket will use, by default, to
   * dispatch handlers for any asynchronous operations performed on the socket.
   */
  explicit basic_datagram_socket(const executor_type& ex)
    : basic_socket<Protocol, Executor>(ex)
  {
  }

  /// Construct a basic_datagram_socket without opening it.
  /**
   * This constructor creates a datagram socket without opening it. The open()
   * function must be called before data can be sent or received on the socket.
   *
   * @param context An execution context which provides the I/O executor that
   * the socket will use, by default, to dispatch handlers for any asynchronous
   * operations performed on the socket.
   */
  template <typename ExecutionContext>
  explicit basic_datagram_socket(ExecutionContext& context,
      constraint_t<
        is_convertible<ExecutionContext&, execution_context&>::value
      > = 0)
    : basic_socket<Protocol, Executor>(context)
  {
  }

  /// Construct and open a basic_datagram_socket.
  /**
   * This constructor creates and opens a datagram socket.
   *
   * @param ex The I/O executor that the socket will use, by default, to
   * dispatch handlers for any asynchronous operations performed on the socket.
   *
   * @param protocol An object specifying protocol parameters to be used.
   *
   * @throws asio::system_error Thrown on failure.
   */
  basic_datagram_socket(const executor_type& ex, const protocol_type& protocol)
    : basic_socket<Protocol, Executor>(ex, protocol)
  {
  }

  /// Construct and open a basic_datagram_socket.
  /**
   * This constructor creates and opens a datagram socket.
   *
   * @param context An execution context which provides the I/O executor that
   * the socket will use, by default, to dispatch handlers for any asynchronous
   * operations performed on the socket.
   *
   * @param protocol An object specifying protocol parameters to be used.
   *
   * @throws asio::system_error Thrown on failure.
   */
  template <typename ExecutionContext>
  basic_datagram_socket(ExecutionContext& context,
      const protocol_type& protocol,
      constraint_t<
        is_convertible<ExecutionContext&, execution_context&>::value,
        defaulted_constraint
      > = defaulted_constraint())
    : basic_socket<Protocol, Executor>(context, protocol)
  {
  }

  /// Construct a basic_datagram_socket, opening it and binding it to the given
  /// local endpoint.
  /**
   * This constructor creates a datagram socket and automatically opens it bound
   * to the specified endpoint on the local machine. The protocol used is the
   * protocol associated with the given endpoint.
   *
   * @param ex The I/O executor that the socket will use, by default, to
   * dispatch handlers for any asynchronous operations performed on the socket.
   *
   * @param endpoint An endpoint on the local machine to which the datagram
   * socket will be bound.
   *
   * @throws asio::system_error Thrown on failure.
   */
  basic_datagram_socket(const executor_type& ex, const endpoint_type& endpoint)
    : basic_socket<Protocol, Executor>(ex, endpoint)
  {
  }

  /// Construct a basic_datagram_socket, opening it and binding it to the given
  /// local endpoint.
  /**
   * This constructor creates a datagram socket and automatically opens it bound
   * to the specified endpoint on the local machine. The protocol used is the
   * protocol associated with the given endpoint.
   *
   * @param context An execution context which provides the I/O executor that
   * the socket will use, by default, to dispatch handlers for any asynchronous
   * operations performed on the socket.
   *
   * @param endpoint An endpoint on the local machine to which the datagram
   * socket will be bound.
   *
   * @throws asio::system_error Thrown on failure.
   */
  template <typename ExecutionContext>
  basic_datagram_socket(ExecutionContext& context,
      const endpoint_type& endpoint,
      constraint_t<
        is_convertible<ExecutionContext&, execution_context&>::value
      > = 0)
    : basic_socket<Protocol, Executor>(context, endpoint)
  {
  }

  /// Construct a basic_datagram_socket on an existing native socket.
  /**
   * This constructor creates a datagram socket object to hold an existing
   * native socket.
   *
   * @param ex The I/O executor that the socket will use, by default, to
   * dispatch handlers for any asynchronous operations performed on the socket.
   *
   * @param protocol An object specifying protocol parameters to be used.
   *
   * @param native_socket The new underlying socket implementation.
   *
   * @throws asio::system_error Thrown on failure.
   */
  basic_datagram_socket(const executor_type& ex,
      const protocol_type& protocol, const native_handle_type& native_socket)
    : basic_socket<Protocol, Executor>(ex, protocol, native_socket)
  {
  }

  /// Construct a basic_datagram_socket on an existing native socket.
  /**
   * This constructor creates a datagram socket object to hold an existing
   * native socket.
   *
   * @param context An execution context which provides the I/O executor that
   * the socket will use, by default, to dispatch handlers for any asynchronous
   * operations performed on the socket.
   *
   * @param protocol An object specifying protocol parameters to be used.
   *
   * @param native_socket The new underlying socket implementation.
   *
   * @throws asio::system_error Thrown on failure.
   */
  template <typename ExecutionContext>
  basic_datagram_socket(ExecutionContext& context,
      const protocol_type& protocol, const native_handle_type& native_socket,
      constraint_t<
        is_convertible<ExecutionContext&, execution_context&>::value
      > = 0)
    : basic_socket<Protocol, Executor>(context, protocol, native_socket)
  {
  }

  /// Move-construct a basic_datagram_socket from another.
  /**
   * This constructor moves a datagram socket from one object to another.
   *
   * @param other The other basic_datagram_socket object from which the move
   * will occur.
   *
   * @note Following the move, the moved-from object is in the same state as if
   * constructed using the @c basic_datagram_socket(const executor_type&)
   * constructor.
   */
  basic_datagram_socket(basic_datagram_socket&& other) noexcept
    : basic_socket<Protocol, Executor>(std::move(other))
  {
  }

  /// Move-assign a basic_datagram_socket from another.
  /**
   * This assignment operator moves a datagram socket from one object to
   * another.
   *
   * @param other The other basic_datagram_socket object from which the move
   * will occur.
   *
   * @note Following the move, the moved-from object is in the same state as if
   * constructed using the @c basic_datagram_socket(const executor_type&)
   * constructor.
   */
  basic_datagram_socket& operator=(basic_datagram_socket&& other)
  {
    basic_socket<Protocol, Executor>::operator=(std::move(other));
    return *this;
  }

  /// Move-construct a basic_datagram_socket from a socket of another protocol
  /// type.
  /**
   * This constructor moves a datagram socket from one object to another.
   *
   * @param other The other basic_datagram_socket object from which the move
   * will occur.
   *
   * @note Following the move, the moved-from object is in the same state as if
   * constructed using the @c basic_datagram_socket(const executor_type&)
   * constructor.
   */
  template <typename Protocol1, typename Executor1>
  basic_datagram_socket(basic_datagram_socket<Protocol1, Executor1>&& other,
      constraint_t<
        is_convertible<Protocol1, Protocol>::value
          && is_convertible<Executor1, Executor>::value
      > = 0)
    : basic_socket<Protocol, Executor>(std::move(other))
  {
  }

  /// Move-assign a basic_datagram_socket from a socket of another protocol
  /// type.
  /**
   * This assignment operator moves a datagram socket from one object to
   * another.
   *
   * @param other The other basic_datagram_socket object from which the move
   * will occur.
   *
   * @note Following the move, the moved-from object is in the same state as if
   * constructed using the @c basic_datagram_socket(const executor_type&)
   * constructor.
   */
  template <typename Protocol1, typename Executor1>
  constraint_t<
    is_convertible<Protocol1, Protocol>::value
      && is_convertible<Executor1, Executor>::value,
    basic_datagram_socket&
  > operator=(basic_datagram_socket<Protocol1, Executor1>&& other)
  {
    basic_socket<Protocol, Executor>::operator=(std::move(other));
    return *this;
  }

  /// Destroys the socket.
  /**
   * This function destroys the socket, cancelling any outstanding asynchronous
   * operations associated with the socket as if by calling @c cancel.
   */
  ~basic_datagram_socket()
  {
  }

  /// Send some data on a connected socket.
  /**
   * This function is used to send data on the datagram socket. The function
   * call will block until the data has been sent successfully or an error
   * occurs.
   *
   * @param buffers One or more data buffers to be sent on the socket.
   *
   * @returns The number of bytes sent.
   *
   * @throws asio::system_error Thrown on failure.
   *
   * @note The send operation can only be used with a connected socket. Use
   * the send_to function to send data on an unconnected datagram socket.
   *
   * @par Example
   * To send a single data buffer use the @ref buffer function as follows:
   * @code socket.send(asio::buffer(data, size)); @endcode
   * See the @ref buffer documentation for information on sending multiple
   * buffers in one go, and how to use it with arrays, boost::array or
   * std::vector.
   */
  template <typename ConstBufferSequence>
  std::size_t send(const ConstBufferSequence& buffers)
  {
    asio::error_code ec;
    std::size_t s = this->impl_.get_service().send(
        this->impl_.get_implementation(), buffers, 0, ec);
    asio::detail::throw_error(ec, "send");
    return s;
  }

  /// Send some data on a connected socket.
  /**
   * This function is used to send data on the datagram socket. The function
   * call will block until the data has been sent successfully or an error
   * occurs.
   *
   * @param buffers One or more data buffers to be sent on the socket.
   *
   * @param flags Flags specifying how the send call is to be made.
   *
   * @returns The number of bytes sent.
   *
   * @throws asio::system_error Thrown on failure.
   *
   * @note The send operation can only be used with a connected socket. Use
   * the send_to function to send data on an unconnected datagram socket.
   */
  template <typename ConstBufferSequence>
  std::size_t send(const ConstBufferSequence& buffers,
      socket_base::message_flags flags)
  {
    asio::error_code ec;
    std::size_t s = this->impl_.get_service().send(
        this->impl_.get_implementation(), buffers, flags, ec);
    asio::detail::throw_error(ec, "send");
    return s;
  }

  /// Send some data on a connected socket.
  /**
   * This function is used to send data on the datagram socket. The function
   * call will block until the data has been sent successfully or an error
   * occurs.
   *
   * @param buffers One or more data buffers to be sent on the socket.
   *
   * @param flags Flags specifying how the send call is to be made.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @returns The number of bytes sent.
   *
   * @note The send operation can only be used with a connected socket. Use
   * the send_to function to send data on an unconnected datagram socket.
   */
  template <typename ConstBufferSequence>
  std::size_t send(const ConstBufferSequence& buffers,
      socket_base::message_flags flags, asio::error_code& ec)
  {
    return this->impl_.get_service().send(
        this->impl_.get_implementation(), buffers, flags, ec);
  }

  /// Start an asynchronous send on a connected socket.
  /**
   * This function is used to asynchronously send data on the datagram socket.
   * It is an initiating function for an @ref asynchronous_operation, and always
   * returns immediately.
   *
   * @param buffers One or more data buffers to be sent on the socket. Although
   * the buffers object may be copied as necessary, ownership of the underlying
   * memory blocks is retained by the caller, which must guarantee that they
   * remain valid until the completion handler is called.
   *
   * @param token The @ref completion_token that will be used to produce a
   * completion handler, which will be called when the send completes. Potential
   * completion tokens include @ref use_future, @ref use_awaitable, @ref
   * yield_context, or a function object with the correct completion signature.
   * The function signature of the completion handler must be:
   * @code void handler(
   *   const asio::error_code& error, // Result of operation.
   *   std::size_t bytes_transferred // Number of bytes sent.
   * ); @endcode
   * Regardless of whether the asynchronous operation completes immediately or
   * not, the completion handler will not be invoked from within this function.
   * On immediate completion, invocation of the handler will be performed in a
   * manner equivalent to using asio::async_immediate().
   *
   * @par Completion Signature
   * @code void(asio::error_code, std::size_t) @endcode
   *
   * @note The async_send operation can only be used with a connected socket.
   * Use the async_send_to function to send data on an unconnected datagram
   * socket.
   *
   * @par Example
   * To send a single data buffer use the @ref buffer function as follows:
   * @code
   * socket.async_send(asio::buffer(data, size), handler);
   * @endcode
   * See the @ref buffer documentation for information on sending multiple
   * buffers in one go, and how to use it with arrays, boost::array or
   * std::vector.
   *
   * @par Per-Operation Cancellation
   * On POSIX or Windows operating systems, this asynchronous operation supports
   * cancellation for the following asio::cancellation_type values:
   *
   * @li @c cancellation_type::terminal
   *
   * @li @c cancellation_type::partial
   *
   * @li @c cancellation_type::total
   */
  template <typename ConstBufferSequence,
      ASIO_COMPLETION_TOKEN_FOR(void (asio::error_code,
        std::size_t)) WriteToken = default_completion_token_t<executor_type>>
  auto async_send(const ConstBufferSequence& buffers,
      WriteToken&& token = default_completion_token_t<executor_type>())
    -> decltype(
      async_initiate<WriteToken,
        void (asio::error_code, std::size_t)>(
          declval<initiate_async_send>(), token,
          buffers, socket_base::message_flags(0)))
  {
    ASIO_INVOKE_INLINE return async_initiate<WriteToken,
      void (asio::error_code, std::size_t)>(
        initiate_async_send(this), token,
        buffers, socket_base::message_flags(0));
  }

  /// Start an asynchronous send on a connected socket.
  /**
   * This function is used to asynchronously send data on the datagram socket.
   * It is an initiating function for an @ref asynchronous_operation, and always
   * returns immediately.
   *
   * @param buffers One or more data buffers to be sent on the socket. Although
   * the buffers object may be copied as necessary, ownership of the underlying
   * memory blocks is retained by the caller, which must guarantee that they
   * remain valid until the completion handler is called.
   *
   * @param flags Flags specifying how the send call is to be made.
   *
   * @param token The @ref completion_token that will be used to produce a
   * completion handler, which will be called when the send completes. Potential
   * completion tokens include @ref use_future, @ref use_awaitable, @ref
   * yield_context, or a function object with the correct completion signature.
   * The function signature of the completion handler must be:
   * @code void handler(
   *   const asio::error_code& error, // Result of operation.
   *   std::size_t bytes_transferred // Number of bytes sent.
   * ); @endcode
   * Regardless of whether the asynchronous operation completes immediately or
   * not, the completion handler will not be invoked from within this function.
   * On immediate completion, invocation of the handler will be performed in a
   * manner equivalent to using asio::async_immediate().
   *
   * @par Completion Signature
   * @code void(asio::error_code, std::size_t) @endcode
   *
   * @note The async_send operation can only be used with a connected socket.
   * Use the async_send_to function to send data on an unconnected datagram
   * socket.
   *
   * @par Per-Operation Cancellation
   * On POSIX or Windows operating systems, this asynchronous operation supports
   * cancellation for the following asio::cancellation_type values:
   *
   * @li @c cancellation_type::terminal
   *
   * @li @c cancellation_type::partial
   *
   * @li @c cancellation_type::total
   */
  template <typename ConstBufferSequence,
      ASIO_COMPLETION_TOKEN_FOR(void (asio::error_code,
        std::size_t)) WriteToken = default_completion_token_t<executor_type>>
  auto async_send(const ConstBufferSequence& buffers,
      socket_base::message_flags flags,
      WriteToken&& token = default_completion_token_t<executor_type>())
    -> decltype(
      async_initiate<WriteToken,
        void (asio::error_code, std::size_t)>(
          declval<initiate_async_send>(), token, buffers, flags))
  {
    ASIO_INVOKE_INLINE return async_initiate<WriteToken,
      void (asio::error_code, std::size_t)>(
        initiate_async_send(this), token, buffers, flags);
  }

  /// Send a datagram to the specified endpoint.
  /**
   * This function is used to send a datagram to the specified remote endpoint.
   * The function call will block until the data has been sent successfully or
   * an error occurs.
   *
   * @param buffers One or more data buffers to be sent to the remote endpoint.
   *
   * @param destination The remote endpoint to which the data will be sent.
   *
   * @returns The number of bytes sent.
   *
   * @throws asio::system_error Thrown on failure.
   *
   * @par Example
   * To send a single data buffer use the @ref buffer function as follows:
   * @code
   * asio::ip::udp::endpoint destination(
   *     asio::ip::make_address("1.2.3.4"), 12345);
   * socket.send_to(asio::buffer(data, size), destination);
   * @endcode
   * See the @ref buffer documentation for information on sending multiple
   * buffers in one go, and how to use it with arrays, boost::array or
   * std::vector.
   */
  template <typename ConstBufferSequence>
  std::size_t send_to(const ConstBufferSequence& buffers,
      const endpoint_type& destination)
  {
    asio::error_code ec;
    std::size_t s = this->impl_.get_service().send_to(
        this->impl_.get_implementation(), buffers, destination, 0, ec);
    asio::detail::throw_error(ec, "send_to");
    return s;
  }

  /// Send a datagram to the specified endpoint.
  /**
   * This function is used to send a datagram to the specified remote endpoint.
   * The function call will block until the data has been sent successfully or
   * an error occurs.
   *
   * @param buffers One or more data buffers to be sent to the remote endpoint.
   *
   * @param destination The remote endpoint to which the data will be sent.
   *
   * @param flags Flags specifying how the send call is to be made.
   *
   * @returns The number of bytes sent.
   *
   * @throws asio::system_error Thrown on failure.
   */
  template <typename ConstBufferSequence>
  std::size_t send_to(const ConstBufferSequence& buffers,
      const endpoint_type& destination, socket_base::message_flags flags)
  {
    asio::error_code ec;
    std::size_t s = this->impl_.get_service().send_to(
        this->impl_.get_implementation(), buffers, destination, flags, ec);
    asio::detail::throw_error(ec, "send_to");
    return s;
  }

  /// Send a datagram to the specified endpoint.
  /**
   * This function is used to send a datagram to the specified remote endpoint.
   * The function call will block until the data has been sent successfully or
   * an error occurs.
   *
   * @param buffers One or more data buffers to be sent to the remote endpoint.
   *
   * @param destination The remote endpoint to which the data will be sent.
   *
   * @param flags Flags specifying how the send call is to be made.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @returns The number of bytes sent.
   */
  template <typename ConstBufferSequence>
  std::size_t send_to(const ConstBufferSequence& buffers,
      const endpoint_type& destination, socket_base::message_flags flags,
      asio::error_code& ec)
  {
    return this->impl_.get_service().send_to(this->impl_.get_implementation(),
        buffers, destination, flags, ec);
  }

  /// Start an asynchronous send.
  /**
   * This function is used to asynchronously send a datagram to the specified
   * remote endpoint. It is an initiating function for an @ref
   * asynchronous_operation, and always returns immediately.
   *
   * @param buffers One or more data buffers to be sent to the remote endpoint.
   * Although the buffers object may be copied as necessary, ownership of the
   * underlying memory blocks is retained by the caller, which must guarantee
   * that they remain valid until the completion handler is called.
   *
   * @param destination The remote endpoint to which the data will be sent.
   * Copies will be made of the endpoint as required.
   *
   * @param token The @ref completion_token that will be used to produce a
   * completion handler, which will be called when the send completes. Potential
   * completion tokens include @ref use_future, @ref use_awaitable, @ref
   * yield_context, or a function object with the correct completion signature.
   * The function signature of the completion handler must be:
   * @code void handler(
   *   const asio::error_code& error, // Result of operation.
   *   std::size_t bytes_transferred // Number of bytes sent.
   * ); @endcode
   * Regardless of whether the asynchronous operation completes immediately or
   * not, the completion handler will not be invoked from within this function.
   * On immediate completion, invocation of the handler will be performed in a
   * manner equivalent to using asio::async_immediate().
   *
   * @par Completion Signature
   * @code void(asio::error_code, std::size_t) @endcode
   *
   * @par Example
   * To send a single data buffer use the @ref buffer function as follows:
   * @code
   * asio::ip::udp::endpoint destination(
   *     asio::ip::make_address("1.2.3.4"), 12345);
   * socket.async_send_to(
   *     asio::buffer(data, size), destination, handler);
   * @endcode
   * See the @ref buffer documentation for information on sending multiple
   * buffers in one go, and how to use it with arrays, boost::array or
   * std::vector.
   *
   * @par Per-Operation Cancellation
   * On POSIX or Windows operating systems, this asynchronous operation supports
   * cancellation for the following asio::cancellation_type values:
   *
   * @li @c cancellation_type::terminal
   *
   * @li @c cancellation_type::partial
   *
   * @li @c cancellation_type::total
   */
  template <typename ConstBufferSequence,
      ASIO_COMPLETION_TOKEN_FOR(void (asio::error_code,
        std::size_t)) WriteToken = default_completion_token_t<executor_type>>
  auto async_send_to(const ConstBufferSequence& buffers,
      const endpoint_type& destination,
      WriteToken&& token = default_completion_token_t<executor_type>())
    -> decltype(
      async_initiate<WriteToken,
        void (asio::error_code, std::size_t)>(
          declval<initiate_async_send_to>(), token, buffers,
          destination, socket_base::message_flags(0)))
  {
    ASIO_INVOKE_INLINE return async_initiate<WriteToken,
      void (asio::error_code, std::size_t)>(
        initiate_async_send_to(this), token, buffers,
        destination, socket_base::message_flags(0));
  }

  /// Start an asynchronous send.
  /**
   * This function is used to asynchronously send a datagram to the specified
   * remote endpoint. It is an initiating function for an @ref
   * asynchronous_operation, and always returns immediately.
   *
   * @param buffers One or more data buffers to be sent to the remote endpoint.
   * Although the buffers object may be copied as necessary, ownership of the
   * underlying memory blocks is retained by the caller, which must guarantee
   * that they remain valid until the completion handler is called.
   *
   * @param flags Flags specifying how the send call is to be made.
   *
   * @param destination The remote endpoint to which the data will be sent.
   * Copies will be made of the endpoint as required.
   *
   * @param token The @ref completion_token that will be used to produce a
   * completion handler, which will be called when the send completes. Potential
   * completion tokens include @ref use_future, @ref use_awaitable, @ref
   * yield_context, or a function object with the correct completion signature.
   * The function signature of the completion handler must be:
   * @code void handler(
   *   const asio::error_code& error, // Result of operation.
   *   std::size_t bytes_transferred // Number of bytes sent.
   * ); @endcode
   * Regardless of whether the asynchronous operation completes immediately or
   * not, the completion handler will not be invoked from within this function.
   * On immediate completion, invocation of the handler will be performed in a
   * manner equivalent to using asio::async_immediate().
   *
   * @par Completion Signature
   * @code void(asio::error_code, std::size_t) @endcode
   *
   * @par Per-Operation Cancellation
   * On POSIX or Windows operating systems, this asynchronous operation supports
   * cancellation for the following asio::cancellation_type values:
   *
   * @li @c cancellation_type::terminal
   *
   * @li @c cancellation_type::partial
   *
   * @li @c cancellation_type::total
   */
  template <typename ConstBufferSequence,
      ASIO_COMPLETION_TOKEN_FOR(void (asio::error_code,
        std::size_t)) WriteToken = default_completion_token_t<executor_type>>
  auto async_send_to(const ConstBufferSequence& buffers,
      const endpoint_type& destination, socket_base::message_flags flags,
      WriteToken&& token = default_completion_token_t<executor_type>())
    -> decltype(
      async_initiate<WriteToken,
        void (asio::error_code, std::size_t)>(
          declval<initiate_async_send_to>(), token,
          buffers, destination, flags))
  {
    ASIO_INVOKE_INLINE return async_initiate<WriteToken,
      void (asio::error_code, std::size_t)>(
        initiate_async_send_to(this), token,
        buffers, destination, flags);
  }

  /// Receive some data on a connected socket.
  /**
   * This function is used to receive data on the datagram socket. The function
   * call will block until data has been received successfully or an error
   * occurs.
   *
   * @param buffers One or more buffers into which the data will be received.
   *
   * @returns The number of bytes received.
   *
   * @throws asio::system_error Thrown on failure.
   *
   * @note The receive operation can only be used with a connected socket. Use
   * the receive_from function to receive data on an unconnected datagram
   * socket.
   *
   * @par Example
   * To receive into a single data buffer use the @ref buffer function as
   * follows:
   * @code socket.receive(asio::buffer(data, size)); @endcode
   * See the @ref buffer documentation for information on receiving into
   * multiple buffers in one go, and how to use it with arrays, boost::array or
   * std::vector.
   */
  template <typename MutableBufferSequence>
  std::size_t receive(const MutableBufferSequence& buffers)
  {
    asio::error_code ec;
    std::size_t s = this->impl_.get_service().receive(
        this->impl_.get_implementation(), buffers, 0, ec);
    asio::detail::throw_error(ec, "receive");
    return s;
  }

  /// Receive some data on a connected socket.
  /**
   * This function is used to receive data on the datagram socket. The function
   * call will block until data has been received successfully or an error
   * occurs.
   *
   * @param buffers One or more buffers into which the data will be received.
   *
   * @param flags Flags specifying how the receive call is to be made.
   *
   * @returns The number of bytes received.
   *
   * @throws asio::system_error Thrown on failure.
   *
   * @note The receive operation can only be used with a connected socket. Use
   * the receive_from function to receive data on an unconnected datagram
   * socket.
   */
  template <typename MutableBufferSequence>
  std::size_t receive(const MutableBufferSequence& buffers,
      socket_base::message_flags flags)
  {
    asio::error_code ec;
    std::size_t s = this->impl_.get_service().receive(
        this->impl_.get_implementation(), buffers, flags, ec);
    asio::detail::throw_error(ec, "receive");
    return s;
  }

  /// Receive some data on a connected socket.
  /**
   * This function is used to receive data on the datagram socket. The function
   * call will block until data has been received successfully or an error
   * occurs.
   *
   * @param buffers One or more buffers into which the data will be received.
   *
   * @param flags Flags specifying how the receive call is to be made.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @returns The number of bytes received.
   *
   * @note The receive operation can only be used with a connected socket. Use
   * the receive_from function to receive data on an unconnected datagram
   * socket.
   */
  template <typename MutableBufferSequence>
  std::size_t receive(const MutableBufferSequence& buffers,
      socket_base::message_flags flags, asio::error_code& ec)
  {
    return this->impl_.get_service().receive(
        this->impl_.get_implementation(), buffers, flags, ec);
  }

  /// Start an asynchronous receive on a connected socket.
  /**
   * This function is used to asynchronously receive data from the datagram
   * socket. It is an initiating function for an @ref asynchronous_operation,
   * and always returns immediately.
   *
   * @param buffers One or more buffers into which the data will be received.
   * Although the buffers object may be copied as necessary, ownership of the
   * underlying memory blocks is retained by the caller, which must guarantee
   * that they remain valid until the completion handler is called.
   *
   * @param token The @ref completion_token that will be used to produce a
   * completion handler, which will be called when the receive completes.
   * Potential completion tokens include @ref use_future, @ref use_awaitable,
   * @ref yield_context, or a function object with the correct completion
   * signature. The function signature of the completion handler must be:
   * @code void handler(
   *   const asio::error_code& error, // Result of operation.
   *   std::size_t bytes_transferred // Number of bytes received.
   * ); @endcode
   * Regardless of whether the asynchronous operation completes immediately or
   * not, the completion handler will not be invoked from within this function.
   * On immediate completion, invocation of the handler will be performed in a
   * manner equivalent to using asio::async_immediate().
   *
   * @par Completion Signature
   * @code void(asio::error_code, std::size_t) @endcode
   *
   * @note The async_receive operation can only be used with a connected socket.
   * Use the async_receive_from function to receive data on an unconnected
   * datagram socket.
   *
   * @par Example
   * To receive into a single data buffer use the @ref buffer function as
   * follows:
   * @code
   * socket.async_receive(asio::buffer(data, size), handler);
   * @endcode
   * See the @ref buffer documentation for information on receiving into
   * multiple buffers in one go, and how to use it with arrays, boost::array or
   * std::vector.
   *
   * @par Per-Operation Cancellation
   * On POSIX or Windows operating systems, this asynchronous operation supports
   * cancellation for the following asio::cancellation_type values:
   *
   * @li @c cancellation_type::terminal
   *
   * @li @c cancellation_type::partial
   *
   * @li @c cancellation_type::total
   */
  template <typename MutableBufferSequence,
      ASIO_COMPLETION_TOKEN_FOR(void (asio::error_code,
        std::size_t)) ReadToken = default_completion_token_t<executor_type>>
  auto async_receive(const MutableBufferSequence& buffers,
      ReadToken&& token = default_completion_token_t<executor_type>())
    -> decltype(
      async_initiate<ReadToken,
        void (asio::error_code, std::size_t)>(
          declval<initiate_async_receive>(), token,
          buffers, socket_base::message_flags(0)))
  {
    ASIO_INVOKE_INLINE return async_initiate<ReadToken,
      void (asio::error_code, std::size_t)>(
        initiate_async_receive(this), token,
        buffers, socket_base::message_flags(0));
  }

  /// Start an asynchronous receive on a connected socket.
  /**
   * This function is used to asynchronously receive data from the datagram
   * socket. It is an initiating function for an @ref asynchronous_operation,
   * and always returns immediately.
   *
   * @param buffers One or more buffers into which the data will be received.
   * Although the buffers object may be copied as necessary, ownership of the
   * underlying memory blocks is retained by the caller, which must guarantee
   * that they remain valid until the completion handler is called.
   *
   * @param flags Flags specifying how the receive call is to be made.
   *
   * @param token The @ref completion_token that will be used to produce a
   * completion handler, which will be called when the receive completes.
   * Potential completion tokens include @ref use_future, @ref use_awaitable,
   * @ref yield_context, or a function object with the correct completion
   * signature. The function signature of the completion handler must be:
   * @code void handler(
   *   const asio::error_code& error, // Result of operation.
   *   std::size_t bytes_transferred // Number of bytes received.
   * ); @endcode
   * Regardless of whether the asynchronous operation completes immediately or
   * not, the completion handler will not be invoked from within this function.
   * On immediate completion, invocation of the handler will be performed in a
   * manner equivalent to using asio::async_immediate().
   *
   * @par Completion Signature
   * @code void(asio::error_code, std::size_t) @endcode
   *
   * @note The async_receive operation can only be used with a connected socket.
   * Use the async_receive_from function to receive data on an unconnected
   * datagram socket.
   *
   * @par Per-Operation Cancellation
   * On POSIX or Windows operating systems, this asynchronous operation supports
   * cancellation for the following asio::cancellation_type values:
   *
   * @li @c cancellation_type::terminal
   *
   * @li @c cancellation_type::partial
   *
   * @li @c cancellation_type::total
   */
  template <typename MutableBufferSequence,
      ASIO_COMPLETION_TOKEN_FOR(void (asio::error_code,
        std::size_t)) ReadToken = default_completion_token_t<executor_type>>
  auto async_receive(const MutableBufferSequence& buffers,
      socket_base::message_flags flags,
      ReadToken&& token = default_completion_token_t<executor_type>())
    -> decltype(
      async_initiate<ReadToken,
        void (asio::error_code, std::size_t)>(
          declval<initiate_async_receive>(), token, buffers, flags))
  {
    ASIO_INVOKE_INLINE return async_initiate<ReadToken,
      void (asio::error_code, std::size_t)>(
        initiate_async_receive(this), token, buffers, flags);
  }

  /// Receive a datagram with the endpoint of the sender.
  /**
   * This function is used to receive a datagram. The function call will block
   * until data has been received successfully or an error occurs.
   *
   * @param buffers One or more buffers into which the data will be received.
   *
   * @param sender_endpoint An endpoint object that receives the endpoint of
   * the remote sender of the datagram.
   *
   * @returns The number of bytes received.
   *
   * @throws asio::system_error Thrown on failure.
   *
   * @par Example
   * To receive into a single data buffer use the @ref buffer function as
   * follows:
   * @code
   * asio::ip::udp::endpoint sender_endpoint;
   * socket.receive_from(
   *     asio::buffer(data, size), sender_endpoint);
   * @endcode
   * See the @ref buffer documentation for information on receiving into
   * multiple buffers in one go, and how to use it with arrays, boost::array or
   * std::vector.
   */
  template <typename MutableBufferSequence>
  std::size_t receive_from(const MutableBufferSequence& buffers,
      endpoint_type& sender_endpoint)
  {
    asio::error_code ec;
    std::size_t s = this->impl_.get_service().receive_from(
        this->impl_.get_implementation(), buffers, sender_endpoint, 0, ec);
    asio::detail::throw_error(ec, "receive_from");
    return s;
  }

  /// Receive a datagram with the endpoint of the sender.
  /**
   * This function is used to receive a datagram. The function call will block
   * until data has been received successfully or an error occurs.
   *
   * @param buffers One or more buffers into which the data will be received.
   *
   * @param sender_endpoint An endpoint object that receives the endpoint of
   * the remote sender of the datagram.
   *
   * @param flags Flags specifying how the receive call is to be made.
   *
   * @returns The number of bytes received.
   *
   * @throws asio::system_error Thrown on failure.
   */
  template <typename MutableBufferSequence>
  std::size_t receive_from(const MutableBufferSequence& buffers,
      endpoint_type& sender_endpoint, socket_base::message_flags flags)
  {
    asio::error_code ec;
    std::size_t s = this->impl_.get_service().receive_from(
        this->impl_.get_implementation(), buffers, sender_endpoint, flags, ec);
    asio::detail::throw_error(ec, "receive_from");
    return s;
  }

  /// Receive a datagram with the endpoint of the sender.
  /**
   * This function is used to receive a datagram. The function call will block
   * until data has been received successfully or an error occurs.
   *
   * @param buffers One or more buffers into which the data will be received.
   *
   * @param sender_endpoint An endpoint object that receives the endpoint of
   * the remote sender of the datagram.
   *
   * @param flags Flags specifying how the receive call is to be made.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @returns The number of bytes received.
   */
  template <typename MutableBufferSequence>
  std::size_t receive_from(const MutableBufferSequence& buffers,
      endpoint_type& sender_endpoint, socket_base::message_flags flags,
      asio::error_code& ec)
  {
    return this->impl_.get_service().receive_from(
        this->impl_.get_implementation(), buffers, sender_endpoint, flags, ec);
  }

  /// Start an asynchronous receive.
  /**
   * This function is used to asynchronously receive a datagram. It is an
   * initiating function for an @ref asynchronous_operation, and always returns
   * immediately.
   *
   * @param buffers One or more buffers into which the data will be received.
   * Although the buffers object may be copied as necessary, ownership of the
   * underlying memory blocks is retained by the caller, which must guarantee
   * that they remain valid until the completion handler is called.
   *
   * @param sender_endpoint An endpoint object that receives the endpoint of
   * the remote sender of the datagram. Ownership of the sender_endpoint object
   * is retained by the caller, which must guarantee that it is valid until the
   * completion handler is called.
   *
   * @param token The @ref completion_token that will be used to produce a
   * completion handler, which will be called when the receive completes.
   * Potential completion tokens include @ref use_future, @ref use_awaitable,
   * @ref yield_context, or a function object with the correct completion
   * signature. The function signature of the completion handler must be:
   * @code void handler(
   *   const asio::error_code& error, // Result of operation.
   *   std::size_t bytes_transferred // Number of bytes received.
   * ); @endcode
   * Regardless of whether the asynchronous operation completes immediately or
   * not, the completion handler will not be invoked from within this function.
   * On immediate completion, invocation of the handler will be performed in a
   * manner equivalent to using asio::async_immediate().
   *
   * @par Completion Signature
   * @code void(asio::error_code, std::size_t) @endcode
   *
   * @par Example
   * To receive into a single data buffer use the @ref buffer function as
   * follows:
   * @code socket.async_receive_from(
   *     asio::buffer(data, size), sender_endpoint, handler); @endcode
   * See the @ref buffer documentation for information on receiving into
   * multiple buffers in one go, and how to use it with arrays, boost::array or
   * std::vector.
   *
   * @par Per-Operation Cancellation
   * On POSIX or Windows operating systems, this asynchronous operation supports
   * cancellation for the following asio::cancellation_type values:
   *
   * @li @c cancellation_type::terminal
   *
   * @li @c cancellation_type::partial
   *
   * @li @c cancellation_type::total
   */
  template <typename MutableBufferSequence,
      ASIO_COMPLETION_TOKEN_FOR(void (asio::error_code,
        std::size_t)) ReadToken = default_completion_token_t<executor_type>>
  auto async_receive_from(const MutableBufferSequence& buffers,
      endpoint_type& sender_endpoint,
      ReadToken&& token = default_completion_token_t<executor_type>())
    -> decltype(
      async_initiate<ReadToken,
        void (asio::error_code, std::size_t)>(
          declval<initiate_async_receive_from>(), token, buffers,
          &sender_endpoint, socket_base::message_flags(0)))
  {
    ASIO_INVOKE_INLINE return async_initiate<ReadToken,
      void (asio::error_code, std::size_t)>(
        initiate_async_receive_from(this), token, buffers,
        &sender_endpoint, socket_base::message_flags(0));
  }

  /// Start an asynchronous receive.
  /**
   * This function is used to asynchronously receive a datagram. It is an
   * initiating function for an @ref asynchronous_operation, and always returns
   * immediately.
   *
   * @param buffers One or more buffers into which the data will be received.
   * Although the buffers object may be copied as necessary, ownership of the
   * underlying memory blocks is retained by the caller, which must guarantee
   * that they remain valid until the completion handler is called.
   *
   * @param sender_endpoint An endpoint object that receives the endpoint of
   * the remote sender of the datagram. Ownership of the sender_endpoint object
   * is retained by the caller, which must guarantee that it is valid until the
   * completion handler is called.
   *
   * @param flags Flags specifying how the receive call is to be made.
   *
   * @param token The @ref completion_token that will be used to produce a
   * completion handler, which will be called when the receive completes.
   * Potential completion tokens include @ref use_future, @ref use_awaitable,
   * @ref yield_context, or a function object with the correct completion
   * signature. The function signature of the completion handler must be:
   * @code void handler(
   *   const asio::error_code& error, // Result of operation.
   *   std::size_t bytes_transferred // Number of bytes received.
   * ); @endcode
   * Regardless of whether the asynchronous operation completes immediately or
   * not, the completion handler will not be invoked from within this function.
   * On immediate completion, invocation of the handler will be performed in a
   * manner equivalent to using asio::async_immediate().
   *
   * @par Completion Signature
   * @code void(asio::error_code, std::size_t) @endcode
   *
   * @par Per-Operation Cancellation
   * On POSIX or Windows operating systems, this asynchronous operation supports
   * cancellation for the following asio::cancellation_type values:
   *
   * @li @c cancellation_type::terminal
   *
   * @li @c cancellation_type::partial
   *
   * @li @c cancellation_type::total
   */
  template <typename MutableBufferSequence,
      ASIO_COMPLETION_TOKEN_FOR(void (asio::error_code,
        std::size_t)) ReadToken = default_completion_token_t<executor_type>>
  auto async_receive_from(const MutableBufferSequence& buffers,
      endpoint_type& sender_endpoint, socket_base::message_flags flags,
      ReadToken&& token = default_completion_token_t<executor_type>())
    -> decltype(
      async_initiate<ReadToken,
        void (asio::error_code, std::size_t)>(
          declval<initiate_async_receive_from>(), token,
          buffers, &sender_endpoint, flags))
  {
    ASIO_INVOKE_INLINE return async_initiate<ReadToken,
      void (asio::error_code, std::size_t)>(
        initiate_async_receive_from(this), token,
        buffers, &sender_endpoint, flags);
  }

  /// Send multiple datagrams on a connected socket.
  /**
   * This function is used to send multiple datagrams on the datagram socket.
   * The function call will block until one or more datagrams have been sent
   * successfully or an error occurs. The number of datagrams sent may be less
   * than the length of the range.
   *
   * @param buffer_sequences A forward range of buffer sequences, one per
   * datagram to send.
   *
   * @returns The number of datagrams sent.
   *
   * @throws asio::system_error Thrown on failure.
   *
   * @note The send_multiple operation can only be used with a connected
   * socket. Use the send_to_multiple function to send data on an unconnected
   * datagram socket.
   */
  template <typename ConstBufferSequences>
  std::size_t send_multiple(ConstBufferSequences&& buffer_sequences)
  {
    asio::error_code ec;
    std::size_t s = this->impl_.get_service().send_multiple(
        this->impl_.get_implementation(),
        static_cast<ConstBufferSequences&&>(buffer_sequences), 0, ec);
    asio::detail::throw_error(ec, "send_multiple");
    return s;
  }

  /// Send multiple datagrams on a connected socket.
  /**
   * This function is used to send multiple datagrams on the datagram socket.
   * The function call will block until one or more datagrams have been sent
   * successfully or an error occurs. The number of datagrams sent may be less
   * than the length of the range.
   *
   * @param buffer_sequences A forward range of buffer sequences, one per
   * datagram to send.
   *
   * @param flags Flags specifying how the send call is to be made.
   *
   * @returns The number of datagrams sent.
   *
   * @throws asio::system_error Thrown on failure.
   *
   * @note The send_multiple operation can only be used with a connected
   * socket. Use the send_to_multiple function to send data on an unconnected
   * datagram socket.
   */
  template <typename ConstBufferSequences>
  std::size_t send_multiple(ConstBufferSequences&& buffer_sequences,
      socket_base::message_flags flags)
  {
    asio::error_code ec;
    std::size_t s = this->impl_.get_service().send_multiple(
        this->impl_.get_implementation(),
        static_cast<ConstBufferSequences&&>(buffer_sequences), flags, ec);
    asio::detail::throw_error(ec, "send_multiple");
    return s;
  }

  /// Send multiple datagrams on a connected socket.
  /**
   * This function is used to send multiple datagrams on the datagram socket.
   * The function call will block until one or more datagrams have been sent
   * successfully or an error occurs. The number of datagrams sent may be less
   * than the length of the range.
   *
   * @param buffer_sequences A forward range of buffer sequences, one per
   * datagram to send.
   *
   * @param flags Flags specifying how the send call is to be made.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @returns The number of datagrams sent.
   *
   * @note The send_multiple operation can only be used with a connected
   * socket. Use the send_to_multiple function to send data on an unconnected
   * datagram socket.
   */
  template <typename ConstBufferSequences>
  std::size_t send_multiple(ConstBufferSequences&& buffer_sequences,
      socket_base::message_flags flags, asio::error_code& ec)
  {
    return this->impl_.get_service().send_multiple(
        this->impl_.get_implementation(),
        static_cast<ConstBufferSequences&&>(buffer_sequences), flags, ec);
  }

  /// Start an asynchronous send of multiple datagrams on a connected socket.
  /**
   * This function is used to asynchronously send multiple datagrams on a
   * connected datagram socket. It is an initiating function for an
   * @ref asynchronous_operation, and always returns immediately. The number of
   * datagrams sent may be less than the length of the range.
   *
   * @param buffer_sequences A forward range of buffer sequences, one per
   * datagram to send. Although the range object may be copied as necessary,
   * ownership of the underlying memory blocks is retained by the caller, which
   * must guarantee that they remain valid until the completion handler is
   * called.
   *
   * @param token The @ref completion_token that will be used to produce a
   * completion handler, which will be called when the send completes. Potential
   * completion tokens include @ref use_future, @ref use_awaitable, @ref
   * yield_context, or a function object with the correct completion signature.
   * The function signature of the completion handler must be:
   * @code void handler(
   *   const asio::error_code& error, // Result of operation.
   *   std::size_t messages_transferred // Number of datagrams sent.
   * ); @endcode
   * Regardless of whether the asynchronous operation completes immediately or
   * not, the completion handler will not be invoked from within this function.
   * On immediate completion, invocation of the handler will be performed in a
   * manner equivalent to using asio::async_immediate().
   *
   * @par Completion Signature
   * @code void(asio::error_code, std::size_t) @endcode
   *
   * @note The async_send_multiple operation can only be used with a connected
   * socket. Use the async_send_to_multiple function to send data on an
   * unconnected datagram socket.
   *
   * @par Per-Operation Cancellation
   * On POSIX or Windows operating systems, this asynchronous operation supports
   * cancellation for the following asio::cancellation_type values:
   *
   * @li @c cancellation_type::terminal
   *
   * @li @c cancellation_type::partial
   *
   * @li @c cancellation_type::total
   */
  template <typename ConstBufferSequences,
      ASIO_COMPLETION_TOKEN_FOR(void (asio::error_code,
        std::size_t)) WriteMultiToken =
          default_completion_token_t<executor_type>>
  auto async_send_multiple(ConstBufferSequences&& buffer_sequences,
      WriteMultiToken&& token = default_completion_token_t<executor_type>())
    -> decltype(
      async_initiate<WriteMultiToken,
        void (asio::error_code, std::size_t)>(
          declval<initiate_async_send_multiple>(), token,
          static_cast<ConstBufferSequences&&>(buffer_sequences),
          socket_base::message_flags(0)))
  {
    ASIO_INVOKE_INLINE return async_initiate<WriteMultiToken,
      void (asio::error_code, std::size_t)>(
        initiate_async_send_multiple(this), token,
        static_cast<ConstBufferSequences&&>(buffer_sequences),
        socket_base::message_flags(0));
  }

  /// Start an asynchronous send of multiple datagrams on a connected socket.
  /**
   * This function is used to asynchronously send multiple datagrams on a
   * connected datagram socket. It is an initiating function for an
   * @ref asynchronous_operation, and always returns immediately. The number of
   * datagrams sent may be less than the length of the range.
   *
   * @param buffer_sequences A forward range of buffer sequences, one per
   * datagram to send. Although the range object may be copied as necessary,
   * ownership of the underlying memory blocks is retained by the caller, which
   * must guarantee that they remain valid until the completion handler is
   * called.
   *
   * @param flags Flags specifying how the send call is to be made.
   *
   * @param token The @ref completion_token that will be used to produce a
   * completion handler, which will be called when the send completes. Potential
   * completion tokens include @ref use_future, @ref use_awaitable, @ref
   * yield_context, or a function object with the correct completion signature.
   * The function signature of the completion handler must be:
   * @code void handler(
   *   const asio::error_code& error, // Result of operation.
   *   std::size_t messages_transferred // Number of datagrams sent.
   * ); @endcode
   * Regardless of whether the asynchronous operation completes immediately or
   * not, the completion handler will not be invoked from within this function.
   * On immediate completion, invocation of the handler will be performed in a
   * manner equivalent to using asio::async_immediate().
   *
   * @par Completion Signature
   * @code void(asio::error_code, std::size_t) @endcode
   *
   * @note The async_send_multiple operation can only be used with a connected
   * socket. Use the async_send_to_multiple function to send data on an
   * unconnected datagram socket.
   *
   * @par Per-Operation Cancellation
   * On POSIX or Windows operating systems, this asynchronous operation supports
   * cancellation for the following asio::cancellation_type values:
   *
   * @li @c cancellation_type::terminal
   *
   * @li @c cancellation_type::partial
   *
   * @li @c cancellation_type::total
   */
  template <typename ConstBufferSequences,
      ASIO_COMPLETION_TOKEN_FOR(void (asio::error_code,
        std::size_t)) WriteMultiToken =
          default_completion_token_t<executor_type>>
  auto async_send_multiple(ConstBufferSequences&& buffer_sequences,
      socket_base::message_flags flags,
      WriteMultiToken&& token = default_completion_token_t<executor_type>())
    -> decltype(
      async_initiate<WriteMultiToken,
        void (asio::error_code, std::size_t)>(
          declval<initiate_async_send_multiple>(), token,
          static_cast<ConstBufferSequences&&>(buffer_sequences), flags))
  {
    ASIO_INVOKE_INLINE return async_initiate<WriteMultiToken,
      void (asio::error_code, std::size_t)>(
        initiate_async_send_multiple(this), token,
        static_cast<ConstBufferSequences&&>(buffer_sequences), flags);
  }

  /// Send multiple datagrams to the specified endpoints.
  /**
   * This function is used to send multiple datagrams to the specified remote
   * endpoints. The function call will block until one or more datagrams have
   * been sent successfully or an error occurs. The number of datagrams
   * transferred may be less than the length of the shortest range.
   *
   * @param buffer_sequences A forward range of buffer sequences, one per
   * datagram to send.
   *
   * @param endpoints A forward range of destination endpoints, one per
   * datagram.
   *
   * @returns The number of datagrams sent.
   *
   * @throws asio::system_error Thrown on failure.
   */
  template <typename ConstBufferSequences, typename EndpointSequence>
  std::size_t send_to_multiple(ConstBufferSequences&& buffer_sequences,
      EndpointSequence&& endpoints)
  {
    asio::error_code ec;
    std::size_t s = this->impl_.get_service().send_to_multiple(
        this->impl_.get_implementation(),
        static_cast<ConstBufferSequences&&>(buffer_sequences),
        static_cast<EndpointSequence&&>(endpoints), 0, ec);
    asio::detail::throw_error(ec, "send_to_multiple");
    return s;
  }

  /// Send multiple datagrams to the specified endpoints.
  /**
   * This function is used to send multiple datagrams to the specified remote
   * endpoints. The function call will block until one or more datagrams have
   * been sent successfully or an error occurs. The number of datagrams
   * transferred may be less than the length of the shortest range.
   *
   * @param buffer_sequences A forward range of buffer sequences, one per
   * datagram to send.
   *
   * @param endpoints A forward range of destination endpoints, one per
   * datagram.
   *
   * @param flags Flags specifying how the send call is to be made.
   *
   * @returns The number of datagrams sent.
   *
   * @throws asio::system_error Thrown on failure.
   */
  template <typename ConstBufferSequences, typename EndpointSequence>
  std::size_t send_to_multiple(ConstBufferSequences&& buffer_sequences,
      EndpointSequence&& endpoints, socket_base::message_flags flags)
  {
    asio::error_code ec;
    std::size_t s = this->impl_.get_service().send_to_multiple(
        this->impl_.get_implementation(),
        static_cast<ConstBufferSequences&&>(buffer_sequences),
        static_cast<EndpointSequence&&>(endpoints), flags, ec);
    asio::detail::throw_error(ec, "send_to_multiple");
    return s;
  }

  /// Send multiple datagrams to the specified endpoints.
  /**
   * This function is used to send multiple datagrams to the specified remote
   * endpoints. The function call will block until one or more datagrams have
   * been sent successfully or an error occurs. The number of datagrams
   * transferred may be less than the length of the shortest range.
   *
   * @param buffer_sequences A forward range of buffer sequences, one per
   * datagram to send.
   *
   * @param endpoints A forward range of destination endpoints, one per
   * datagram.
   *
   * @param flags Flags specifying how the send call is to be made.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @returns The number of datagrams sent.
   */
  template <typename ConstBufferSequences, typename EndpointSequence>
  std::size_t send_to_multiple(ConstBufferSequences&& buffer_sequences,
      EndpointSequence&& endpoints, socket_base::message_flags flags,
      asio::error_code& ec)
  {
    return this->impl_.get_service().send_to_multiple(
        this->impl_.get_implementation(),
        static_cast<ConstBufferSequences&&>(buffer_sequences),
        static_cast<EndpointSequence&&>(endpoints), flags, ec);
  }

  /// Start an asynchronous send of multiple datagrams to the endpoints.
  /**
   * This function is used to asynchronously send multiple datagrams to the
   * specified remote endpoints. It is an initiating function for an
   * @ref asynchronous_operation, and always returns immediately. The number of
   * datagrams transferred may be less than the length of the shortest range.
   *
   * @param buffer_sequences A forward range of buffer sequences, one per
   * datagram to send. Although the range object may be copied as necessary,
   * ownership of the underlying memory blocks is retained by the caller, which
   * must guarantee that they remain valid until the completion handler is
   * called.
   *
   * @param endpoints A forward range of destination endpoints, one per
   * datagram. Although the range object may be copied as necessary, ownership
   * of any endpoints to which the range yields references is retained by the
   * caller, which must guarantee that they remain valid until the completion
   * handler is called. Copies will be made of endpoints yielded by value as
   * required.
   *
   * @param token The @ref completion_token that will be used to produce a
   * completion handler, which will be called when the send completes. Potential
   * completion tokens include @ref use_future, @ref use_awaitable, @ref
   * yield_context, or a function object with the correct completion signature.
   * The function signature of the completion handler must be:
   * @code void handler(
   *   const asio::error_code& error, // Result of operation.
   *   std::size_t messages_transferred // Number of datagrams sent.
   * ); @endcode
   * Regardless of whether the asynchronous operation completes immediately or
   * not, the completion handler will not be invoked from within this function.
   * On immediate completion, invocation of the handler will be performed in a
   * manner equivalent to using asio::async_immediate().
   *
   * @par Completion Signature
   * @code void(asio::error_code, std::size_t) @endcode
   *
   * @par Per-Operation Cancellation
   * On POSIX or Windows operating systems, this asynchronous operation supports
   * cancellation for the following asio::cancellation_type values:
   *
   * @li @c cancellation_type::terminal
   *
   * @li @c cancellation_type::partial
   *
   * @li @c cancellation_type::total
   */
  template <typename ConstBufferSequences, typename EndpointSequence,
      ASIO_COMPLETION_TOKEN_FOR(void (asio::error_code,
        std::size_t)) WriteMultiToken =
          default_completion_token_t<executor_type>>
  auto async_send_to_multiple(ConstBufferSequences&& buffer_sequences,
      EndpointSequence&& endpoints,
      WriteMultiToken&& token = default_completion_token_t<executor_type>())
    -> decltype(
      async_initiate<WriteMultiToken,
        void (asio::error_code, std::size_t)>(
          declval<initiate_async_send_to_multiple>(), token,
          static_cast<ConstBufferSequences&&>(buffer_sequences),
          static_cast<EndpointSequence&&>(endpoints),
          socket_base::message_flags(0)))
  {
    ASIO_INVOKE_INLINE return async_initiate<WriteMultiToken,
      void (asio::error_code, std::size_t)>(
        initiate_async_send_to_multiple(this), token,
        static_cast<ConstBufferSequences&&>(buffer_sequences),
        static_cast<EndpointSequence&&>(endpoints),
        socket_base::message_flags(0));
  }

  /// Start an asynchronous send of multiple datagrams to the endpoints.
  /**
   * This function is used to asynchronously send multiple datagrams to the
   * specified remote endpoints. It is an initiating function for an
   * @ref asynchronous_operation, and always returns immediately. The number of
   * datagrams transferred may be less than the length of the shortest range.
   *
   * @param buffer_sequences A forward range of buffer sequences, one per
   * datagram to send. Although the range object may be copied as necessary,
   * ownership of the underlying memory blocks is retained by the caller, which
   * must guarantee that they remain valid until the completion handler is
   * called.
   *
   * @param endpoints A forward range of destination endpoints, one per
   * datagram. Although the range object may be copied as necessary, ownership
   * of any endpoints to which the range yields references is retained by the
   * caller, which must guarantee that they remain valid until the completion
   * handler is called. Copies will be made of endpoints yielded by value as
   * required.
   *
   * @param flags Flags specifying how the send call is to be made.
   *
   * @param token The @ref completion_token that will be used to produce a
   * completion handler, which will be called when the send completes. Potential
   * completion tokens include @ref use_future, @ref use_awaitable, @ref
   * yield_context, or a function object with the correct completion signature.
   * The function signature of the completion handler must be:
   * @code void handler(
   *   const asio::error_code& error, // Result of operation.
   *   std::size_t messages_transferred // Number of datagrams sent.
   * ); @endcode
   * Regardless of whether the asynchronous operation completes immediately or
   * not, the completion handler will not be invoked from within this function.
   * On immediate completion, invocation of the handler will be performed in a
   * manner equivalent to using asio::async_immediate().
   *
   * @par Completion Signature
   * @code void(asio::error_code, std::size_t) @endcode
   *
   * @par Per-Operation Cancellation
   * On POSIX or Windows operating systems, this asynchronous operation supports
   * cancellation for the following asio::cancellation_type values:
   *
   * @li @c cancellation_type::terminal
   *
   * @li @c cancellation_type::partial
   *
   * @li @c cancellation_type::total
   */
  template <typename ConstBufferSequences, typename EndpointSequence,
      ASIO_COMPLETION_TOKEN_FOR(void (asio::error_code,
        std::size_t)) WriteMultiToken =
          default_completion_token_t<executor_type>>
  auto async_send_to_multiple(ConstBufferSequences&& buffer_sequences,
      EndpointSequence&& endpoints, socket_base::message_flags flags,
      WriteMultiToken&& token = default_completion_token_t<executor_type>())
    -> decltype(
      async_initiate<WriteMultiToken,
        void (asio::error_code, std::size_t)>(
          declval<initiate_async_send_to_multiple>(), token,
          static_cast<ConstBufferSequences&&>(buffer_sequences),
          static_cast<EndpointSequence&&>(endpoints), flags))
  {
    ASIO_INVOKE_INLINE return async_initiate<WriteMultiToken,
      void (asio::error_code, std::size_t)>(
        initiate_async_send_to_multiple(this), token,
        static_cast<ConstBufferSequences&&>(buffer_sequences),
        static_cast<EndpointSequence&&>(endpoints), flags);
  }

  /// Receive multiple datagrams on a connected socket.
  /**
   * This function is used to receive multiple datagrams on the datagram socket.
   * The function call will block until one or more datagrams have been received
   * successfully or an error occurs. The number of datagrams transferred may be
   * less than the length of the shortest range.
   *
   * @param buffer_sequences A forward range of mutable buffer sequences, one
   * per datagram, into which the data will be received.
   *
   * @param sizes A forward range of @c std::size_t, one per datagram, into
   * which the number of bytes received in each datagram is written.
   *
   * @returns The number of datagrams received.
   *
   * @throws asio::system_error Thrown on failure.
   *
   * @note The receive_multiple operation can only be used with a connected
   * socket. Use the receive_from_multiple function to receive data on an
   * unconnected datagram socket.
   */
  template <typename MutableBufferSequences, typename MutableSizeSequence>
  std::size_t receive_multiple(MutableBufferSequences&& buffer_sequences,
      MutableSizeSequence&& sizes)
  {
    static_assert(
        is_shallow_mutable_sequence<decay_t<MutableSizeSequence>>::value,
        "sizes must be a shallow mutable sequence");

    asio::error_code ec;
    std::size_t s = this->impl_.get_service().receive_multiple(
        this->impl_.get_implementation(),
        static_cast<MutableBufferSequences&&>(buffer_sequences),
        static_cast<MutableSizeSequence&&>(sizes), detail::empty_range(),
        0, ec);
    asio::detail::throw_error(ec, "receive_multiple");
    return s;
  }

  /// Receive multiple datagrams on a connected socket.
  /**
   * This function is used to receive multiple datagrams on the datagram socket.
   * The function call will block until one or more datagrams have been received
   * successfully or an error occurs. The number of datagrams transferred may be
   * less than the length of the shortest range.
   *
   * @param buffer_sequences A forward range of mutable buffer sequences, one
   * per datagram, into which the data will be received.
   *
   * @param sizes A forward range of @c std::size_t, one per datagram, into
   * which the number of bytes received in each datagram is written.
   *
   * @param flags Flags specifying how the receive call is to be made.
   *
   * @returns The number of datagrams received.
   *
   * @throws asio::system_error Thrown on failure.
   *
   * @note The receive_multiple operation can only be used with a connected
   * socket. Use the receive_from_multiple function to receive data on an
   * unconnected datagram socket.
   */
  template <typename MutableBufferSequences, typename MutableSizeSequence>
  std::size_t receive_multiple(MutableBufferSequences&& buffer_sequences,
      MutableSizeSequence&& sizes, socket_base::message_flags flags)
  {
    static_assert(
        is_shallow_mutable_sequence<decay_t<MutableSizeSequence>>::value,
        "sizes must be a shallow mutable sequence");

    asio::error_code ec;
    std::size_t s = this->impl_.get_service().receive_multiple(
        this->impl_.get_implementation(),
        static_cast<MutableBufferSequences&&>(buffer_sequences),
        static_cast<MutableSizeSequence&&>(sizes), detail::empty_range(),
        flags, ec);
    asio::detail::throw_error(ec, "receive_multiple");
    return s;
  }

  /// Receive multiple datagrams on a connected socket.
  /**
   * This function is used to receive multiple datagrams on the datagram socket.
   * The function call will block until one or more datagrams have been received
   * successfully or an error occurs. The number of datagrams transferred may be
   * less than the length of the shortest range.
   *
   * @param buffer_sequences A forward range of mutable buffer sequences, one
   * per datagram, into which the data will be received.
   *
   * @param sizes A forward range of @c std::size_t, one per datagram, into
   * which the number of bytes received in each datagram is written.
   *
   * @param flags Flags specifying how the receive call is to be made.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @returns The number of datagrams received.
   *
   * @note The receive_multiple operation can only be used with a connected
   * socket. Use the receive_from_multiple function to receive data on an
   * unconnected datagram socket.
   */
  template <typename MutableBufferSequences, typename MutableSizeSequence>
  std::size_t receive_multiple(MutableBufferSequences&& buffer_sequences,
      MutableSizeSequence&& sizes, socket_base::message_flags flags,
      asio::error_code& ec)
  {
    static_assert(
        is_shallow_mutable_sequence<decay_t<MutableSizeSequence>>::value,
        "sizes must be a shallow mutable sequence");

    return this->impl_.get_service().receive_multiple(
        this->impl_.get_implementation(),
        static_cast<MutableBufferSequences&&>(buffer_sequences),
        static_cast<MutableSizeSequence&&>(sizes), detail::empty_range(),
        flags, ec);
  }

  /// Receive multiple datagrams and their flags on a connected socket.
  /**
   * This function is used to receive multiple datagrams on the datagram socket.
   * The function call will block until one or more datagrams have been received
   * successfully or an error occurs. The number of datagrams transferred may be
   * less than the length of the shortest range.
   *
   * @param buffer_sequences A forward range of mutable buffer sequences, one
   * per datagram, into which the data will be received.
   *
   * @param sizes A forward range of @c std::size_t, one per datagram, into
   * which the number of bytes received in each datagram is written.
   *
   * @param out_flags A forward range of @c socket_base::message_flags, one per
   * datagram, into which the flags associated with each received datagram are
   * written.
   *
   * @returns The number of datagrams received.
   *
   * @throws asio::system_error Thrown on failure.
   *
   * @note The receive_multiple operation can only be used with a connected
   * socket. Use the receive_from_multiple function to receive data on an
   * unconnected datagram socket.
   */
  template <typename MutableBufferSequences, typename MutableSizeSequence,
      typename MutableFlagsSequence>
  std::size_t receive_multiple(MutableBufferSequences&& buffer_sequences,
      MutableSizeSequence&& sizes, MutableFlagsSequence&& out_flags,
      constraint_t<
        is_shallow_mutable_sequence<decay_t<MutableFlagsSequence>>::value
      > = 0)
  {
    static_assert(
        is_shallow_mutable_sequence<decay_t<MutableSizeSequence>>::value,
        "sizes must be a shallow mutable sequence");

    asio::error_code ec;
    std::size_t s = this->impl_.get_service().receive_multiple(
        this->impl_.get_implementation(),
        static_cast<MutableBufferSequences&&>(buffer_sequences),
        static_cast<MutableSizeSequence&&>(sizes),
        static_cast<MutableFlagsSequence&&>(out_flags), 0, ec);
    asio::detail::throw_error(ec, "receive_multiple");
    return s;
  }

  /// Receive multiple datagrams and their flags on a connected socket.
  /**
   * This function is used to receive multiple datagrams on the datagram socket.
   * The function call will block until one or more datagrams have been received
   * successfully or an error occurs. The number of datagrams transferred may be
   * less than the length of the shortest range.
   *
   * @param buffer_sequences A forward range of mutable buffer sequences, one
   * per datagram, into which the data will be received.
   *
   * @param sizes A forward range of @c std::size_t, one per datagram, into
   * which the number of bytes received in each datagram is written.
   *
   * @param flags Flags specifying how the receive call is to be made.
   *
   * @param out_flags A forward range of @c socket_base::message_flags, one per
   * datagram, into which the flags associated with each received datagram are
   * written.
   *
   * @returns The number of datagrams received.
   *
   * @throws asio::system_error Thrown on failure.
   *
   * @note The receive_multiple operation can only be used with a connected
   * socket. Use the receive_from_multiple function to receive data on an
   * unconnected datagram socket.
   */
  template <typename MutableBufferSequences, typename MutableSizeSequence,
      typename MutableFlagsSequence>
  std::size_t receive_multiple(MutableBufferSequences&& buffer_sequences,
      MutableSizeSequence&& sizes, socket_base::message_flags flags,
      MutableFlagsSequence&& out_flags,
      constraint_t<
        is_shallow_mutable_sequence<decay_t<MutableFlagsSequence>>::value
      > = 0)
  {
    static_assert(
        is_shallow_mutable_sequence<decay_t<MutableSizeSequence>>::value,
        "sizes must be a shallow mutable sequence");

    asio::error_code ec;
    std::size_t s = this->impl_.get_service().receive_multiple(
        this->impl_.get_implementation(),
        static_cast<MutableBufferSequences&&>(buffer_sequences),
        static_cast<MutableSizeSequence&&>(sizes),
        static_cast<MutableFlagsSequence&&>(out_flags), flags, ec);
    asio::detail::throw_error(ec, "receive_multiple");
    return s;
  }

  /// Receive multiple datagrams and their flags on a connected socket.
  /**
   * This function is used to receive multiple datagrams on the datagram socket.
   * The function call will block until one or more datagrams have been received
   * successfully or an error occurs. The number of datagrams transferred may be
   * less than the length of the shortest range.
   *
   * @param buffer_sequences A forward range of mutable buffer sequences, one
   * per datagram, into which the data will be received.
   *
   * @param sizes A forward range of @c std::size_t, one per datagram, into
   * which the number of bytes received in each datagram is written.
   *
   * @param flags Flags specifying how the receive call is to be made.
   *
   * @param out_flags A forward range of @c socket_base::message_flags, one per
   * datagram, into which the flags associated with each received datagram are
   * written.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @returns The number of datagrams received.
   *
   * @note The receive_multiple operation can only be used with a connected
   * socket. Use the receive_from_multiple function to receive data on an
   * unconnected datagram socket.
   */
  template <typename MutableBufferSequences, typename MutableSizeSequence,
      typename MutableFlagsSequence>
  std::size_t receive_multiple(MutableBufferSequences&& buffer_sequences,
      MutableSizeSequence&& sizes, socket_base::message_flags flags,
      MutableFlagsSequence&& out_flags, asio::error_code& ec,
      constraint_t<
        is_shallow_mutable_sequence<decay_t<MutableFlagsSequence>>::value
      > = 0)
  {
    static_assert(
        is_shallow_mutable_sequence<decay_t<MutableSizeSequence>>::value,
        "sizes must be a shallow mutable sequence");

    return this->impl_.get_service().receive_multiple(
        this->impl_.get_implementation(),
        static_cast<MutableBufferSequences&&>(buffer_sequences),
        static_cast<MutableSizeSequence&&>(sizes),
        static_cast<MutableFlagsSequence&&>(out_flags), flags, ec);
  }

  /// Start an asynchronous receive of multiple datagrams on a connected socket.
  /**
   * This function is used to asynchronously receive multiple datagrams on a
   * connected datagram socket. It is an initiating function for an
   * @ref asynchronous_operation, and always returns immediately. The number of
   * datagrams transferred may be less than the length of the shortest range.
   *
   * @param buffer_sequences A forward range of mutable buffer sequences, one
   * per datagram, into which the data will be received. Although the range
   * object may be copied as necessary, ownership of the underlying memory
   * blocks is retained by the caller, which must guarantee that they remain
   * valid until the completion handler is called.
   *
   * @param sizes A forward range of @c std::size_t, one per datagram, into
   * which the number of bytes received in each datagram is written. Although
   * the range object may be copied as necessary, ownership of the underlying
   * elements is retained by the caller, which must guarantee that they remain
   * valid until the completion handler is called.
   *
   * @param token The @ref completion_token that will be used to produce a
   * completion handler, which will be called when the receive completes.
   * Potential completion tokens include @ref use_future, @ref use_awaitable,
   * @ref yield_context, or a function object with the correct completion
   * signature.
   * The function signature of the completion handler must be:
   * @code void handler(
   *   const asio::error_code& error, // Result of operation.
   *   std::size_t messages_transferred // Number of datagrams received.
   * ); @endcode
   * Regardless of whether the asynchronous operation completes immediately or
   * not, the completion handler will not be invoked from within this function.
   * On immediate completion, invocation of the handler will be performed in a
   * manner equivalent to using asio::async_immediate().
   *
   * @par Completion Signature
   * @code void(asio::error_code, std::size_t) @endcode
   *
   * @note The async_receive_multiple operation can only be used with a
   * connected socket. Use the async_receive_from_multiple function to receive
   * data on an unconnected datagram socket.
   *
   * @par Per-Operation Cancellation
   * On POSIX or Windows operating systems, this asynchronous operation supports
   * cancellation for the following asio::cancellation_type values:
   *
   * @li @c cancellation_type::terminal
   *
   * @li @c cancellation_type::partial
   *
   * @li @c cancellation_type::total
   */
  template <typename MutableBufferSequences, typename MutableSizeSequence,
      ASIO_COMPLETION_TOKEN_FOR(void (asio::error_code,
        std::size_t)) ReadMultiToken =
          default_completion_token_t<executor_type>>
  auto async_receive_multiple(MutableBufferSequences&& buffer_sequences,
      MutableSizeSequence&& sizes,
      ReadMultiToken&& token = default_completion_token_t<executor_type>(),
      constraint_t<
        !is_shallow_mutable_sequence<decay_t<ReadMultiToken>>::value
      > = 0)
    -> decltype(
      async_initiate<ReadMultiToken,
        void (asio::error_code, std::size_t)>(
          declval<initiate_async_receive_multiple>(), token,
          static_cast<MutableBufferSequences&&>(buffer_sequences),
          static_cast<MutableSizeSequence&&>(sizes), detail::empty_range(),
          socket_base::message_flags(0)))
  {
    ASIO_INVOKE_INLINE return async_initiate<ReadMultiToken,
      void (asio::error_code, std::size_t)>(
        initiate_async_receive_multiple(this), token,
        static_cast<MutableBufferSequences&&>(buffer_sequences),
        static_cast<MutableSizeSequence&&>(sizes), detail::empty_range(),
        socket_base::message_flags(0));
  }

  /// Start an asynchronous receive of multiple datagrams on a connected socket.
  /**
   * This function is used to asynchronously receive multiple datagrams on a
   * connected datagram socket. It is an initiating function for an
   * @ref asynchronous_operation, and always returns immediately. The number of
   * datagrams transferred may be less than the length of the shortest range.
   *
   * @param buffer_sequences A forward range of mutable buffer sequences, one
   * per datagram, into which the data will be received. Although the range
   * object may be copied as necessary, ownership of the underlying memory
   * blocks is retained by the caller, which must guarantee that they remain
   * valid until the completion handler is called.
   *
   * @param sizes A forward range of @c std::size_t, one per datagram, into
   * which the number of bytes received in each datagram is written. Although
   * the range object may be copied as necessary, ownership of the underlying
   * elements is retained by the caller, which must guarantee that they remain
   * valid until the completion handler is called.
   *
   * @param flags Flags specifying how the receive call is to be made.
   *
   * @param token The @ref completion_token that will be used to produce a
   * completion handler, which will be called when the receive completes.
   * Potential completion tokens include @ref use_future, @ref use_awaitable,
   * @ref yield_context, or a function object with the correct completion
   * signature.
   * The function signature of the completion handler must be:
   * @code void handler(
   *   const asio::error_code& error, // Result of operation.
   *   std::size_t messages_transferred // Number of datagrams received.
   * ); @endcode
   * Regardless of whether the asynchronous operation completes immediately or
   * not, the completion handler will not be invoked from within this function.
   * On immediate completion, invocation of the handler will be performed in a
   * manner equivalent to using asio::async_immediate().
   *
   * @par Completion Signature
   * @code void(asio::error_code, std::size_t) @endcode
   *
   * @note The async_receive_multiple operation can only be used with a
   * connected socket. Use the async_receive_from_multiple function to receive
   * data on an unconnected datagram socket.
   *
   * @par Per-Operation Cancellation
   * On POSIX or Windows operating systems, this asynchronous operation supports
   * cancellation for the following asio::cancellation_type values:
   *
   * @li @c cancellation_type::terminal
   *
   * @li @c cancellation_type::partial
   *
   * @li @c cancellation_type::total
   */
  template <typename MutableBufferSequences, typename MutableSizeSequence,
      ASIO_COMPLETION_TOKEN_FOR(void (asio::error_code,
        std::size_t)) ReadMultiToken =
          default_completion_token_t<executor_type>>
  auto async_receive_multiple(MutableBufferSequences&& buffer_sequences,
      MutableSizeSequence&& sizes, socket_base::message_flags flags,
      ReadMultiToken&& token = default_completion_token_t<executor_type>(),
      constraint_t<
        !is_shallow_mutable_sequence<decay_t<ReadMultiToken>>::value
      > = 0)
    -> decltype(
      async_initiate<ReadMultiToken,
        void (asio::error_code, std::size_t)>(
          declval<initiate_async_receive_multiple>(), token,
          static_cast<MutableBufferSequences&&>(buffer_sequences),
          static_cast<MutableSizeSequence&&>(sizes), detail::empty_range(),
          flags))
  {
    ASIO_INVOKE_INLINE return async_initiate<ReadMultiToken,
      void (asio::error_code, std::size_t)>(
        initiate_async_receive_multiple(this), token,
        static_cast<MutableBufferSequences&&>(buffer_sequences),
        static_cast<MutableSizeSequence&&>(sizes), detail::empty_range(),
        flags);
  }

  /// Start an asynchronous receive of multiple datagrams and their flags.
  /**
   * This function is used to asynchronously receive multiple datagrams on a
   * connected datagram socket. It is an initiating function for an
   * @ref asynchronous_operation, and always returns immediately. The number of
   * datagrams transferred may be less than the length of the shortest range.
   *
   * @param buffer_sequences A forward range of mutable buffer sequences, one
   * per datagram, into which the data will be received. Although the range
   * object may be copied as necessary, ownership of the underlying memory
   * blocks is retained by the caller, which must guarantee that they remain
   * valid until the completion handler is called.
   *
   * @param sizes A forward range of @c std::size_t, one per datagram, into
   * which the number of bytes received in each datagram is written. Although
   * the range object may be copied as necessary, ownership of the underlying
   * elements is retained by the caller, which must guarantee that they remain
   * valid until the completion handler is called.
   *
   * @param out_flags A forward range of @c socket_base::message_flags, one per
   * datagram, into which the flags associated with each received datagram are
   * written. Although the range object may be copied as necessary, ownership
   * of the underlying elements is retained by the caller, which must guarantee
   * that they remain valid until the completion handler is called.
   *
   * @param token The @ref completion_token that will be used to produce a
   * completion handler, which will be called when the receive completes.
   * Potential completion tokens include @ref use_future, @ref use_awaitable,
   * @ref yield_context, or a function object with the correct completion
   * signature.
   * The function signature of the completion handler must be:
   * @code void handler(
   *   const asio::error_code& error, // Result of operation.
   *   std::size_t messages_transferred // Number of datagrams received.
   * ); @endcode
   * Regardless of whether the asynchronous operation completes immediately or
   * not, the completion handler will not be invoked from within this function.
   * On immediate completion, invocation of the handler will be performed in a
   * manner equivalent to using asio::async_immediate().
   *
   * @par Completion Signature
   * @code void(asio::error_code, std::size_t) @endcode
   *
   * @note The async_receive_multiple operation can only be used with a
   * connected socket. Use the async_receive_from_multiple function to receive
   * data on an unconnected datagram socket.
   *
   * @par Per-Operation Cancellation
   * On POSIX or Windows operating systems, this asynchronous operation supports
   * cancellation for the following asio::cancellation_type values:
   *
   * @li @c cancellation_type::terminal
   *
   * @li @c cancellation_type::partial
   *
   * @li @c cancellation_type::total
   */
  template <typename MutableBufferSequences, typename MutableSizeSequence,
      typename MutableFlagsSequence,
      ASIO_COMPLETION_TOKEN_FOR(void (asio::error_code,
        std::size_t)) ReadMultiToken =
          default_completion_token_t<executor_type>>
  auto async_receive_multiple(MutableBufferSequences&& buffer_sequences,
      MutableSizeSequence&& sizes, MutableFlagsSequence&& out_flags,
      ReadMultiToken&& token = default_completion_token_t<executor_type>(),
      constraint_t<
        is_shallow_mutable_sequence<decay_t<MutableFlagsSequence>>::value
      > = 0)
    -> decltype(
      async_initiate<ReadMultiToken,
        void (asio::error_code, std::size_t)>(
          declval<initiate_async_receive_multiple>(), token,
          static_cast<MutableBufferSequences&&>(buffer_sequences),
          static_cast<MutableSizeSequence&&>(sizes),
          static_cast<MutableFlagsSequence&&>(out_flags),
          socket_base::message_flags(0)))
  {
    ASIO_INVOKE_INLINE return async_initiate<ReadMultiToken,
      void (asio::error_code, std::size_t)>(
        initiate_async_receive_multiple(this), token,
        static_cast<MutableBufferSequences&&>(buffer_sequences),
        static_cast<MutableSizeSequence&&>(sizes),
        static_cast<MutableFlagsSequence&&>(out_flags),
        socket_base::message_flags(0));
  }

  /// Start an asynchronous receive of multiple datagrams and their flags.
  /**
   * This function is used to asynchronously receive multiple datagrams on a
   * connected datagram socket. It is an initiating function for an
   * @ref asynchronous_operation, and always returns immediately. The number of
   * datagrams transferred may be less than the length of the shortest range.
   *
   * @param buffer_sequences A forward range of mutable buffer sequences, one
   * per datagram, into which the data will be received. Although the range
   * object may be copied as necessary, ownership of the underlying memory
   * blocks is retained by the caller, which must guarantee that they remain
   * valid until the completion handler is called.
   *
   * @param sizes A forward range of @c std::size_t, one per datagram, into
   * which the number of bytes received in each datagram is written. Although
   * the range object may be copied as necessary, ownership of the underlying
   * elements is retained by the caller, which must guarantee that they remain
   * valid until the completion handler is called.
   *
   * @param flags Flags specifying how the receive call is to be made.
   *
   * @param out_flags A forward range of @c socket_base::message_flags, one per
   * datagram, into which the flags associated with each received datagram are
   * written. Although the range object may be copied as necessary, ownership
   * of the underlying elements is retained by the caller, which must guarantee
   * that they remain valid until the completion handler is called.
   *
   * @param token The @ref completion_token that will be used to produce a
   * completion handler, which will be called when the receive completes.
   * Potential completion tokens include @ref use_future, @ref use_awaitable,
   * @ref yield_context, or a function object with the correct completion
   * signature.
   * The function signature of the completion handler must be:
   * @code void handler(
   *   const asio::error_code& error, // Result of operation.
   *   std::size_t messages_transferred // Number of datagrams received.
   * ); @endcode
   * Regardless of whether the asynchronous operation completes immediately or
   * not, the completion handler will not be invoked from within this function.
   * On immediate completion, invocation of the handler will be performed in a
   * manner equivalent to using asio::async_immediate().
   *
   * @par Completion Signature
   * @code void(asio::error_code, std::size_t) @endcode
   *
   * @note The async_receive_multiple operation can only be used with a
   * connected socket. Use the async_receive_from_multiple function to receive
   * data on an unconnected datagram socket.
   *
   * @par Per-Operation Cancellation
   * On POSIX or Windows operating systems, this asynchronous operation supports
   * cancellation for the following asio::cancellation_type values:
   *
   * @li @c cancellation_type::terminal
   *
   * @li @c cancellation_type::partial
   *
   * @li @c cancellation_type::total
   */
  template <typename MutableBufferSequences, typename MutableSizeSequence,
      typename MutableFlagsSequence,
      ASIO_COMPLETION_TOKEN_FOR(void (asio::error_code,
        std::size_t)) ReadMultiToken =
          default_completion_token_t<executor_type>>
  auto async_receive_multiple(MutableBufferSequences&& buffer_sequences,
      MutableSizeSequence&& sizes, socket_base::message_flags flags,
      MutableFlagsSequence&& out_flags,
      ReadMultiToken&& token = default_completion_token_t<executor_type>(),
      constraint_t<
        is_shallow_mutable_sequence<decay_t<MutableFlagsSequence>>::value
      > = 0)
    -> decltype(
      async_initiate<ReadMultiToken,
        void (asio::error_code, std::size_t)>(
          declval<initiate_async_receive_multiple>(), token,
          static_cast<MutableBufferSequences&&>(buffer_sequences),
          static_cast<MutableSizeSequence&&>(sizes),
          static_cast<MutableFlagsSequence&&>(out_flags), flags))
  {
    ASIO_INVOKE_INLINE return async_initiate<ReadMultiToken,
      void (asio::error_code, std::size_t)>(
        initiate_async_receive_multiple(this), token,
        static_cast<MutableBufferSequences&&>(buffer_sequences),
        static_cast<MutableSizeSequence&&>(sizes),
        static_cast<MutableFlagsSequence&&>(out_flags), flags);
  }

  /// Receive multiple datagrams with the endpoints of the senders.
  /**
   * This function is used to receive multiple datagrams, recording the endpoint
   * of the remote sender of each. The function call will block until one or
   * more datagrams have been received successfully or an error occurs. The
   * number of datagrams transferred may be less than the length of the shortest
   * range.
   *
   * @param buffer_sequences A forward range of mutable buffer sequences, one
   * per datagram, into which the data will be received.
   *
   * @param endpoints A forward range of endpoint objects, one per datagram,
   * that receive the endpoint of the remote sender of each datagram.
   *
   * @param sizes A forward range of @c std::size_t, one per datagram, into
   * which the number of bytes received in each datagram is written.
   *
   * @returns The number of datagrams received.
   *
   * @throws asio::system_error Thrown on failure.
   */
  template <typename MutableBufferSequences, typename MutableEndpointSequence,
      typename MutableSizeSequence>
  std::size_t receive_from_multiple(MutableBufferSequences&& buffer_sequences,
      MutableEndpointSequence&& endpoints, MutableSizeSequence&& sizes)
  {
    static_assert(
        is_shallow_mutable_sequence<decay_t<MutableEndpointSequence>>::value,
        "endpoints must be a shallow mutable sequence");
    static_assert(
        is_shallow_mutable_sequence<decay_t<MutableSizeSequence>>::value,
        "sizes must be a shallow mutable sequence");

    asio::error_code ec;
    std::size_t s = this->impl_.get_service().receive_from_multiple(
        this->impl_.get_implementation(),
        static_cast<MutableBufferSequences&&>(buffer_sequences),
        static_cast<MutableEndpointSequence&&>(endpoints),
        static_cast<MutableSizeSequence&&>(sizes), detail::empty_range(),
        0, ec);
    asio::detail::throw_error(ec, "receive_from_multiple");
    return s;
  }

  /// Receive multiple datagrams with the endpoints of the senders.
  /**
   * This function is used to receive multiple datagrams, recording the endpoint
   * of the remote sender of each. The function call will block until one or
   * more datagrams have been received successfully or an error occurs. The
   * number of datagrams transferred may be less than the length of the shortest
   * range.
   *
   * @param buffer_sequences A forward range of mutable buffer sequences, one
   * per datagram, into which the data will be received.
   *
   * @param endpoints A forward range of endpoint objects, one per datagram,
   * that receive the endpoint of the remote sender of each datagram.
   *
   * @param sizes A forward range of @c std::size_t, one per datagram, into
   * which the number of bytes received in each datagram is written.
   *
   * @param flags Flags specifying how the receive call is to be made.
   *
   * @returns The number of datagrams received.
   *
   * @throws asio::system_error Thrown on failure.
   */
  template <typename MutableBufferSequences, typename MutableEndpointSequence,
      typename MutableSizeSequence>
  std::size_t receive_from_multiple(MutableBufferSequences&& buffer_sequences,
      MutableEndpointSequence&& endpoints, MutableSizeSequence&& sizes,
      socket_base::message_flags flags)
  {
    static_assert(
        is_shallow_mutable_sequence<decay_t<MutableEndpointSequence>>::value,
        "endpoints must be a shallow mutable sequence");
    static_assert(
        is_shallow_mutable_sequence<decay_t<MutableSizeSequence>>::value,
        "sizes must be a shallow mutable sequence");

    asio::error_code ec;
    std::size_t s = this->impl_.get_service().receive_from_multiple(
        this->impl_.get_implementation(),
        static_cast<MutableBufferSequences&&>(buffer_sequences),
        static_cast<MutableEndpointSequence&&>(endpoints),
        static_cast<MutableSizeSequence&&>(sizes), detail::empty_range(),
        flags, ec);
    asio::detail::throw_error(ec, "receive_from_multiple");
    return s;
  }

  /// Receive multiple datagrams with the endpoints of the senders.
  /**
   * This function is used to receive multiple datagrams, recording the endpoint
   * of the remote sender of each. The function call will block until one or
   * more datagrams have been received successfully or an error occurs. The
   * number of datagrams transferred may be less than the length of the shortest
   * range.
   *
   * @param buffer_sequences A forward range of mutable buffer sequences, one
   * per datagram, into which the data will be received.
   *
   * @param endpoints A forward range of endpoint objects, one per datagram,
   * that receive the endpoint of the remote sender of each datagram.
   *
   * @param sizes A forward range of @c std::size_t, one per datagram, into
   * which the number of bytes received in each datagram is written.
   *
   * @param flags Flags specifying how the receive call is to be made.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @returns The number of datagrams received.
   */
  template <typename MutableBufferSequences, typename MutableEndpointSequence,
      typename MutableSizeSequence>
  std::size_t receive_from_multiple(MutableBufferSequences&& buffer_sequences,
      MutableEndpointSequence&& endpoints, MutableSizeSequence&& sizes,
      socket_base::message_flags flags, asio::error_code& ec)
  {
    static_assert(
        is_shallow_mutable_sequence<decay_t<MutableEndpointSequence>>::value,
        "endpoints must be a shallow mutable sequence");
    static_assert(
        is_shallow_mutable_sequence<decay_t<MutableSizeSequence>>::value,
        "sizes must be a shallow mutable sequence");

    return this->impl_.get_service().receive_from_multiple(
        this->impl_.get_implementation(),
        static_cast<MutableBufferSequences&&>(buffer_sequences),
        static_cast<MutableEndpointSequence&&>(endpoints),
        static_cast<MutableSizeSequence&&>(sizes), detail::empty_range(),
        flags, ec);
  }

  /// Receive multiple datagrams and their flags with the sender endpoints.
  /**
   * This function is used to receive multiple datagrams, recording the endpoint
   * of the remote sender of each. The function call will block until one or
   * more datagrams have been received successfully or an error occurs. The
   * number of datagrams transferred may be less than the length of the shortest
   * range.
   *
   * @param buffer_sequences A forward range of mutable buffer sequences, one
   * per datagram, into which the data will be received.
   *
   * @param endpoints A forward range of endpoint objects, one per datagram,
   * that receive the endpoint of the remote sender of each datagram.
   *
   * @param sizes A forward range of @c std::size_t, one per datagram, into
   * which the number of bytes received in each datagram is written.
   *
   * @param out_flags A forward range of @c socket_base::message_flags, one per
   * datagram, into which the flags associated with each received datagram are
   * written.
   *
   * @returns The number of datagrams received.
   *
   * @throws asio::system_error Thrown on failure.
   */
  template <typename MutableBufferSequences, typename MutableEndpointSequence,
      typename MutableSizeSequence, typename MutableFlagsSequence>
  std::size_t receive_from_multiple(MutableBufferSequences&& buffer_sequences,
      MutableEndpointSequence&& endpoints, MutableSizeSequence&& sizes,
      MutableFlagsSequence&& out_flags,
      constraint_t<
        is_shallow_mutable_sequence<decay_t<MutableFlagsSequence>>::value
      > = 0)
  {
    static_assert(
        is_shallow_mutable_sequence<decay_t<MutableEndpointSequence>>::value,
        "endpoints must be a shallow mutable sequence");
    static_assert(
        is_shallow_mutable_sequence<decay_t<MutableSizeSequence>>::value,
        "sizes must be a shallow mutable sequence");

    asio::error_code ec;
    std::size_t s = this->impl_.get_service().receive_from_multiple(
        this->impl_.get_implementation(),
        static_cast<MutableBufferSequences&&>(buffer_sequences),
        static_cast<MutableEndpointSequence&&>(endpoints),
        static_cast<MutableSizeSequence&&>(sizes),
        static_cast<MutableFlagsSequence&&>(out_flags), 0, ec);
    asio::detail::throw_error(ec, "receive_from_multiple");
    return s;
  }

  /// Receive multiple datagrams and their flags with the sender endpoints.
  /**
   * This function is used to receive multiple datagrams, recording the endpoint
   * of the remote sender of each. The function call will block until one or
   * more datagrams have been received successfully or an error occurs. The
   * number of datagrams transferred may be less than the length of the shortest
   * range.
   *
   * @param buffer_sequences A forward range of mutable buffer sequences, one
   * per datagram, into which the data will be received.
   *
   * @param endpoints A forward range of endpoint objects, one per datagram,
   * that receive the endpoint of the remote sender of each datagram.
   *
   * @param sizes A forward range of @c std::size_t, one per datagram, into
   * which the number of bytes received in each datagram is written.
   *
   * @param flags Flags specifying how the receive call is to be made.
   *
   * @param out_flags A forward range of @c socket_base::message_flags, one per
   * datagram, into which the flags associated with each received datagram are
   * written.
   *
   * @returns The number of datagrams received.
   *
   * @throws asio::system_error Thrown on failure.
   */
  template <typename MutableBufferSequences, typename MutableEndpointSequence,
      typename MutableSizeSequence, typename MutableFlagsSequence>
  std::size_t receive_from_multiple(MutableBufferSequences&& buffer_sequences,
      MutableEndpointSequence&& endpoints, MutableSizeSequence&& sizes,
      socket_base::message_flags flags, MutableFlagsSequence&& out_flags,
      constraint_t<
        is_shallow_mutable_sequence<decay_t<MutableFlagsSequence>>::value
      > = 0)
  {
    static_assert(
        is_shallow_mutable_sequence<decay_t<MutableEndpointSequence>>::value,
        "endpoints must be a shallow mutable sequence");
    static_assert(
        is_shallow_mutable_sequence<decay_t<MutableSizeSequence>>::value,
        "sizes must be a shallow mutable sequence");

    asio::error_code ec;
    std::size_t s = this->impl_.get_service().receive_from_multiple(
        this->impl_.get_implementation(),
        static_cast<MutableBufferSequences&&>(buffer_sequences),
        static_cast<MutableEndpointSequence&&>(endpoints),
        static_cast<MutableSizeSequence&&>(sizes),
        static_cast<MutableFlagsSequence&&>(out_flags), flags, ec);
    asio::detail::throw_error(ec, "receive_from_multiple");
    return s;
  }

  /// Receive multiple datagrams and their flags with the sender endpoints.
  /**
   * This function is used to receive multiple datagrams, recording the endpoint
   * of the remote sender of each. The function call will block until one or
   * more datagrams have been received successfully or an error occurs. The
   * number of datagrams transferred may be less than the length of the shortest
   * range.
   *
   * @param buffer_sequences A forward range of mutable buffer sequences, one
   * per datagram, into which the data will be received.
   *
   * @param endpoints A forward range of endpoint objects, one per datagram,
   * that receive the endpoint of the remote sender of each datagram.
   *
   * @param sizes A forward range of @c std::size_t, one per datagram, into
   * which the number of bytes received in each datagram is written.
   *
   * @param flags Flags specifying how the receive call is to be made.
   *
   * @param out_flags A forward range of @c socket_base::message_flags, one per
   * datagram, into which the flags associated with each received datagram are
   * written.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @returns The number of datagrams received.
   */
  template <typename MutableBufferSequences, typename MutableEndpointSequence,
      typename MutableSizeSequence, typename MutableFlagsSequence>
  std::size_t receive_from_multiple(MutableBufferSequences&& buffer_sequences,
      MutableEndpointSequence&& endpoints, MutableSizeSequence&& sizes,
      socket_base::message_flags flags, MutableFlagsSequence&& out_flags,
      asio::error_code& ec,
      constraint_t<
        is_shallow_mutable_sequence<decay_t<MutableFlagsSequence>>::value
      > = 0)
  {
    static_assert(
        is_shallow_mutable_sequence<decay_t<MutableEndpointSequence>>::value,
        "endpoints must be a shallow mutable sequence");
    static_assert(
        is_shallow_mutable_sequence<decay_t<MutableSizeSequence>>::value,
        "sizes must be a shallow mutable sequence");

    return this->impl_.get_service().receive_from_multiple(
        this->impl_.get_implementation(),
        static_cast<MutableBufferSequences&&>(buffer_sequences),
        static_cast<MutableEndpointSequence&&>(endpoints),
        static_cast<MutableSizeSequence&&>(sizes),
        static_cast<MutableFlagsSequence&&>(out_flags), flags, ec);
  }

  /// Start an asynchronous receive of multiple datagrams with their senders.
  /**
   * This function is used to asynchronously receive multiple datagrams,
   * recording the endpoint of the remote sender of each. It is an initiating
   * function for an @ref asynchronous_operation, and always returns
   * immediately. The number of datagrams transferred may be less than the
   * length of the shortest range.
   *
   * @param buffer_sequences A forward range of mutable buffer sequences, one
   * per datagram, into which the data will be received. Although the range
   * object may be copied as necessary, ownership of the underlying memory
   * blocks is retained by the caller, which must guarantee that they remain
   * valid until the completion handler is called.
   *
   * @param endpoints A forward range of endpoint objects, one per datagram,
   * that receive the endpoint of the remote sender of each datagram. Although
   * the range object may be copied as necessary, ownership of the underlying
   * endpoint objects is retained by the caller, which must guarantee that they
   * remain valid until the completion handler is called.
   *
   * @param sizes A forward range of @c std::size_t, one per datagram, into
   * which the number of bytes received in each datagram is written. Although
   * the range object may be copied as necessary, ownership of the underlying
   * elements is retained by the caller, which must guarantee that they remain
   * valid until the completion handler is called.
   *
   * @param token The @ref completion_token that will be used to produce a
   * completion handler, which will be called when the receive completes.
   * Potential completion tokens include @ref use_future, @ref use_awaitable,
   * @ref yield_context, or a function object with the correct completion
   * signature.
   * The function signature of the completion handler must be:
   * @code void handler(
   *   const asio::error_code& error, // Result of operation.
   *   std::size_t messages_transferred // Number of datagrams received.
   * ); @endcode
   * Regardless of whether the asynchronous operation completes immediately or
   * not, the completion handler will not be invoked from within this function.
   * On immediate completion, invocation of the handler will be performed in a
   * manner equivalent to using asio::async_immediate().
   *
   * @par Completion Signature
   * @code void(asio::error_code, std::size_t) @endcode
   *
   * @par Per-Operation Cancellation
   * On POSIX or Windows operating systems, this asynchronous operation supports
   * cancellation for the following asio::cancellation_type values:
   *
   * @li @c cancellation_type::terminal
   *
   * @li @c cancellation_type::partial
   *
   * @li @c cancellation_type::total
   */
  template <typename MutableBufferSequences, typename MutableEndpointSequence,
      typename MutableSizeSequence,
      ASIO_COMPLETION_TOKEN_FOR(void (asio::error_code,
        std::size_t)) ReadMultiToken =
          default_completion_token_t<executor_type>>
  auto async_receive_from_multiple(MutableBufferSequences&& buffer_sequences,
      MutableEndpointSequence&& endpoints, MutableSizeSequence&& sizes,
      ReadMultiToken&& token = default_completion_token_t<executor_type>(),
      constraint_t<
        !is_shallow_mutable_sequence<decay_t<ReadMultiToken>>::value
      > = 0)
    -> decltype(
      async_initiate<ReadMultiToken,
        void (asio::error_code, std::size_t)>(
          declval<initiate_async_receive_from_multiple>(), token,
          static_cast<MutableBufferSequences&&>(buffer_sequences),
          static_cast<MutableEndpointSequence&&>(endpoints),
          static_cast<MutableSizeSequence&&>(sizes), detail::empty_range(),
          socket_base::message_flags(0)))
  {
    ASIO_INVOKE_INLINE return async_initiate<ReadMultiToken,
      void (asio::error_code, std::size_t)>(
        initiate_async_receive_from_multiple(this), token,
        static_cast<MutableBufferSequences&&>(buffer_sequences),
        static_cast<MutableEndpointSequence&&>(endpoints),
        static_cast<MutableSizeSequence&&>(sizes), detail::empty_range(),
        socket_base::message_flags(0));
  }

  /// Start an asynchronous receive of multiple datagrams with their senders.
  /**
   * This function is used to asynchronously receive multiple datagrams,
   * recording the endpoint of the remote sender of each. It is an initiating
   * function for an @ref asynchronous_operation, and always returns
   * immediately. The number of datagrams transferred may be less than the
   * length of the shortest range.
   *
   * @param buffer_sequences A forward range of mutable buffer sequences, one
   * per datagram, into which the data will be received. Although the range
   * object may be copied as necessary, ownership of the underlying memory
   * blocks is retained by the caller, which must guarantee that they remain
   * valid until the completion handler is called.
   *
   * @param endpoints A forward range of endpoint objects, one per datagram,
   * that receive the endpoint of the remote sender of each datagram. Although
   * the range object may be copied as necessary, ownership of the underlying
   * endpoint objects is retained by the caller, which must guarantee that they
   * remain valid until the completion handler is called.
   *
   * @param sizes A forward range of @c std::size_t, one per datagram, into
   * which the number of bytes received in each datagram is written. Although
   * the range object may be copied as necessary, ownership of the underlying
   * elements is retained by the caller, which must guarantee that they remain
   * valid until the completion handler is called.
   *
   * @param flags Flags specifying how the receive call is to be made.
   *
   * @param token The @ref completion_token that will be used to produce a
   * completion handler, which will be called when the receive completes.
   * Potential completion tokens include @ref use_future, @ref use_awaitable,
   * @ref yield_context, or a function object with the correct completion
   * signature.
   * The function signature of the completion handler must be:
   * @code void handler(
   *   const asio::error_code& error, // Result of operation.
   *   std::size_t messages_transferred // Number of datagrams received.
   * ); @endcode
   * Regardless of whether the asynchronous operation completes immediately or
   * not, the completion handler will not be invoked from within this function.
   * On immediate completion, invocation of the handler will be performed in a
   * manner equivalent to using asio::async_immediate().
   *
   * @par Completion Signature
   * @code void(asio::error_code, std::size_t) @endcode
   *
   * @par Per-Operation Cancellation
   * On POSIX or Windows operating systems, this asynchronous operation supports
   * cancellation for the following asio::cancellation_type values:
   *
   * @li @c cancellation_type::terminal
   *
   * @li @c cancellation_type::partial
   *
   * @li @c cancellation_type::total
   */
  template <typename MutableBufferSequences, typename MutableEndpointSequence,
      typename MutableSizeSequence,
      ASIO_COMPLETION_TOKEN_FOR(void (asio::error_code,
        std::size_t)) ReadMultiToken =
          default_completion_token_t<executor_type>>
  auto async_receive_from_multiple(MutableBufferSequences&& buffer_sequences,
      MutableEndpointSequence&& endpoints, MutableSizeSequence&& sizes,
      socket_base::message_flags flags,
      ReadMultiToken&& token = default_completion_token_t<executor_type>(),
      constraint_t<
        !is_shallow_mutable_sequence<decay_t<ReadMultiToken>>::value
      > = 0)
    -> decltype(
      async_initiate<ReadMultiToken,
        void (asio::error_code, std::size_t)>(
          declval<initiate_async_receive_from_multiple>(), token,
          static_cast<MutableBufferSequences&&>(buffer_sequences),
          static_cast<MutableEndpointSequence&&>(endpoints),
          static_cast<MutableSizeSequence&&>(sizes), detail::empty_range(),
          flags))
  {
    ASIO_INVOKE_INLINE return async_initiate<ReadMultiToken,
      void (asio::error_code, std::size_t)>(
        initiate_async_receive_from_multiple(this), token,
        static_cast<MutableBufferSequences&&>(buffer_sequences),
        static_cast<MutableEndpointSequence&&>(endpoints),
        static_cast<MutableSizeSequence&&>(sizes), detail::empty_range(),
        flags);
  }

  /// Start an asynchronous receive of datagrams with their senders and flags.
  /**
   * This function is used to asynchronously receive multiple datagrams,
   * recording the endpoint of the remote sender of each. It is an initiating
   * function for an @ref asynchronous_operation, and always returns
   * immediately. The number of datagrams transferred may be less than the
   * length of the shortest range.
   *
   * @param buffer_sequences A forward range of mutable buffer sequences, one
   * per datagram, into which the data will be received. Although the range
   * object may be copied as necessary, ownership of the underlying memory
   * blocks is retained by the caller, which must guarantee that they remain
   * valid until the completion handler is called.
   *
   * @param endpoints A forward range of endpoint objects, one per datagram,
   * that receive the endpoint of the remote sender of each datagram. Although
   * the range object may be copied as necessary, ownership of the underlying
   * endpoint objects is retained by the caller, which must guarantee that they
   * remain valid until the completion handler is called.
   *
   * @param sizes A forward range of @c std::size_t, one per datagram, into
   * which the number of bytes received in each datagram is written. Although
   * the range object may be copied as necessary, ownership of the underlying
   * elements is retained by the caller, which must guarantee that they remain
   * valid until the completion handler is called.
   *
   * @param out_flags A forward range of @c socket_base::message_flags, one per
   * datagram, into which the flags associated with each received datagram are
   * written. Although the range object may be copied as necessary, ownership
   * of the underlying elements is retained by the caller, which must guarantee
   * that they remain valid until the completion handler is called.
   *
   * @param token The @ref completion_token that will be used to produce a
   * completion handler, which will be called when the receive completes.
   * Potential completion tokens include @ref use_future, @ref use_awaitable,
   * @ref yield_context, or a function object with the correct completion
   * signature.
   * The function signature of the completion handler must be:
   * @code void handler(
   *   const asio::error_code& error, // Result of operation.
   *   std::size_t messages_transferred // Number of datagrams received.
   * ); @endcode
   * Regardless of whether the asynchronous operation completes immediately or
   * not, the completion handler will not be invoked from within this function.
   * On immediate completion, invocation of the handler will be performed in a
   * manner equivalent to using asio::async_immediate().
   *
   * @par Completion Signature
   * @code void(asio::error_code, std::size_t) @endcode
   *
   * @par Per-Operation Cancellation
   * On POSIX or Windows operating systems, this asynchronous operation supports
   * cancellation for the following asio::cancellation_type values:
   *
   * @li @c cancellation_type::terminal
   *
   * @li @c cancellation_type::partial
   *
   * @li @c cancellation_type::total
   */
  template <typename MutableBufferSequences, typename MutableEndpointSequence,
      typename MutableSizeSequence, typename MutableFlagsSequence,
      ASIO_COMPLETION_TOKEN_FOR(void (asio::error_code,
        std::size_t)) ReadMultiToken =
          default_completion_token_t<executor_type>>
  auto async_receive_from_multiple(MutableBufferSequences&& buffer_sequences,
      MutableEndpointSequence&& endpoints, MutableSizeSequence&& sizes,
      MutableFlagsSequence&& out_flags,
      ReadMultiToken&& token = default_completion_token_t<executor_type>(),
      constraint_t<
        is_shallow_mutable_sequence<decay_t<MutableFlagsSequence>>::value
      > = 0)
    -> decltype(
      async_initiate<ReadMultiToken,
        void (asio::error_code, std::size_t)>(
          declval<initiate_async_receive_from_multiple>(), token,
          static_cast<MutableBufferSequences&&>(buffer_sequences),
          static_cast<MutableEndpointSequence&&>(endpoints),
          static_cast<MutableSizeSequence&&>(sizes),
          static_cast<MutableFlagsSequence&&>(out_flags),
          socket_base::message_flags(0)))
  {
    ASIO_INVOKE_INLINE return async_initiate<ReadMultiToken,
      void (asio::error_code, std::size_t)>(
        initiate_async_receive_from_multiple(this), token,
        static_cast<MutableBufferSequences&&>(buffer_sequences),
        static_cast<MutableEndpointSequence&&>(endpoints),
        static_cast<MutableSizeSequence&&>(sizes),
        static_cast<MutableFlagsSequence&&>(out_flags),
        socket_base::message_flags(0));
  }

  /// Start an asynchronous receive of datagrams with their senders and flags.
  /**
   * This function is used to asynchronously receive multiple datagrams,
   * recording the endpoint of the remote sender of each. It is an initiating
   * function for an @ref asynchronous_operation, and always returns
   * immediately. The number of datagrams transferred may be less than the
   * length of the shortest range.
   *
   * @param buffer_sequences A forward range of mutable buffer sequences, one
   * per datagram, into which the data will be received. Although the range
   * object may be copied as necessary, ownership of the underlying memory
   * blocks is retained by the caller, which must guarantee that they remain
   * valid until the completion handler is called.
   *
   * @param endpoints A forward range of endpoint objects, one per datagram,
   * that receive the endpoint of the remote sender of each datagram. Although
   * the range object may be copied as necessary, ownership of the underlying
   * endpoint objects is retained by the caller, which must guarantee that they
   * remain valid until the completion handler is called.
   *
   * @param sizes A forward range of @c std::size_t, one per datagram, into
   * which the number of bytes received in each datagram is written. Although
   * the range object may be copied as necessary, ownership of the underlying
   * elements is retained by the caller, which must guarantee that they remain
   * valid until the completion handler is called.
   *
   * @param flags Flags specifying how the receive call is to be made.
   *
   * @param out_flags A forward range of @c socket_base::message_flags, one per
   * datagram, into which the flags associated with each received datagram are
   * written. Although the range object may be copied as necessary, ownership
   * of the underlying elements is retained by the caller, which must guarantee
   * that they remain valid until the completion handler is called.
   *
   * @param token The @ref completion_token that will be used to produce a
   * completion handler, which will be called when the receive completes.
   * Potential completion tokens include @ref use_future, @ref use_awaitable,
   * @ref yield_context, or a function object with the correct completion
   * signature.
   * The function signature of the completion handler must be:
   * @code void handler(
   *   const asio::error_code& error, // Result of operation.
   *   std::size_t messages_transferred // Number of datagrams received.
   * ); @endcode
   * Regardless of whether the asynchronous operation completes immediately or
   * not, the completion handler will not be invoked from within this function.
   * On immediate completion, invocation of the handler will be performed in a
   * manner equivalent to using asio::async_immediate().
   *
   * @par Completion Signature
   * @code void(asio::error_code, std::size_t) @endcode
   *
   * @par Per-Operation Cancellation
   * On POSIX or Windows operating systems, this asynchronous operation supports
   * cancellation for the following asio::cancellation_type values:
   *
   * @li @c cancellation_type::terminal
   *
   * @li @c cancellation_type::partial
   *
   * @li @c cancellation_type::total
   */
  template <typename MutableBufferSequences, typename MutableEndpointSequence,
      typename MutableSizeSequence, typename MutableFlagsSequence,
      ASIO_COMPLETION_TOKEN_FOR(void (asio::error_code,
        std::size_t)) ReadMultiToken =
          default_completion_token_t<executor_type>>
  auto async_receive_from_multiple(MutableBufferSequences&& buffer_sequences,
      MutableEndpointSequence&& endpoints, MutableSizeSequence&& sizes,
      socket_base::message_flags flags, MutableFlagsSequence&& out_flags,
      ReadMultiToken&& token = default_completion_token_t<executor_type>(),
      constraint_t<
        is_shallow_mutable_sequence<decay_t<MutableFlagsSequence>>::value
      > = 0)
    -> decltype(
      async_initiate<ReadMultiToken,
        void (asio::error_code, std::size_t)>(
          declval<initiate_async_receive_from_multiple>(), token,
          static_cast<MutableBufferSequences&&>(buffer_sequences),
          static_cast<MutableEndpointSequence&&>(endpoints),
          static_cast<MutableSizeSequence&&>(sizes),
          static_cast<MutableFlagsSequence&&>(out_flags), flags))
  {
    ASIO_INVOKE_INLINE return async_initiate<ReadMultiToken,
      void (asio::error_code, std::size_t)>(
        initiate_async_receive_from_multiple(this), token,
        static_cast<MutableBufferSequences&&>(buffer_sequences),
        static_cast<MutableEndpointSequence&&>(endpoints),
        static_cast<MutableSizeSequence&&>(sizes),
        static_cast<MutableFlagsSequence&&>(out_flags), flags);
  }
private:
  // Disallow copying and assignment.
  basic_datagram_socket(const basic_datagram_socket&) = delete;
  basic_datagram_socket& operator=(
      const basic_datagram_socket&) = delete;

  class initiate_async_send
  {
  public:
    typedef Executor executor_type;

    explicit initiate_async_send(basic_datagram_socket* self)
      : self_(self)
    {
    }

    const executor_type& get_executor() const noexcept
    {
      return self_->get_executor();
    }

    template <typename WriteHandler, typename ConstBufferSequence>
    void operator()(WriteHandler&& handler,
        const ConstBufferSequence& buffers,
        socket_base::message_flags flags) const
    {
      // If you get an error on the following line it means that your handler
      // does not meet the documented type requirements for a WriteHandler.
      ASIO_WRITE_HANDLER_CHECK(WriteHandler, handler) type_check;

      detail::non_const_lvalue<WriteHandler> handler2(handler);
      ASIO_INVOKE_INLINE self_->impl_.get_service().async_send(
          self_->impl_.get_implementation(), buffers, flags,
          handler2.value, self_->impl_.get_executor());
    }

  private:
    basic_datagram_socket* self_;
  };

  class initiate_async_send_to
  {
  public:
    typedef Executor executor_type;

    explicit initiate_async_send_to(basic_datagram_socket* self)
      : self_(self)
    {
    }

    const executor_type& get_executor() const noexcept
    {
      return self_->get_executor();
    }

    template <typename WriteHandler, typename ConstBufferSequence>
    void operator()(WriteHandler&& handler,
        const ConstBufferSequence& buffers, const endpoint_type& destination,
        socket_base::message_flags flags) const
    {
      // If you get an error on the following line it means that your handler
      // does not meet the documented type requirements for a WriteHandler.
      ASIO_WRITE_HANDLER_CHECK(WriteHandler, handler) type_check;

      detail::non_const_lvalue<WriteHandler> handler2(handler);
      ASIO_INVOKE_INLINE self_->impl_.get_service().async_send_to(
          self_->impl_.get_implementation(), buffers, destination,
          flags, handler2.value, self_->impl_.get_executor());
    }

  private:
    basic_datagram_socket* self_;
  };

  class initiate_async_receive
  {
  public:
    typedef Executor executor_type;

    explicit initiate_async_receive(basic_datagram_socket* self)
      : self_(self)
    {
    }

    const executor_type& get_executor() const noexcept
    {
      return self_->get_executor();
    }

    template <typename ReadHandler, typename MutableBufferSequence>
    void operator()(ReadHandler&& handler,
        const MutableBufferSequence& buffers,
        socket_base::message_flags flags) const
    {
      // If you get an error on the following line it means that your handler
      // does not meet the documented type requirements for a ReadHandler.
      ASIO_READ_HANDLER_CHECK(ReadHandler, handler) type_check;

      detail::non_const_lvalue<ReadHandler> handler2(handler);
      ASIO_INVOKE_INLINE self_->impl_.get_service().async_receive(
          self_->impl_.get_implementation(), buffers, flags,
          handler2.value, self_->impl_.get_executor());
    }

  private:
    basic_datagram_socket* self_;
  };

  class initiate_async_receive_from
  {
  public:
    typedef Executor executor_type;

    explicit initiate_async_receive_from(basic_datagram_socket* self)
      : self_(self)
    {
    }

    const executor_type& get_executor() const noexcept
    {
      return self_->get_executor();
    }

    template <typename ReadHandler, typename MutableBufferSequence>
    void operator()(ReadHandler&& handler,
        const MutableBufferSequence& buffers, endpoint_type* sender_endpoint,
        socket_base::message_flags flags) const
    {
      // If you get an error on the following line it means that your handler
      // does not meet the documented type requirements for a ReadHandler.
      ASIO_READ_HANDLER_CHECK(ReadHandler, handler) type_check;

      detail::non_const_lvalue<ReadHandler> handler2(handler);
      ASIO_INVOKE_INLINE self_->impl_.get_service().async_receive_from(
          self_->impl_.get_implementation(), buffers, *sender_endpoint,
          flags, handler2.value, self_->impl_.get_executor());
    }

  private:
    basic_datagram_socket* self_;
  };

  class initiate_async_send_multiple
  {
  public:
    typedef Executor executor_type;

    explicit initiate_async_send_multiple(basic_datagram_socket* self)
      : self_(self)
    {
    }

    const executor_type& get_executor() const noexcept
    {
      return self_->get_executor();
    }

    template <typename WriteMultiHandler, typename ConstBufferSequences>
    void operator()(WriteMultiHandler&& handler,
        ConstBufferSequences&& buffer_sequences,
        socket_base::message_flags flags) const
    {
      // If you get an error on the following line it means that your handler
      // does not meet the documented type requirements for a WriteMultiHandler.
      ASIO_WRITE_MULTI_HANDLER_CHECK(
          WriteMultiHandler, handler) type_check;

      detail::non_const_lvalue<WriteMultiHandler> handler2(handler);
      ASIO_INVOKE_INLINE self_->impl_.get_service().async_send_multiple(
          self_->impl_.get_implementation(),
          static_cast<ConstBufferSequences&&>(buffer_sequences), flags,
          handler2.value, self_->impl_.get_executor());
    }

  private:
    basic_datagram_socket* self_;
  };

  class initiate_async_send_to_multiple
  {
  public:
    typedef Executor executor_type;

    explicit initiate_async_send_to_multiple(basic_datagram_socket* self)
      : self_(self)
    {
    }

    const executor_type& get_executor() const noexcept
    {
      return self_->get_executor();
    }

    template <typename WriteMultiHandler, typename ConstBufferSequences,
        typename EndpointSequence>
    void operator()(WriteMultiHandler&& handler,
        ConstBufferSequences&& buffer_sequences,
        EndpointSequence&& endpoints,
        socket_base::message_flags flags) const
    {
      // If you get an error on the following line it means that your handler
      // does not meet the documented type requirements for a WriteMultiHandler.
      ASIO_WRITE_MULTI_HANDLER_CHECK(
          WriteMultiHandler, handler) type_check;

      detail::non_const_lvalue<WriteMultiHandler> handler2(handler);
      ASIO_INVOKE_INLINE
      self_->impl_.get_service().async_send_to_multiple(
          self_->impl_.get_implementation(),
          static_cast<ConstBufferSequences&&>(buffer_sequences),
          static_cast<EndpointSequence&&>(endpoints), flags,
          handler2.value, self_->impl_.get_executor());
    }

  private:
    basic_datagram_socket* self_;
  };

  class initiate_async_receive_multiple
  {
  public:
    typedef Executor executor_type;

    explicit initiate_async_receive_multiple(basic_datagram_socket* self)
      : self_(self)
    {
    }

    const executor_type& get_executor() const noexcept
    {
      return self_->get_executor();
    }

    template <typename ReadMultiHandler, typename MutableBufferSequences,
        typename MutableSizeSequence, typename MutableFlagsSequence>
    void operator()(ReadMultiHandler&& handler,
        MutableBufferSequences&& buffer_sequences,
        MutableSizeSequence&& sizes, MutableFlagsSequence&& out_flags,
        socket_base::message_flags flags) const
    {
      static_assert(
          is_shallow_mutable_sequence<decay_t<MutableSizeSequence>>::value,
          "sizes must be a shallow mutable sequence");
      static_assert(!detail::is_range<ReadMultiHandler>::value,
          "out_flags must be a shallow mutable sequence");

      // If you get an error on the following line it means that your handler
      // does not meet the documented type requirements for a ReadMultiHandler.
      ASIO_READ_MULTI_HANDLER_CHECK(
          ReadMultiHandler, handler) type_check;

      detail::non_const_lvalue<ReadMultiHandler> handler2(handler);
      ASIO_INVOKE_INLINE
      self_->impl_.get_service().async_receive_multiple(
          self_->impl_.get_implementation(),
          static_cast<MutableBufferSequences&&>(buffer_sequences),
          static_cast<MutableSizeSequence&&>(sizes),
          static_cast<MutableFlagsSequence&&>(out_flags), flags,
          handler2.value, self_->impl_.get_executor());
    }

  private:
    basic_datagram_socket* self_;
  };

  class initiate_async_receive_from_multiple
  {
  public:
    typedef Executor executor_type;

    explicit initiate_async_receive_from_multiple(basic_datagram_socket* self)
      : self_(self)
    {
    }

    const executor_type& get_executor() const noexcept
    {
      return self_->get_executor();
    }

    template <typename ReadMultiHandler, typename MutableBufferSequences,
        typename MutableEndpointSequence, typename MutableSizeSequence,
        typename MutableFlagsSequence>
    void operator()(ReadMultiHandler&& handler,
        MutableBufferSequences&& buffer_sequences,
        MutableEndpointSequence&& endpoints, MutableSizeSequence&& sizes,
        MutableFlagsSequence&& out_flags,
        socket_base::message_flags flags) const
    {
      static_assert(
          is_shallow_mutable_sequence<decay_t<MutableEndpointSequence>>::value,
          "endpoints must be a shallow mutable sequence");
      static_assert(
          is_shallow_mutable_sequence<decay_t<MutableSizeSequence>>::value,
          "sizes must be a shallow mutable sequence");
      static_assert(!detail::is_range<ReadMultiHandler>::value,
          "out_flags must be a shallow mutable sequence");

      // If you get an error on the following line it means that your handler
      // does not meet the documented type requirements for a ReadMultiHandler.
      ASIO_READ_MULTI_HANDLER_CHECK(
          ReadMultiHandler, handler) type_check;

      detail::non_const_lvalue<ReadMultiHandler> handler2(handler);
      ASIO_INVOKE_INLINE
      self_->impl_.get_service().async_receive_from_multiple(
          self_->impl_.get_implementation(),
          static_cast<MutableBufferSequences&&>(buffer_sequences),
          static_cast<MutableEndpointSequence&&>(endpoints),
          static_cast<MutableSizeSequence&&>(sizes),
          static_cast<MutableFlagsSequence&&>(out_flags), flags,
          handler2.value, self_->impl_.get_executor());
    }

  private:
    basic_datagram_socket* self_;
  };
};

ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_BASIC_DATAGRAM_SOCKET_HPP
