//
// basic_io_object.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_BASIC_IO_OBJECT_HPP
#define ASIO_BASIC_IO_OBJECT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/io_service.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {

#if defined(ASIO_HAS_MOVE)
namespace detail
{
  // Type trait used to determine whether a service supports move.
  template <typename IoObjectService>
  class service_has_move
  {
  private:
    typedef IoObjectService service_type;
    typedef typename service_type::implementation_type implementation_type;

    template <typename T, typename U>
    static auto eval(T* t, U* u) -> decltype(t->move_construct(*u, *u), char());
    static char (&eval(...))[2];

  public:
    static const bool value =
      sizeof(service_has_move::eval(
        static_cast<service_type*>(0),
        static_cast<implementation_type*>(0))) == 1;
  };
}
#endif // defined(ASIO_HAS_MOVE)

/// Base class for all I/O objects.
/**
 * @note All I/O objects are non-copyable. However, when using C++0x, certain
 * I/O objects do support move construction and move assignment.
 */
#if !defined(ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)
template <typename IoObjectService>
#else
template <typename IoObjectService,
    bool Movable = detail::service_has_move<IoObjectService>::value>
#endif
class basic_io_object
{
public:
  /// The type of the service that will be used to provide I/O operations.
  typedef IoObjectService service_type;

  /// The underlying implementation type of I/O object.
  typedef typename service_type::implementation_type implementation_type;

  /// Get the io_service associated with the object.
  /**
   * This function may be used to obtain the io_service object that the I/O
   * object uses to dispatch handlers for asynchronous operations.
   *
   * @return A reference to the io_service object that the I/O object will use
   * to dispatch handlers. Ownership is not transferred to the caller.
   */
  asio::io_service& get_io_service()
  {
    return service.get_io_service();
  }

protected:
  /// Construct a basic_io_object.
  /**
   * Performs:
   * @code get_service().construct(get_implementation()); @endcode
   */
  explicit basic_io_object(asio::io_service& io_service)
    : service(asio::use_service<IoObjectService>(io_service))
  {
    service.construct(implementation);
  }

#if defined(GENERATING_DOCUMENTATION)
  /// Move-construct a basic_io_object.
  /**
   * Performs:
   * @code get_service().move_construct(
   *     get_implementation(), other.get_implementation()); @endcode
   *
   * @note Available only for services that support movability,
   */
  basic_io_object(basic_io_object&& other);

  /// Move-assign a basic_io_object.
  /**
   * Performs:
   * @code get_service().move_assign(get_implementation(),
   *     other.get_service(), other.get_implementation()); @endcode
   *
   * @note Available only for services that support movability,
   */
  basic_io_object& operator=(basic_io_object&& other);
#endif // defined(GENERATING_DOCUMENTATION)

  /// Protected destructor to prevent deletion through this type.
  /**
   * Performs:
   * @code get_service().destroy(get_implementation()); @endcode
   */
  ~basic_io_object()
  {
    service.destroy(implementation);
  }

  /// Get the service associated with the I/O object.
  service_type& get_service()
  {
    return service;
  }

  /// Get the service associated with the I/O object.
  const service_type& get_service() const
  {
    return service;
  }

  /// (Deprecated: Use get_service().) The service associated with the I/O
  /// object.
  /**
   * @note Available only for services that do not support movability.
   */
  service_type& service;

  /// Get the underlying implementation of the I/O object.
  implementation_type& get_implementation()
  {
    return implementation;
  }

  /// Get the underlying implementation of the I/O object.
  const implementation_type& get_implementation() const
  {
    return implementation;
  }

  /// (Deprecated: Use get_implementation().) The underlying implementation of
  /// the I/O object.
  implementation_type implementation;

private:
  basic_io_object(const basic_io_object&);
  basic_io_object& operator=(const basic_io_object&);
};

#if defined(ASIO_HAS_MOVE)
// Specialisation for movable objects.
template <typename IoObjectService>
class basic_io_object<IoObjectService, true>
{
public:
  typedef IoObjectService service_type;
  typedef typename service_type::implementation_type implementation_type;

  asio::io_service& get_io_service()
  {
    return service_->get_io_service();
  }

protected:
  explicit basic_io_object(asio::io_service& io_service)
    : service_(&asio::use_service<IoObjectService>(io_service))
  {
    service_->construct(implementation);
  }

  basic_io_object(basic_io_object&& other)
    : service_(&other.get_service())
  {
    service_->move_construct(implementation, other.implementation);
  }

  ~basic_io_object()
  {
    service_->destroy(implementation);
  }

  basic_io_object& operator=(basic_io_object&& other)
  {
    service_->move_assign(implementation,
        *other.service_, other.implementation);
    service_ = other.service_;
    return *this;
  }

  service_type& get_service()
  {
    return *service_;
  }

  const service_type& get_service() const
  {
    return *service_;
  }

  implementation_type& get_implementation()
  {
    return implementation;
  }

  const implementation_type& get_implementation() const
  {
    return implementation;
  }

  implementation_type implementation;

private:
  basic_io_object(const basic_io_object&);
  void operator=(const basic_io_object&);

  IoObjectService* service_;
};
#endif // defined(ASIO_HAS_MOVE)

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_BASIC_IO_OBJECT_HPP
