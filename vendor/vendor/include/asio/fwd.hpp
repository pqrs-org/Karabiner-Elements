//
// fwd.hpp
// ~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_FWD_HPP
#define ASIO_FWD_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/chrono.hpp"
#include "asio/basic_streambuf_fwd.hpp"
#include "asio/buffered_read_stream_fwd.hpp"
#include "asio/buffered_stream_fwd.hpp"
#include "asio/buffered_write_stream_fwd.hpp"

#include <memory>

#if !defined(GENERATING_DOCUMENTATION)

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN

class execution_context;

template <typename Clock>
struct wait_traits;

#if !defined(ASIO_EXECUTOR_WORK_GUARD_DECL)
#define ASIO_EXECUTOR_WORK_GUARD_DECL

template <typename Executor, typename = void, typename = void>
class executor_work_guard;

#endif // !defined(ASIO_EXECUTOR_WORK_GUARD_DECL)

template <typename Blocking, typename Relationship, typename Allocator>
class basic_system_executor;

template <typename InlineExceptionHandling>
class basic_inline_executor;

#if !defined(ASIO_NO_TS_EXECUTORS)

class executor;

#endif // !defined(ASIO_NO_TS_EXECUTORS)

#if defined(ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

typedef executor any_io_executor;

typedef executor any_completion_executor;

#else // defined(ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

namespace execution {

#if !defined(ASIO_EXECUTION_ANY_EXECUTOR_FWD_DECL)
#define ASIO_EXECUTION_ANY_EXECUTOR_FWD_DECL

template <typename... SupportableProperties>
class any_executor;

#endif // !defined(ASIO_EXECUTION_ANY_EXECUTOR_FWD_DECL)

} // namespace execution

class any_io_executor;

class any_completion_executor;

#endif // defined(ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

template <typename... Signatures>
class any_completion_handler;

template <typename Executor>
class strand;

class io_context;

class system_context;

class thread_pool;

class thread;

#if !defined(ASIO_BASIC_SOCKET_FWD_DECL)
#define ASIO_BASIC_SOCKET_FWD_DECL

template <typename Protocol, typename Executor = any_io_executor>
class basic_socket;

#endif // !defined(ASIO_BASIC_SOCKET_FWD_DECL)

#if !defined(ASIO_BASIC_DATAGRAM_SOCKET_FWD_DECL)
#define ASIO_BASIC_DATAGRAM_SOCKET_FWD_DECL

template <typename Protocol, typename Executor = any_io_executor>
class basic_datagram_socket;

#endif // !defined(ASIO_BASIC_DATAGRAM_SOCKET_FWD_DECL)

#if !defined(ASIO_BASIC_STREAM_SOCKET_FWD_DECL)
#define ASIO_BASIC_STREAM_SOCKET_FWD_DECL

// Forward declaration with defaulted arguments.
template <typename Protocol, typename Executor = any_io_executor>
class basic_stream_socket;

#endif // !defined(ASIO_BASIC_STREAM_SOCKET_FWD_DECL)

#if !defined(ASIO_BASIC_SOCKET_ACCEPTOR_FWD_DECL)
#define ASIO_BASIC_SOCKET_ACCEPTOR_FWD_DECL

template <typename Protocol, typename Executor = any_io_executor>
class basic_socket_acceptor;

#endif // !defined(ASIO_BASIC_SOCKET_ACCEPTOR_FWD_DECL)

#if !defined(ASIO_BASIC_RAW_SOCKET_FWD_DECL)
#define ASIO_BASIC_RAW_SOCKET_FWD_DECL

template <typename Protocol, typename Executor = any_io_executor>
class basic_raw_socket;

#endif // !defined(ASIO_BASIC_RAW_SOCKET_FWD_DECL)

#if !defined(ASIO_BASIC_SEQ_PACKET_SOCKET_FWD_DECL)
#define ASIO_BASIC_SEQ_PACKET_SOCKET_FWD_DECL

template <typename Protocol, typename Executor = any_io_executor>
class basic_seq_packet_socket;

#endif // !defined(ASIO_BASIC_SEQ_PACKET_SOCKET_FWD_DECL)

#if !defined(ASIO_BASIC_SOCKET_STREAMBUF_FWD_DECL)
#define ASIO_BASIC_SOCKET_STREAMBUF_FWD_DECL

// Forward declaration with defaulted arguments.
template <typename Protocol,
    typename Clock = chrono::steady_clock,
    typename WaitTraits = wait_traits<Clock>>
class basic_socket_streambuf;

#endif // !defined(ASIO_BASIC_SOCKET_STREAMBUF_FWD_DECL)

#if !defined(ASIO_BASIC_SOCKET_IOSTREAM_FWD_DECL)
#define ASIO_BASIC_SOCKET_IOSTREAM_FWD_DECL

// Forward declaration with defaulted arguments.
template <typename Protocol,
    typename Clock = chrono::steady_clock,
    typename WaitTraits = wait_traits<Clock>>
class basic_socket_iostream;

#endif // !defined(ASIO_BASIC_SOCKET_IOSTREAM_FWD_DECL)

#if !defined(ASIO_BASIC_FILE_FWD_DECL)
#define ASIO_BASIC_FILE_FWD_DECL

template <typename Executor = any_io_executor>
class basic_file;

#endif // !defined(ASIO_BASIC_FILE_FWD_DECL)

#if !defined(ASIO_BASIC_RANDOM_ACCESS_FILE_FWD_DECL)
#define ASIO_BASIC_RANDOM_ACCESS_FILE_FWD_DECL

template <typename Executor = any_io_executor>
class basic_random_access_file;

#endif // !defined(ASIO_BASIC_RANDOM_ACCESS_FILE_FWD_DECL)

typedef basic_random_access_file<> random_access_file;

#if !defined(ASIO_BASIC_STREAM_FILE_FWD_DECL)
#define ASIO_BASIC_STREAM_FILE_FWD_DECL

template <typename Executor = any_io_executor>
class basic_stream_file;

#endif // !defined(ASIO_BASIC_STREAM_FILE_FWD_DECL)

typedef basic_stream_file<> stream_file;

#if !defined(ASIO_BASIC_READABLE_PIPE_FWD_DECL)
#define ASIO_BASIC_READABLE_PIPE_FWD_DECL

template <typename Executor = any_io_executor>
class basic_readable_pipe;

#endif // !defined(ASIO_BASIC_READABLE_PIPE_FWD_DECL)

typedef basic_readable_pipe<> readable_pipe;

#if !defined(ASIO_BASIC_WRITABLE_PIPE_FWD_DECL)
#define ASIO_BASIC_WRITABLE_PIPE_FWD_DECL

template <typename Executor = any_io_executor>
class basic_writable_pipe;

#endif // !defined(ASIO_BASIC_WRITABLE_PIPE_FWD_DECL)

typedef basic_writable_pipe<> writable_pipe;

#if !defined(ASIO_BASIC_SERIAL_PORT_FWD_DECL)
#define ASIO_BASIC_SERIAL_PORT_FWD_DECL

template <typename Executor = any_io_executor>
class basic_serial_port;

#endif // !defined(ASIO_BASIC_SERIAL_PORT_FWD_DECL)

typedef basic_serial_port<> serial_port;

#if !defined(ASIO_BASIC_SIGNAL_SET_FWD_DECL)
#define ASIO_BASIC_SIGNAL_SET_FWD_DECL

template <typename Executor = any_io_executor>
class basic_signal_set;

#endif // !defined(ASIO_BASIC_SIGNAL_SET_FWD_DECL)

typedef basic_signal_set<> signal_set;

#if !defined(ASIO_BASIC_WAITABLE_TIMER_FWD_DECL)
#define ASIO_BASIC_WAITABLE_TIMER_FWD_DECL

// Forward declaration with defaulted arguments.
template <typename Clock,
    typename WaitTraits = asio::wait_traits<Clock>,
    typename Executor = any_io_executor>
class basic_waitable_timer;

#endif // !defined(ASIO_BASIC_WAITABLE_TIMER_FWD_DECL)

typedef basic_waitable_timer<chrono::system_clock> system_timer;

typedef basic_waitable_timer<chrono::steady_clock> steady_timer;

typedef basic_waitable_timer<chrono::high_resolution_clock>
  high_resolution_timer;

typedef basic_streambuf<> streambuf;

class mutable_buffer;

class const_buffer;

class registered_buffer_id;

class mutable_registered_buffer;

class const_registered_buffer;

template <typename Elem, typename Traits, typename Allocator>
class dynamic_string_buffer;

template <typename Elem, typename Allocator>
class dynamic_vector_buffer;

#if !defined(ASIO_BUFFER_REGISTRATION_FWD_DECL)
#define ASIO_BUFFER_REGISTRATION_FWD_DECL

// Forward declaration with defaulted arguments.
template <typename MutableBufferSequence,
    typename Allocator = std::allocator<void>>
class buffer_registration;

#endif // !defined(ASIO_BUFFER_REGISTRATION_FWD_DECL)

#if !defined(ASIO_BUFFERS_ITERATOR_FWD_DECL)
#define ASIO_BUFFERS_ITERATOR_FWD_DECL

// Forward declaration with defaulted arguments.
template <typename BufferSequence, typename ByteType = char>
class buffers_iterator;

#endif // !defined(ASIO_BUFFERS_ITERATOR_FWD_DECL)

class cancellation_signal;

class cancellation_slot;

class cancellation_state;

class coroutine;

template <typename T>
class recycling_allocator;

namespace ip {

class address;

class address_v4;

class address_v6;

template <typename Address>
class basic_address_iterator;

typedef basic_address_iterator<address_v4> address_v4_iterator;

typedef basic_address_iterator<address_v6> address_v6_iterator;

template <typename Address>
class basic_address_range;

typedef basic_address_range<address_v4> address_v4_range;

typedef basic_address_range<address_v6> address_v6_range;

class network_v4;

class network_v6;

template <typename InternetProtocol>
class basic_endpoint;

template <typename InternetProtocol>
class basic_resolver_entry;

template <typename InternetProtocol>
class basic_resolver_iterator;

template <typename InternetProtocol>
class basic_resolver_query;

template <typename InternetProtocol>
class basic_resolver_results;

#if !defined(ASIO_IP_BASIC_RESOLVER_FWD_DECL)
#define ASIO_IP_BASIC_RESOLVER_FWD_DECL

template <typename InternetProtocol, typename Executor = any_io_executor>
class basic_resolver;

#endif // !defined(ASIO_IP_BASIC_RESOLVER_FWD_DECL)

class tcp;

class udp;

class icmp;

} // namespace ip

namespace local {

template <typename Protocol>
class basic_endpoint;

class stream_protocol;

class datagram_protocol;

class seq_packet_protocol;

} // namespace local

namespace generic {

template <typename Protocol>
class basic_endpoint;

class stream_protocol;

class datagram_protocol;

class raw_protocol;

class seq_packet_protocol;

} // namespace generic

namespace posix {

#if !defined(ASIO_POSIX_BASIC_DESCRIPTOR_FWD_DECL)
#define ASIO_POSIX_BASIC_DESCRIPTOR_FWD_DECL

template <typename Executor = any_io_executor>
class basic_descriptor;

#endif // !defined(ASIO_POSIX_BASIC_DESCRIPTOR_FWD_DECL)

typedef basic_descriptor<> descriptor;

#if !defined(ASIO_POSIX_BASIC_STREAM_DESCRIPTOR_FWD_DECL)
#define ASIO_POSIX_BASIC_STREAM_DESCRIPTOR_FWD_DECL

template <typename Executor = any_io_executor>
class basic_stream_descriptor;

#endif // !defined(ASIO_POSIX_BASIC_STREAM_DESCRIPTOR_FWD_DECL)

typedef basic_stream_descriptor<> stream_descriptor;

} // namespace posix

namespace windows {

#if !defined(ASIO_WINDOWS_BASIC_OBJECT_HANDLE_FWD_DECL)
#define ASIO_WINDOWS_BASIC_OBJECT_HANDLE_FWD_DECL

template <typename Executor = any_io_executor>
class basic_object_handle;

#endif // !defined(ASIO_WINDOWS_BASIC_OBJECT_HANDLE_FWD_DECL)

typedef basic_object_handle<> object_handle;

#if !defined(ASIO_WINDOWS_BASIC_OVERLAPPED_HANDLE_FWD_DECL)
#define ASIO_WINDOWS_BASIC_OVERLAPPED_HANDLE_FWD_DECL

template <typename Executor = any_io_executor>
class basic_overlapped_handle;

#endif // !defined(ASIO_WINDOWS_BASIC_OVERLAPPED_HANDLE_FWD_DECL)

typedef basic_overlapped_handle<> overlapped_handle;

#if !defined(ASIO_WINDOWS_BASIC_RANDOM_ACCESS_HANDLE_FWD_DECL)
#define ASIO_WINDOWS_BASIC_RANDOM_ACCESS_HANDLE_FWD_DECL

template <typename Executor = any_io_executor>
class basic_random_access_handle;

#endif // !defined(ASIO_WINDOWS_BASIC_RANDOM_ACCESS_HANDLE_FWD_DECL)

typedef basic_random_access_handle<> random_access_handle;

#if !defined(ASIO_WINDOWS_BASIC_STREAM_HANDLE_FWD_DECL)
#define ASIO_WINDOWS_BASIC_STREAM_HANDLE_FWD_DECL

template <typename Executor = any_io_executor>
class basic_stream_handle;

#endif // !defined(ASIO_WINDOWS_BASIC_STREAM_HANDLE_FWD_DECL)

typedef basic_stream_handle<> stream_handle;

class overlapped_ptr;

} // namespace windows

namespace ssl {

class context;

template <typename Stream>
class stream;

} // namespace ssl

ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // !defined(GENERATING_DOCUMENTATION)

#endif // ASIO_FWD_HPP
