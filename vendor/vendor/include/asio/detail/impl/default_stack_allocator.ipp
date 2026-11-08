//
// detail/impl/default_stack_allocator.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_IMPL_DEFAULT_STACK_ALLOCATOR_IPP
#define ASIO_DETAIL_IMPL_DEFAULT_STACK_ALLOCATOR_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_ASM_FIBER)

#include <new>
#include "asio/detail/atomic_count.hpp"
#include <sys/mman.h>
#include <unistd.h>
#include "asio/detail/default_stack_allocator.hpp"
#include "asio/detail/memory.hpp"
#include "asio/detail/throw_exception.hpp"

#if !defined(ASIO_DETAIL_MAP_ANONYMOUS)
# if defined(MAP_ANONYMOUS)
#  define ASIO_DETAIL_MAP_ANONYMOUS MAP_ANONYMOUS
# else // defined(MAP_ANONYMOUS)
#  define ASIO_DETAIL_MAP_ANONYMOUS MAP_ANON
# endif // defined(MAP_ANONYMOUS)
#endif // !defined(ASIO_DETAIL_MAP_ANONYMOUS)

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

std::size_t default_stack_allocator::page_size()
{
  static const std::size_t page_size =
    static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));
  return page_size;
}

std::size_t default_stack_allocator::region_size() const
{
  const std::size_t page = page_size();
  std::size_t requested_size = size_ == 0 ? 1 : size_;

  const std::size_t usable_size =
    ((requested_size + colour_count * colour_step + page - 1) / page) * page;

#if defined(ASIO_HAS_FIBER_GUARD_PAGE)
  return usable_size + page;
#else // defined(ASIO_HAS_FIBER_GUARD_PAGE)
  return usable_size;
#endif // defined(ASIO_HAS_FIBER_GUARD_PAGE)
}

default_stack_allocator::stack_context
default_stack_allocator::allocate() const
{
  const std::size_t page = page_size();
  const std::size_t total_size = region_size();

  static atomic_count colour(0);
  increment(colour, 1);
  const std::size_t offset =
    (static_cast<std::size_t>(static_cast<long>(colour)) % colour_count)
    * colour_step;

  stack_context stack;

#if defined(ASIO_HAS_FIBER_GUARD_PAGE)
  int flags = MAP_PRIVATE | ASIO_DETAIL_MAP_ANONYMOUS;
# if defined(MAP_STACK)
  // Linux only.
  flags |= MAP_STACK;
# endif // defined(MAP_STACK)

  void* base = ::mmap(0, total_size, PROT_READ | PROT_WRITE, flags, -1, 0);
  if (base == MAP_FAILED)
  {
    std::bad_alloc ex;
    asio::detail::throw_exception(ex);
  }

  // Stacks grow down, so the guard belongs at the low end.
  if (::mprotect(base, page, PROT_NONE) != 0)
  {
    ::munmap(base, total_size);
    std::bad_alloc ex;
    asio::detail::throw_exception(ex);
  }
#else // defined(ASIO_HAS_FIBER_GUARD_PAGE)
  void* base = aligned_new(page, total_size);
#endif // defined(ASIO_HAS_FIBER_GUARD_PAGE)

  // The colour offset is cut from the top, so the size must exclude it too.
  stack.base = base;
  stack.sp = static_cast<char*>(base) + total_size - offset;
  stack.size = total_size - offset;
  return stack;
}

void default_stack_allocator::deallocate(stack_context& stack) const
{
  if (!stack.base)
    return;
#if defined(ASIO_HAS_FIBER_GUARD_PAGE)
  // The stack's size excludes the colour offset, so it can't be used here.
  ::munmap(stack.base, region_size());
#else // defined(ASIO_HAS_FIBER_GUARD_PAGE)
  aligned_delete(stack.base);
#endif // defined(ASIO_HAS_FIBER_GUARD_PAGE)
}

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // defined(ASIO_HAS_ASM_FIBER)

#endif // ASIO_DETAIL_IMPL_DEFAULT_STACK_ALLOCATOR_IPP
