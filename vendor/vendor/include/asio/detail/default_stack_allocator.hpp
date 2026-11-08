//
// detail/default_stack_allocator.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_DEFAULT_STACK_ALLOCATOR_HPP
#define ASIO_DETAIL_DEFAULT_STACK_ALLOCATOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_ASM_FIBER)

#include <cstddef>

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

class default_stack_allocator
{
public:
  struct stack_context
  {
    // One past the last usable byte.
    void* sp;

    // The size of the region below sp, so that sp - size is its low end.
    std::size_t size;

    // The low end of the region. The guard page if there is one.
    void* base;
  };

  explicit default_stack_allocator(std::size_t size) noexcept
    : size_(size)
  {
  }

  // Distinct starting offsets handed out to successive stacks, and their
  // spacing. Every mapping is page-aligned, so the same offsets in each stack
  // would otherwise fall in the same cache sets and evict one another.
  // Positioning each stack a little further down spreads them out.
  enum
  {
    colour_count = 64,
    colour_step = 128
  };

  // The system page size, and the size of the guard page.
  ASIO_DECL static std::size_t page_size();

  // Obtains a stack, adding a guard page if required. Throws std::bad_alloc on
  // failure.
  ASIO_DECL stack_context allocate() const;

  // Release a stack obtained from allocate().
  ASIO_DECL void deallocate(stack_context& stack) const;

private:
  // The size of each stack's memory, including any guard page.
  ASIO_DECL std::size_t region_size() const;

  std::size_t size_;
};

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#if defined(ASIO_HEADER_ONLY)
# include "asio/detail/impl/default_stack_allocator.ipp"
#endif // defined(ASIO_HEADER_ONLY)

#endif // defined(ASIO_HAS_ASM_FIBER)

#endif // ASIO_DETAIL_DEFAULT_STACK_ALLOCATOR_HPP
