//
// detail/stderr_handler_tracking.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_STDERR_HANDLER_TRACKING_HPP
#define ASIO_DETAIL_STDERR_HANDLER_TRACKING_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_ENABLE_HANDLER_TRACKING) \
  && !defined(ASIO_PERFETTO_HANDLER_TRACKING) \
  && !defined(ASIO_CUSTOM_HANDLER_TRACKING)

#include "asio/error_code.hpp"
#include "asio/detail/cstdint.hpp"
#include "asio/detail/static_mutex.hpp"
#include "asio/detail/tss_ptr.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN

class execution_context;

namespace detail {

class stderr_handler_tracking
{
public:
  class completion;

  // Base class for objects containing tracked handlers.
  class tracked_handler
  {
  private:
    // Only the stderr_handler_tracking class will have access to the id.
    friend class stderr_handler_tracking;
    friend class completion;
    uint64_t id_;

  protected:
    // Constructor initialises with no id.
    tracked_handler() : id_(0) {}

    // Prevent deletion through this type.
    ~tracked_handler() {}
  };

  // Initialise the tracking system.
  ASIO_DECL static void init();

  class location
  {
  public:
    // Constructor adds a location to the stack.
    ASIO_DECL explicit location(const char* file,
        int line, const char* func);

    // Destructor removes a location from the stack.
    ASIO_DECL ~location();

  private:
    // Disallow copying and assignment.
    location(const location&) = delete;
    location& operator=(const location&) = delete;

    friend class stderr_handler_tracking;
    const char* file_;
    int line_;
    const char* func_;
    location* next_;
  };

  // Record the creation of a tracked handler.
  ASIO_DECL static void creation(tracked_handler& h,
      const char* object_type, void* object,
      uintmax_t native_handle, const char* op_name);

  // Record the creation of a tracked handler.
  ASIO_DECL static void creation(
      execution_context& context, tracked_handler& h,
      const char* object_type, void* object,
      uintmax_t native_handle, const char* op_name);

  class completion
  {
  public:
    // Constructor records that handler is to be invoked with no arguments.
    ASIO_DECL explicit completion(const tracked_handler& h);

    // Destructor records only when an exception is thrown from the handler, or
    // if the memory is being freed without the handler having been invoked.
    ASIO_DECL ~completion();

    // Records that handler is to be invoked with no arguments.
    ASIO_DECL void invocation_begin();

    // Records that handler is to be invoked with one argument.
    ASIO_DECL void invocation_begin(const asio::error_code& ec);

    // Constructor records that handler is to be invoked with two arguments.
    ASIO_DECL void invocation_begin(
        const asio::error_code& ec, std::size_t bytes_transferred);

    // Constructor records that handler is to be invoked with two arguments.
    ASIO_DECL void invocation_begin(
        const asio::error_code& ec, int signal_number);

    // Constructor records that handler is to be invoked with two arguments.
    ASIO_DECL void invocation_begin(
        const asio::error_code& ec, const char* arg);

    // Record that handler invocation has ended.
    ASIO_DECL void invocation_end();

  private:
    friend class stderr_handler_tracking;
    uint64_t id_;
    bool invoked_;
    completion* next_;
  };

  // Record an operation that is not directly associated with a handler.
  ASIO_DECL static void operation(execution_context& context,
      const char* object_type, void* object,
      uintmax_t native_handle, const char* op_name);

  // Record that a descriptor has been registered with the reactor.
  ASIO_DECL static void reactor_registration(execution_context& context,
      uintmax_t native_handle, uintmax_t registration);

  // Record that a descriptor has been deregistered from the reactor.
  ASIO_DECL static void reactor_deregistration(execution_context& context,
      uintmax_t native_handle, uintmax_t registration);

  // Record a reactor-based operation that is associated with a handler.
  ASIO_DECL static void reactor_events(execution_context& context,
      uintmax_t registration, unsigned events);

  // Record a reactor-based operation that is associated with a handler.
  ASIO_DECL static void reactor_operation(
      const tracked_handler& h, const char* op_name,
      const asio::error_code& ec);

  // Record a reactor-based operation that is associated with a handler.
  ASIO_DECL static void reactor_operation(
      const tracked_handler& h, const char* op_name,
      const asio::error_code& ec, std::size_t bytes_transferred);

  // Write a line of output.
  ASIO_DECL static void write_line(const char* format, ...);

private:
  struct tracking_state;
  ASIO_DECL static tracking_state* get_state();
};

# define ASIO_INHERIT_TRACKED_HANDLER \
  : public asio::detail::stderr_handler_tracking::tracked_handler

# define ASIO_ALSO_INHERIT_TRACKED_HANDLER \
  , public asio::detail::stderr_handler_tracking::tracked_handler

# define ASIO_HANDLER_TRACKING_INIT \
  asio::detail::stderr_handler_tracking::init()

# define ASIO_HANDLER_LOCATION(args) \
  asio::detail::stderr_handler_tracking::location tracked_location args

# define ASIO_HANDLER_CREATION(args) \
  asio::detail::stderr_handler_tracking::creation args

# define ASIO_HANDLER_COMPLETION(args) \
  asio::detail::stderr_handler_tracking::completion \
    tracked_completion args

# define ASIO_HANDLER_INVOCATION_BEGIN(args) \
  tracked_completion.invocation_begin args

# define ASIO_HANDLER_INVOCATION_END \
  tracked_completion.invocation_end()

# define ASIO_HANDLER_OPERATION(args) \
  asio::detail::stderr_handler_tracking::operation args

# define ASIO_HANDLER_REACTOR_REGISTRATION(args) \
  asio::detail::stderr_handler_tracking::reactor_registration args

# define ASIO_HANDLER_REACTOR_DEREGISTRATION(args) \
  asio::detail::stderr_handler_tracking::reactor_deregistration args

# define ASIO_HANDLER_REACTOR_READ_EVENT 1
# define ASIO_HANDLER_REACTOR_WRITE_EVENT 2
# define ASIO_HANDLER_REACTOR_ERROR_EVENT 4

# define ASIO_HANDLER_REACTOR_EVENTS(args) \
  asio::detail::stderr_handler_tracking::reactor_events args

# define ASIO_HANDLER_REACTOR_OPERATION(args) \
  asio::detail::stderr_handler_tracking::reactor_operation args

# define ASIO_HANDLER_SCHEDULER_RUN(args) (void)0

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // defined(ASIO_ENABLE_HANDLER_TRACKING)
       //   && !defined(ASIO_PERFETTO_HANDLER_TRACKING)
       //   && !defined(ASIO_CUSTOM_HANDLER_TRACKING)

#if defined(ASIO_HEADER_ONLY)
# include "asio/detail/impl/stderr_handler_tracking.ipp"
#endif // defined(ASIO_HEADER_ONLY)

#endif // ASIO_DETAIL_STDERR_HANDLER_TRACKING_HPP
