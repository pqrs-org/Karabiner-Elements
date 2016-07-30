//
// detail/handler_alloc_helpers.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
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
#include "asio/detail/addressof.hpp"
#include "asio/detail/noncopyable.hpp"
#include "asio/handler_alloc_hook.hpp"

#include "asio/detail/push_options.hpp"

// Calls to asio_handler_allocate and asio_handler_deallocate must be made from
// a namespace that does not contain any overloads of these functions. The
// asio_handler_alloc_helpers namespace is defined here for that purpose.
namespace asio_handler_alloc_helpers {

template <typename Handler>
inline void* allocate(std::size_t s, Handler& h)
{
#if !defined(ASIO_HAS_HANDLER_HOOKS)
  return ::operator new(s);
#else
  using asio::asio_handler_allocate;
  return asio_handler_allocate(s, asio::detail::addressof(h));
#endif
}

template <typename Handler>
inline void deallocate(void* p, std::size_t s, Handler& h)
{
#if !defined(ASIO_HAS_HANDLER_HOOKS)
  ::operator delete(p);
#else
  using asio::asio_handler_deallocate;
  asio_handler_deallocate(p, s, asio::detail::addressof(h));
#endif
}

} // namespace asio_handler_alloc_helpers

#define ASIO_DEFINE_HANDLER_PTR(op) \
  struct ptr \
  { \
    Handler* h; \
    void* v; \
    op* p; \
    ~ptr() \
    { \
      reset(); \
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
        asio_handler_alloc_helpers::deallocate(v, sizeof(op), *h); \
        v = 0; \
      } \
    } \
  } \
  /**/

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_HANDLER_ALLOC_HELPERS_HPP
