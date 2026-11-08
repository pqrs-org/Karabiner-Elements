//
// detail/boost_fiber.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_BOOST_FIBER_HPP
#define ASIO_DETAIL_BOOST_FIBER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_BOOST_CONTEXT_FIBER)

#include <cstddef>
#include <boost/context/fiber.hpp>
#include "asio/detail/memory.hpp"
#include "asio/detail/type_traits.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

// Thrown into a coroutine to unwind it.
using boost_forced_unwind = boost::context::detail::forced_unwind;

class boost_fiber
{
public:
  boost_fiber() noexcept
  {
  }

  static boost::context::fixedsize_stack default_stack() noexcept
  {
    return boost::context::fixedsize_stack(ASIO_DEFAULT_FIBER_STACK_SIZE);
  }

  template <typename StackAllocator, typename Fn>
  boost_fiber(allocator_arg_t, StackAllocator&& stack_allocator, Fn&& fn)
    : impl_(allocator_arg_t(),
        adapted_allocator_t<StackAllocator>(
          static_cast<StackAllocator&&>(stack_allocator)),
        entry_point<decay_t<Fn>>(static_cast<Fn&&>(fn)))
  {
  }

  boost_fiber(boost_fiber&& other) noexcept
    : impl_(static_cast<boost::context::fiber&&>(other.impl_))
  {
  }

  boost_fiber& operator=(boost_fiber&& other) noexcept
  {
    impl_ = static_cast<boost::context::fiber&&>(other.impl_);
    return *this;
  }

  ~boost_fiber()
  {
  }

  ASIO_NEVER_INLINE void resume()
  {
    impl_ = boost::context::fiber(
        static_cast<boost::context::fiber&&>(impl_)).resume();
  }

  boost_fiber(const boost_fiber&) = delete;
  boost_fiber& operator=(const boost_fiber&) = delete;

private:
  explicit boost_fiber(boost::context::fiber&& impl) noexcept
    : impl_(static_cast<boost::context::fiber&&>(impl))
  {
  }

  template <typename StackAllocator>
  using stack_context_of_t = decay_t<decltype(
      declval<StackAllocator&>().allocate())>;

  // Adapts a stack allocator to produce boost::context::stack_context.
  template <typename StackAllocator>
  class allocator_adapter
  {
  public:
    using stack_context_type = decay_t<decltype(
        declval<StackAllocator&>().allocate())>;

    template <typename A>
    explicit allocator_adapter(A&& a)
      : alloc_(static_cast<A&&>(a)),
        have_stack_context_(false)
    {
    }

    allocator_adapter(allocator_adapter&& other)
      : alloc_(static_cast<StackAllocator&&>(other.alloc_)),
        have_stack_context_(other.have_stack_context_)
    {
      if (have_stack_context_)
      {
        new (&stack_context_.obj_) stack_context_type(
            other.stack_context_.obj_);
        other.destroy_stack_context();
      }
    }

    ~allocator_adapter()
    {
      destroy_stack_context();
    }

    boost::context::stack_context allocate()
    {
      new (&stack_context_.obj_) stack_context_type(alloc_.allocate());
      have_stack_context_ = true;
      boost::context::stack_context result;
      result.size = stack_context_.obj_.size;
      result.sp = stack_context_.obj_.sp;
      return result;
    }

    void deallocate(boost::context::stack_context&)
    {
      alloc_.deallocate(stack_context_.obj_);
      destroy_stack_context();
    }

  private:
    void destroy_stack_context()
    {
      if (have_stack_context_)
      {
        stack_context_.obj_.~stack_context_type();
        have_stack_context_ = false;
      }
    }

    union storage
    {
      storage() {}
      ~storage() {}

      char dummy_;
      stack_context_type obj_;
    };

    StackAllocator alloc_;
    bool have_stack_context_;
    storage stack_context_;
  };

  template <typename StackAllocator>
  using adapted_allocator_t = conditional_t<
      is_same<stack_context_of_t<decay_t<StackAllocator>>,
          boost::context::stack_context>::value,
      decay_t<StackAllocator>,
      allocator_adapter<decay_t<StackAllocator>>>;

  // Adapts the coroutine function to the signature Boost.Context expects.
  template <typename Fn>
  class entry_point
  {
  public:
    template <typename F>
    explicit entry_point(F&& f)
      : fn_(static_cast<F&&>(f))
    {
    }

    boost::context::fiber operator()(boost::context::fiber&& caller)
    {
      boost_fiber result = fn_(
          boost_fiber(static_cast<boost::context::fiber&&>(caller)));
      return static_cast<boost::context::fiber&&>(result.impl_);
    }

  private:
    Fn fn_;
  };

  boost::context::fiber impl_;
};

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // defined(ASIO_HAS_BOOST_CONTEXT_FIBER)

#endif // ASIO_DETAIL_BOOST_FIBER_HPP
