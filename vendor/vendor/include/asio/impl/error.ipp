//
// impl/error.ipp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2025 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_IMPL_ERROR_IPP
#define ASIO_IMPL_ERROR_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include <string>
#include "asio/error.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace error {
// boostify: non-boost code starts here
namespace detail {

std::error_condition error_number_to_condition(int ev)
{
    switch (ev)
    {
    case access_denied:
      return std::errc::permission_denied;
    case address_family_not_supported:
      return std::errc::address_family_not_supported;
    case address_in_use:
      return std::errc::address_in_use;
    case already_connected:
      return std::errc::already_connected;
    case already_started:
      return std::errc::connection_already_in_progress;
    case broken_pipe:
      return std::errc::broken_pipe;
    case connection_aborted:
      return std::errc::connection_aborted;
    case connection_refused:
      return std::errc::connection_refused;
    case connection_reset:
      return std::errc::connection_reset;
    case bad_descriptor:
      return std::errc::bad_file_descriptor;
    case fault:
      return std::errc::bad_address;
    case host_unreachable:
      return std::errc::host_unreachable;
    case in_progress:
      return std::errc::operation_in_progress;
    case interrupted:
      return std::errc::interrupted;
    case invalid_argument:
      return std::errc::invalid_argument;
    case message_size:
      return std::errc::message_size;
    case name_too_long:
      return std::errc::filename_too_long;
    case network_down:
      return std::errc::network_down;
    case network_reset:
      return std::errc::network_reset;
    case network_unreachable:
      return std::errc::network_unreachable;
    case no_descriptors:
      return std::errc::too_many_files_open;
    case no_buffer_space:
      return std::errc::no_buffer_space;
    case no_memory:
      return std::errc::not_enough_memory;
    case no_permission:
      return std::errc::operation_not_permitted;
    case no_protocol_option:
      return std::errc::no_protocol_option;
    case no_such_device:
      return std::errc::no_such_device;
    case not_connected:
      return std::errc::not_connected;
    case not_socket:
      return std::errc::not_a_socket;
    case operation_aborted:
      return std::errc::operation_canceled;
    case operation_not_supported:
      return std::errc::operation_not_supported;
    case shut_down:
      return std::error_condition(ev, asio::system_category());
    case timed_out:
      return std::errc::timed_out;
    case try_again:
      return std::errc::resource_unavailable_try_again;
    default:
      if (ev == would_block)
        return std::errc::operation_would_block;
      return std::error_condition(ev, asio::system_category());
    }
}

} // namespace detail
// boostify: non-boost code ends here

#if !defined(ASIO_WINDOWS) && !defined(__CYGWIN__)

namespace detail {

class netdb_category : public asio::error_category
{
public:
  const char* name() const noexcept
  {
    return "asio.netdb";
  }

  std::string message(int value) const
  {
    if (value == error::host_not_found)
      return "Host not found (authoritative)";
    if (value == error::host_not_found_try_again)
      return "Host not found (non-authoritative), try again later";
    if (value == error::no_data)
      return "The query is valid, but it does not have associated data";
    if (value == error::no_recovery)
      return "A non-recoverable error occurred during database lookup";
    return "asio.netdb error";
  }
};

} // namespace detail

const asio::error_category& get_netdb_category()
{
  static detail::netdb_category instance;
  return instance;
}

namespace detail {

class addrinfo_category : public asio::error_category
{
public:
  const char* name() const noexcept
  {
    return "asio.addrinfo";
  }

  std::string message(int value) const
  {
    if (value == error::service_not_found)
      return "Service not found";
    if (value == error::socket_type_not_supported)
      return "Socket type not supported";
    return "asio.addrinfo error";
  }
};

} // namespace detail

const asio::error_category& get_addrinfo_category()
{
  static detail::addrinfo_category instance;
  return instance;
}

#endif // !defined(ASIO_WINDOWS) && !defined(__CYGWIN__)

namespace detail {

class misc_category : public asio::error_category
{
public:
  const char* name() const noexcept
  {
    return "asio.misc";
  }

  std::string message(int value) const
  {
    if (value == error::already_open)
      return "Already open";
    if (value == error::eof)
      return "End of file";
    if (value == error::not_found)
      return "Element not found";
    if (value == error::fd_set_failure)
      return "The descriptor does not fit into the select call's fd_set";
    return "asio.misc error";
  }
};

} // namespace detail

const asio::error_category& get_misc_category()
{
  static detail::misc_category instance;
  return instance;
}

} // namespace error
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_IMPL_ERROR_IPP
