//
// impl/execution_context.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2025 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_IMPL_EXECUTION_CONTEXT_HPP
#define ASIO_IMPL_EXECUTION_CONTEXT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstring>
#include "asio/detail/handler_type_requirements.hpp"
#include "asio/detail/memory.hpp"
#include "asio/detail/service_registry.hpp"
#include "asio/detail/throw_exception.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {

template <typename Allocator>
execution_context::execution_context(allocator_arg_t, const Allocator& a)
  : execution_context(detail::allocate_object<allocator_impl<Allocator>>(a, a))
{
}

template <typename Allocator>
execution_context::execution_context(allocator_arg_t, const Allocator& a,
    const service_maker& initial_services)
  : execution_context(detail::allocate_object<allocator_impl<Allocator>>(a, a),
      initial_services)
{
}

inline execution_context::auto_allocator_ptr::~auto_allocator_ptr()
{
  ptr_->destroy();
}

template <typename Allocator>
void execution_context::allocator_impl<Allocator>::destroy()
{
  detail::deallocate_object(allocator_, this);
}

template <typename Allocator>
void* execution_context::allocator_impl<Allocator>::allocate(
    std::size_t size, std::size_t align)
{
  typename std::allocator_traits<Allocator>::template
    rebind_alloc<unsigned char> alloc(allocator_);

  std::size_t space = size + align - 1;
  unsigned char* base = std::allocator_traits<decltype(alloc)>::allocate(
      alloc, space + sizeof(std::ptrdiff_t));

  void* p = base;
  if (detail::align(align, size, p, space))
  {
    std::ptrdiff_t off = static_cast<unsigned char*>(p) - base;
    std::memcpy(static_cast<unsigned char*>(p) + size, &off, sizeof(off));
    return p;
  }

  std::bad_alloc ex;
  asio::detail::throw_exception(ex);
  return 0;
}

template <typename Allocator>
void execution_context::allocator_impl<Allocator>::deallocate(
    void* ptr, std::size_t size, std::size_t align)
{
  if (ptr)
  {
    typename std::allocator_traits<Allocator>::template
      rebind_alloc<unsigned char> alloc(allocator_);

    std::ptrdiff_t off;
    std::memcpy(&off, static_cast<unsigned char*>(ptr) + size, sizeof(off));
    unsigned char* base = static_cast<unsigned char*>(ptr) - off;

    std::allocator_traits<decltype(alloc)>::deallocate(
        alloc, base, size + align - 1 + sizeof(std::ptrdiff_t));
  }
}

#if !defined(GENERATING_DOCUMENTATION)

template <typename Service>
inline Service& use_service(execution_context& e)
{
  // Check that Service meets the necessary type requirements.
  (void)static_cast<execution_context::service*>(static_cast<Service*>(0));

  return e.service_registry_->template use_service<Service>();
}

template <typename Service, typename... Args>
Service& make_service(execution_context& e, Args&&... args)
{
  // Check that Service meets the necessary type requirements.
  (void)static_cast<execution_context::service*>(static_cast<Service*>(0));

  return e.service_registry_->template make_service<Service>(
      static_cast<Args&&>(args)...);
}

template <typename Service>
ASIO_DEPRECATED_MSG("Use make_service()")
inline void add_service(execution_context& e, Service* svc)
{
  // Check that Service meets the necessary type requirements.
  (void)static_cast<execution_context::service*>(static_cast<Service*>(0));

  e.service_registry_->template add_service<Service>(svc);
}

template <typename Service>
inline bool has_service(execution_context& e)
{
  // Check that Service meets the necessary type requirements.
  (void)static_cast<execution_context::service*>(static_cast<Service*>(0));

  return e.service_registry_->template has_service<Service>();
}

#endif // !defined(GENERATING_DOCUMENTATION)

inline execution_context& execution_context::service::context()
{
  return owner_;
}

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_IMPL_EXECUTION_CONTEXT_HPP
