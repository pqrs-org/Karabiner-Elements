//
// detail/io_control.hpp
// ~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_IO_CONTROL_HPP
#define ASIO_DETAIL_IO_CONTROL_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include <cstddef>
#include <cstring>
#include "asio/detail/socket_types.hpp"
#include "asio/error.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {
namespace io_control {

// I/O control command for getting number of bytes available.
class bytes_readable
{
public:
  // Default constructor.
  bytes_readable()
    : value_(0)
  {
  }

  // Construct with a specific command value.
  bytes_readable(std::size_t value)
    : value_(static_cast<detail::ioctl_arg_type>(value))
  {
  }

  // Get the name of the IO control command.
  int name() const
  {
    return static_cast<int>(ASIO_OS_DEF(FIONREAD));
  }

  // Set the value of the I/O control command.
  void set(std::size_t value)
  {
    value_ = static_cast<detail::ioctl_arg_type>(value);
  }

  // Get the current value of the I/O control command.
  std::size_t get() const
  {
    return static_cast<std::size_t>(value_);
  }

  // Get the address of the command data.
  detail::ioctl_arg_type* data()
  {
    return &value_;
  }

  // Get the address of the command data.
  const detail::ioctl_arg_type* data() const
  {
    return &value_;
  }

private:
  detail::ioctl_arg_type value_;
};

// I/O control command to give a descriptor a name. Used by reactors to look up
// per-descriptor configuration.
class descriptor_name
{
public:
  // Default constructor.
  descriptor_name()
    : value_(0)
  {
  }

  // Construct with a specific name. The name is not copied, so it must outlive
  // the call that applies this command.
  explicit descriptor_name(const char* name)
    : value_(name)
  {
  }

  // Get the name of the I/O control command.
  int name() const
  {
    return custom_io_control_name;
  }

  // Set the descriptor's name.
  void set(const char* name)
  {
    value_ = name;
  }

  // Get the descriptor's name.
  const char* get() const
  {
    return value_;
  }

  // Get the address of the command data. The services read it back as a
  // const char* const*.
  void* data()
  {
    return &value_;
  }

  // Get the address of the command data.
  const void* data() const
  {
    return &value_;
  }

private:
  const char* value_;
};

} // namespace io_control

// The longest descriptor name that will be accepted.
enum { max_descriptor_name_len = 64 };

// Recognise and validate the I/O control command that names a descriptor.
template <typename IO_Control_Command>
bool get_descriptor_name(IO_Control_Command& command,
    const char*& name, asio::error_code& ec)
{
  if (command.name() != custom_io_control_name)
    return false;

  name = *static_cast<const char* const*>(
      static_cast<const void*>(command.data()));

  if (!name || !*name || std::strlen(name) > max_descriptor_name_len)
  {
    name = 0;
    ec = asio::error::invalid_argument;
    return true;
  }

  asio::error::clear(ec);
  return true;
}

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_IO_CONTROL_HPP
