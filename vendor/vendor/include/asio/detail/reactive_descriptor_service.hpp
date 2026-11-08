//
// detail/reactive_descriptor_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_REACTIVE_DESCRIPTOR_SERVICE_HPP
#define ASIO_DETAIL_REACTIVE_DESCRIPTOR_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/io_control.hpp"

#if !defined(ASIO_WINDOWS) \
  && !defined(ASIO_WINDOWS_RUNTIME) \
  && !defined(ASIO_CYGWIN_W32_SOCKETS) \
  && !defined(ASIO_HAS_IO_URING_AS_DEFAULT)

#include "asio/associated_cancellation_slot.hpp"
#include "asio/associated_immediate_executor.hpp"
#include "asio/buffer.hpp"
#include "asio/cancellation_type.hpp"
#include "asio/execution_context.hpp"
#include "asio/detail/bind_handler.hpp"
#include "asio/detail/buffer_sequence_adapter.hpp"
#include "asio/detail/descriptor_ops.hpp"
#include "asio/detail/descriptor_read_op.hpp"
#include "asio/detail/descriptor_write_op.hpp"
#include "asio/detail/fenced_block.hpp"
#include "asio/detail/memory.hpp"
#include "asio/detail/noncopyable.hpp"
#include "asio/detail/reactive_null_buffers_op.hpp"
#include "asio/detail/reactive_wait_op.hpp"
#include "asio/detail/reactor.hpp"
#include "asio/posix/descriptor_base.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

class reactive_descriptor_service :
  public execution_context_service_base<reactive_descriptor_service>
{
public:
  // The native type of a descriptor.
  typedef int native_handle_type;

  // The implementation type of the descriptor.
  class implementation_type
    : private asio::detail::noncopyable
  {
  public:
    // Default constructor.
    implementation_type()
      : descriptor_(-1),
        state_(0)
    {
    }

  private:
    // Only this service will have access to the internal values.
    friend class reactive_descriptor_service;

    // The native descriptor representation.
    int descriptor_;

    // The current state of the descriptor.
    descriptor_ops::state_type state_;

    // Per-descriptor data used by the reactor.
    reactor::per_descriptor_data reactor_data_;
  };

  // Constructor.
  ASIO_DECL reactive_descriptor_service(execution_context& context);

  // Destroy all user-defined handler objects owned by the service.
  ASIO_DECL void shutdown();

  // Construct a new descriptor implementation.
  ASIO_DECL void construct(implementation_type& impl);

  // Move-construct a new descriptor implementation.
  ASIO_DECL void move_construct(implementation_type& impl,
      implementation_type& other_impl) noexcept;

  // Move-assign from another descriptor implementation.
  ASIO_DECL void move_assign(implementation_type& impl,
      reactive_descriptor_service& other_service,
      implementation_type& other_impl);

  // Destroy a descriptor implementation.
  ASIO_DECL void destroy(implementation_type& impl);

  // Assign a native descriptor to a descriptor implementation.
  ASIO_DECL asio::error_code assign(implementation_type& impl,
      const native_handle_type& native_descriptor,
      asio::error_code& ec);

  // Determine whether the descriptor is open.
  bool is_open(const implementation_type& impl) const
  {
    return impl.descriptor_ != -1;
  }

  // Destroy a descriptor implementation.
  ASIO_DECL asio::error_code close(implementation_type& impl,
      asio::error_code& ec);

  // Get the native descriptor representation.
  native_handle_type native_handle(const implementation_type& impl) const
  {
    return impl.descriptor_;
  }

  // Release ownership of the native descriptor representation.
  ASIO_DECL native_handle_type release(implementation_type& impl);

  // Release ownership of the native descriptor representation.
  native_handle_type release(implementation_type& impl,
      asio::error_code& ec)
  {
    ec = success_ec_;
    return release(impl);
  }

  // Cancel all operations associated with the descriptor.
  ASIO_DECL asio::error_code cancel(implementation_type& impl,
      asio::error_code& ec);

  // Perform an IO control command on the descriptor.
  template <typename IO_Control_Command>
  asio::error_code io_control(implementation_type& impl,
      IO_Control_Command& command, asio::error_code& ec)
  {
    const char* descriptor_name = 0;
    if (get_descriptor_name(command, descriptor_name, ec))
    {
      if (!ec)
        reactor_.set_descriptor_name(impl.reactor_data_, descriptor_name, ec);
      ASIO_ERROR_LOCATION(ec);
      return ec;
    }

    descriptor_ops::ioctl(impl.descriptor_, impl.state_,
        command.name(), static_cast<ioctl_arg_type*>(command.data()), ec);
    ASIO_ERROR_LOCATION(ec);
    return ec;
  }

  // Gets the non-blocking mode of the descriptor.
  bool non_blocking(const implementation_type& impl) const
  {
    return (impl.state_ & descriptor_ops::user_set_non_blocking) != 0;
  }

  // Sets the non-blocking mode of the descriptor.
  asio::error_code non_blocking(implementation_type& impl,
      bool mode, asio::error_code& ec)
  {
    descriptor_ops::set_user_non_blocking(
        impl.descriptor_, impl.state_, mode, ec);
    ASIO_ERROR_LOCATION(ec);
    return ec;
  }

  // Gets the non-blocking mode of the native descriptor implementation.
  bool native_non_blocking(const implementation_type& impl) const
  {
    return (impl.state_ & descriptor_ops::internal_non_blocking) != 0;
  }

  // Sets the non-blocking mode of the native descriptor implementation.
  asio::error_code native_non_blocking(implementation_type& impl,
      bool mode, asio::error_code& ec)
  {
    descriptor_ops::set_internal_non_blocking(
        impl.descriptor_, impl.state_, mode, ec);
    return ec;
  }

  // Wait for the descriptor to become ready to read, ready to write, or to have
  // pending error conditions.
  asio::error_code wait(implementation_type& impl,
      posix::descriptor_base::wait_type w, asio::error_code& ec)
  {
    switch (w)
    {
    case posix::descriptor_base::wait_read:
      descriptor_ops::poll_read(impl.descriptor_, impl.state_, ec);
      break;
    case posix::descriptor_base::wait_write:
      descriptor_ops::poll_write(impl.descriptor_, impl.state_, ec);
      break;
    case posix::descriptor_base::wait_error:
      descriptor_ops::poll_error(impl.descriptor_, impl.state_, ec);
      break;
    default:
      ec = asio::error::invalid_argument;
      break;
    }

    ASIO_ERROR_LOCATION(ec);
    return ec;
  }

  // Asynchronously wait for the descriptor to become ready to read, ready to
  // write, or to have pending error conditions.
  template <typename Handler, typename IoExecutor>
  void async_wait(implementation_type& impl,
      posix::descriptor_base::wait_type w,
      Handler& handler, const IoExecutor& io_ex);

  // Write some data to the descriptor.
  template <typename ConstBufferSequence>
  size_t write_some(implementation_type& impl,
      const ConstBufferSequence& buffers, asio::error_code& ec)
  {
    typedef buffer_sequence_adapter<asio::const_buffer,
        ConstBufferSequence> bufs_type;

    size_t n;
    if (bufs_type::is_single_buffer)
    {
      n = descriptor_ops::sync_write1(impl.descriptor_,
          impl.state_, bufs_type::first(buffers).data(),
          bufs_type::first(buffers).size(), ec);
    }
    else
    {
      bufs_type bufs(buffers);

      n = descriptor_ops::sync_write(impl.descriptor_, impl.state_,
          bufs.buffers(), bufs.count(), bufs.all_empty(), ec);
    }

    ASIO_ERROR_LOCATION(ec);
    return n;
  }

  // Wait until data can be written without blocking.
  size_t write_some(implementation_type& impl,
      const null_buffers&, asio::error_code& ec)
  {
    // Wait for descriptor to become ready.
    descriptor_ops::poll_write(impl.descriptor_, impl.state_, ec);
    ASIO_ERROR_LOCATION(ec);
    return 0;
  }

  // Start an asynchronous write. The data being sent must be valid for the
  // lifetime of the asynchronous operation.
  template <typename ConstBufferSequence, typename Handler, typename IoExecutor>
  void async_write_some(implementation_type& impl,
      const ConstBufferSequence& buffers, Handler& handler,
      const IoExecutor& io_ex);

  // Start an asynchronous wait until data can be written without blocking.
  template <typename Handler, typename IoExecutor>
  void async_write_some(implementation_type& impl,
      const null_buffers&, Handler& handler, const IoExecutor& io_ex);

  // Read some data from the stream. Returns the number of bytes read.
  template <typename MutableBufferSequence>
  size_t read_some(implementation_type& impl,
      const MutableBufferSequence& buffers, asio::error_code& ec)
  {
    typedef buffer_sequence_adapter<asio::mutable_buffer,
        MutableBufferSequence> bufs_type;

    size_t n;
    if (bufs_type::is_single_buffer)
    {
      n = descriptor_ops::sync_read1(impl.descriptor_,
          impl.state_, bufs_type::first(buffers).data(),
          bufs_type::first(buffers).size(), ec);
    }
    else
    {
      bufs_type bufs(buffers);

      n = descriptor_ops::sync_read(impl.descriptor_, impl.state_,
          bufs.buffers(), bufs.count(), bufs.all_empty(), ec);
    }

    ASIO_ERROR_LOCATION(ec);
    return n;
  }

  // Wait until data can be read without blocking.
  size_t read_some(implementation_type& impl,
      const null_buffers&, asio::error_code& ec)
  {
    // Wait for descriptor to become ready.
    descriptor_ops::poll_read(impl.descriptor_, impl.state_, ec);
    ASIO_ERROR_LOCATION(ec);
    return 0;
  }

  // Start an asynchronous read. The buffer for the data being read must be
  // valid for the lifetime of the asynchronous operation.
  template <typename MutableBufferSequence,
      typename Handler, typename IoExecutor>
  void async_read_some(implementation_type& impl,
      const MutableBufferSequence& buffers,
      Handler& handler, const IoExecutor& io_ex);

  // Wait until data can be read without blocking.
  template <typename Handler, typename IoExecutor>
  void async_read_some(implementation_type& impl,
      const null_buffers&, Handler& handler, const IoExecutor& io_ex);

private:
  // Start the asynchronous operation. Returns true if the operation is ready
  // for its immediate completion.
  template <reactor_op::perform_func_type Perform, int OpType>
  bool do_start_op(implementation_type& impl,
      reactor_op* op, bool noop, bool needs_non_blocking)
  {
    if (!noop)
    {
      if ((impl.state_ & descriptor_ops::non_blocking)
          || !needs_non_blocking
          || descriptor_ops::set_internal_non_blocking(
            impl.descriptor_, impl.state_, true, op->ec_))
      {
        return reactor_.start_op<OpType, Perform>(
            impl.descriptor_, impl.reactor_data_, op);
      }
    }

    return true;
  }

  // Start the asynchronous operation for handlers that are specialised for
  // immediate completion.
  template <reactor_op::perform_func_type Perform, int OpType, typename Op>
  void start_op(implementation_type& impl, Op* op, bool is_continuation,
      bool noop, bool needs_non_blocking, const void* io_ex, ...)
  {
    if (do_start_op<Perform, OpType>(impl, op, noop, needs_non_blocking))
      Op::do_immediate(op, is_continuation, io_ex);
  }

  // Start the asynchronous operation for handlers that are not specialised for
  // immediate completion.
  template <reactor_op::perform_func_type Perform, int OpType, typename Op>
  void start_op(implementation_type& impl, Op* op, bool is_continuation,
      bool noop, bool needs_non_blocking, const void*,
      enable_if_t<
        !has_associated_immediate_executor<
          typename Op::handler_type,
          typename Op::io_executor_type
        >::value
      >*)
  {
    if (do_start_op<Perform, OpType>(impl, op, noop, needs_non_blocking))
      reactor::call_post_immediate_completion(op, is_continuation, &reactor_);
  }

  // Helper class used to implement per-operation cancellation
  class reactor_op_cancellation
  {
  public:
    reactor_op_cancellation(reactor* r,
        reactor::per_descriptor_data* p, int d, int o)
      : reactor_(r),
        reactor_data_(p),
        descriptor_(d),
        op_type_(o)
    {
    }

    void operator()(cancellation_type_t type)
    {
      if (!!(type &
            (cancellation_type::terminal
              | cancellation_type::partial
              | cancellation_type::total)))
      {
        reactor_->cancel_ops_by_key(descriptor_,
            *reactor_data_, op_type_, this);
      }
    }

  private:
    reactor* reactor_;
    reactor::per_descriptor_data* reactor_data_;
    int descriptor_;
    int op_type_;
  };

  // The selector that performs event demultiplexing for the service.
  reactor& reactor_;

  // Cached success value to avoid accessing category singleton.
  const asio::error_code success_ec_;
};

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#include "asio/detail/impl/reactive_descriptor_service.hpp"

#if defined(ASIO_HEADER_ONLY)
# include "asio/detail/impl/reactive_descriptor_service.ipp"
#endif // defined(ASIO_HEADER_ONLY)

#endif // !defined(ASIO_WINDOWS)
       //   && !defined(ASIO_WINDOWS_RUNTIME)
       //   && !defined(ASIO_CYGWIN_W32_SOCKETS)
       //   && !defined(ASIO_HAS_IO_URING_AS_DEFAULT)

#endif // ASIO_DETAIL_REACTIVE_DESCRIPTOR_SERVICE_HPP
