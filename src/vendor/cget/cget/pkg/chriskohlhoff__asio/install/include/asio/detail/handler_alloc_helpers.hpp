//
// detail/handler_alloc_helpers.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2023 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_HANDLER_ALLOC_HELPERS_HPP
#define ASIO_DETAIL_HANDLER_ALLOC_HELPERS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/memory.hpp"
#include "asio/detail/noncopyable.hpp"
#include "asio/detail/recycling_allocator.hpp"
#include "asio/detail/thread_info_base.hpp"
#include "asio/associated_allocator.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

inline void* default_allocate(std::size_t s,
    std::size_t align = ASIO_DEFAULT_ALIGN)
{
#if !defined(ASIO_DISABLE_SMALL_BLOCK_RECYCLING)
  return asio::detail::thread_info_base::allocate(
      asio::detail::thread_context::top_of_thread_call_stack(),
      s, align);
#else // !defined(ASIO_DISABLE_SMALL_BLOCK_RECYCLING)
  return asio::aligned_new(align, s);
#endif // !defined(ASIO_DISABLE_SMALL_BLOCK_RECYCLING)
}

inline void default_deallocate(void* p, std::size_t s)
{
#if !defined(ASIO_DISABLE_SMALL_BLOCK_RECYCLING)
  asio::detail::thread_info_base::deallocate(
      asio::detail::thread_context::top_of_thread_call_stack(), p, s);
#else // !defined(ASIO_DISABLE_SMALL_BLOCK_RECYCLING)
  (void)s;
  asio::aligned_delete(p);
#endif // !defined(ASIO_DISABLE_SMALL_BLOCK_RECYCLING)
}

template <typename T>
class default_allocator
{
public:
  typedef T value_type;

  template <typename U>
  struct rebind
  {
    typedef default_allocator<U> other;
  };

  default_allocator() noexcept
  {
  }

  template <typename U>
  default_allocator(const default_allocator<U>&) noexcept
  {
  }

  T* allocate(std::size_t n)
  {
    return static_cast<T*>(default_allocate(sizeof(T) * n, alignof(T)));
  }

  void deallocate(T* p, std::size_t n)
  {
    default_deallocate(p, sizeof(T) * n);
  }
};

template <>
class default_allocator<void>
{
public:
  typedef void value_type;

  template <typename U>
  struct rebind
  {
    typedef default_allocator<U> other;
  };

  default_allocator() noexcept
  {
  }

  template <typename U>
  default_allocator(const default_allocator<U>&) noexcept
  {
  }
};

template <typename Allocator>
struct get_default_allocator
{
  typedef Allocator type;

  static type get(const Allocator& a)
  {
    return a;
  }
};

template <typename T>
struct get_default_allocator<std::allocator<T>>
{
  typedef default_allocator<T> type;

  static type get(const std::allocator<T>&)
  {
    return type();
  }
};

} // namespace detail
} // namespace asio

#define ASIO_DEFINE_HANDLER_PTR(op) \
  struct ptr \
  { \
    Handler* h; \
    op* v; \
    op* p; \
    ~ptr() \
    { \
      reset(); \
    } \
    static op* allocate(Handler& handler) \
    { \
      typedef typename ::asio::associated_allocator< \
        Handler>::type associated_allocator_type; \
      typedef typename ::asio::detail::get_default_allocator< \
        associated_allocator_type>::type default_allocator_type; \
      ASIO_REBIND_ALLOC(default_allocator_type, op) a( \
            ::asio::detail::get_default_allocator< \
              associated_allocator_type>::get( \
                ::asio::get_associated_allocator(handler))); \
      return a.allocate(1); \
    } \
    void reset() \
    { \
      if (p) \
      { \
        p->~op(); \
        p = 0; \
      } \
      if (v) \
      { \
        typedef typename ::asio::associated_allocator< \
          Handler>::type associated_allocator_type; \
        typedef typename ::asio::detail::get_default_allocator< \
          associated_allocator_type>::type default_allocator_type; \
        ASIO_REBIND_ALLOC(default_allocator_type, op) a( \
              ::asio::detail::get_default_allocator< \
                associated_allocator_type>::get( \
                  ::asio::get_associated_allocator(*h))); \
        a.deallocate(static_cast<op*>(v), 1); \
        v = 0; \
      } \
    } \
  } \
  /**/

#define ASIO_DEFINE_TAGGED_HANDLER_ALLOCATOR_PTR(purpose, op) \
  struct ptr \
  { \
    const Alloc* a; \
    void* v; \
    op* p; \
    ~ptr() \
    { \
      reset(); \
    } \
    static op* allocate(const Alloc& a) \
    { \
      typedef typename ::asio::detail::get_recycling_allocator< \
        Alloc, purpose>::type recycling_allocator_type; \
      ASIO_REBIND_ALLOC(recycling_allocator_type, op) a1( \
            ::asio::detail::get_recycling_allocator< \
              Alloc, purpose>::get(a)); \
      return a1.allocate(1); \
    } \
    void reset() \
    { \
      if (p) \
      { \
        p->~op(); \
        p = 0; \
      } \
      if (v) \
      { \
        typedef typename ::asio::detail::get_recycling_allocator< \
          Alloc, purpose>::type recycling_allocator_type; \
        ASIO_REBIND_ALLOC(recycling_allocator_type, op) a1( \
              ::asio::detail::get_recycling_allocator< \
                Alloc, purpose>::get(*a)); \
        a1.deallocate(static_cast<op*>(v), 1); \
        v = 0; \
      } \
    } \
  } \
  /**/

#define ASIO_DEFINE_HANDLER_ALLOCATOR_PTR(op) \
  ASIO_DEFINE_TAGGED_HANDLER_ALLOCATOR_PTR( \
      ::asio::detail::thread_info_base::default_tag, op ) \
  /**/

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_HANDLER_ALLOC_HELPERS_HPP
