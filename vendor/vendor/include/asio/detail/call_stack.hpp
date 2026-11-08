//
// detail/call_stack.hpp
// ~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_CALL_STACK_HPP
#define ASIO_DETAIL_CALL_STACK_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/noncopyable.hpp"
#include "asio/detail/tss_ptr.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

template <typename T> class call_stack;

// Intrusive link node. T derives from call_frame<T> and provides key_type
// and key() (see thread_info_base). The linked object is the value, so
// top() needs no pointer chase to reach it, just a static_cast<T*>.
template <typename T>
class call_frame
  : private noncopyable
{
protected:
  call_frame() : key_(0), next_(0) {}
  ~call_frame() {}

  void* key_;
  call_frame* next_;

private:
  friend class call_stack<T>;
};

// Helper to determine whether or not the current thread is inside an
// invocation of io_context::run() for a specified owner, and to retrieve
// the T pushed for it.
template <typename T>
class call_stack
{
public:
  typedef typename T::key_type key_type;

  // Context class automatically links a T on to the stack, tagged with a
  // key, for the context object's lifetime.
  class context
    : private noncopyable
  {
  public:
    // Push t on to the stack under key k.
    context(key_type* k, T& t)
      : linked_(&t)
    {
      call_frame<T>* node = &t;
      node->key_ = k;
      node->next_ = call_stack<T>::top_;
      call_stack<T>::top_ = node;
    }

    // Pop t from the stack.
    ~context()
    {
      call_stack<T>::top_ = static_cast<call_frame<T>*>(linked_)->next_;
    }

    // Find the next linked T with the same key.
    T* next_by_key() const
    {
      call_frame<T>* self = linked_;
      call_frame<T>* elem = self->next_;
      while (elem)
      {
        if (elem->key_ == self->key_)
          return static_cast<T*>(elem);
        elem = elem->next_;
      }
      return 0;
    }

  private:
    T* linked_;
  };

  // Determine whether the specified owner is on the stack. Returns the
  // linked T if present, 0 otherwise.
  static T* contains(key_type* k)
  {
    call_frame<T>* elem = top_;
    while (elem)
    {
      T* t = static_cast<T*>(elem);
      if (t->key() == k)
        return t;
      elem = elem->next_;
    }
    return 0;
  }

  // Obtain the T at the top of the stack.
  static T* top()
  {
    call_frame<T>* elem = top_;
    return elem ? static_cast<T*>(elem) : 0;
  }

private:
  friend class context;

  // The top of the stack of calls for the current thread.
  static tss_ptr<call_frame<T> > top_;
};

template <typename T>
tss_ptr<call_frame<T> >
call_stack<T>::top_;

// A minimal T for stacks with no value beyond presence. contains() returns
// a non-null pointer, which is all such callers (e.g. strand) ever test.
template <typename Key>
class call_stack_key_only
  : public call_frame<call_stack_key_only<Key> >
{
public:
  typedef Key key_type;

  key_type* key() const
  {
    return static_cast<key_type*>(this->key_);
  }
};

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_CALL_STACK_HPP
