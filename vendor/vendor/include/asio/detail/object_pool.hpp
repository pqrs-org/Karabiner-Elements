//
// detail/object_pool.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2025 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_OBJECT_POOL_HPP
#define ASIO_DETAIL_OBJECT_POOL_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/memory.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

template <typename Object, typename Allocator>
class object_pool
{
public:
  // Constructor.
  template <typename... Args>
  object_pool(const Allocator& allocator,
      unsigned int preallocated, Args... args)
    : allocator_(allocator),
      live_list_(0),
      free_list_(0)
  {
    while (preallocated > 0)
    {
      Object* o = allocate_object<Object>(allocator_, args...);
      o->next_ = free_list_;
      o->prev_ = 0;
      free_list_ = o;
      --preallocated;
    }
  }

  // Destructor destroys all objects.
  ~object_pool()
  {
    destroy_list(live_list_);
    destroy_list(free_list_);
  }

  // Get the object at the start of the live list.
  Object* first()
  {
    return live_list_;
  }

  // Allocate a new object with an argument.
  template <typename... Args>
  Object* alloc(Args... args)
  {
    Object* o = free_list_;
    if (o)
      free_list_ = free_list_->next_;
    else
      o = allocate_object<Object>(allocator_, args...);

    o->next_ = live_list_;
    o->prev_ = 0;
    if (live_list_)
      live_list_->prev_ = o;
    live_list_ = o;

    return o;
  }

  // Free an object. Moves it to the free list. No destructors are run.
  void free(Object* o)
  {
    if (live_list_ == o)
      live_list_ = o->next_;

    if (o->prev_)
      o->prev_->next_ = o->next_;

    if (o->next_)
      o->next_->prev_ = o->prev_;

    o->next_ = free_list_;
    o->prev_ = 0;
    free_list_ = o;
  }

private:
  object_pool(const object_pool&) = delete;
  object_pool& operator=(const object_pool&) = delete;

  // Helper function to destroy all elements in a list.
  void destroy_list(Object* list)
  {
    while (list)
    {
      Object* o = list;
      list = o->next_;
      deallocate_object(allocator_, o);
    }
  }

  // The execution_context allocator used to manage pooled object memory.
  Allocator allocator_;

  // The list of live objects.
  Object* live_list_;

  // The free list.
  Object* free_list_;
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_OBJECT_POOL_HPP
