//
// detail/asm_fiber.hpp
// ~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_ASM_FIBER_HPP
#define ASIO_DETAIL_ASM_FIBER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_ASM_FIBER)

#include <cstddef>
#include <new>
#include "asio/detail/assert.hpp"
#include "asio/detail/default_stack_allocator.hpp"
#include "asio/detail/memory.hpp"
#include "asio/detail/type_traits.hpp"

#if defined(ASIO_HAS_THREAD_SANITIZER_FIBERS)
# include <sanitizer/tsan_interface.h>
#endif // defined(ASIO_HAS_THREAD_SANITIZER_FIBERS)

#if defined(ASIO_HAS_ADDRESS_SANITIZER)
# if defined(ASIO_HAS_SANITIZER_COMMON_INTERFACE_DEFS)
#  include <sanitizer/common_interface_defs.h>
# else // defined(ASIO_HAS_SANITIZER_COMMON_INTERFACE_DEFS)
extern "C" void __sanitizer_start_switch_fiber(
    void** fake_stack_save, const void* bottom, std::size_t size);
extern "C" void __sanitizer_finish_switch_fiber(
    void* fake_stack_save, const void** bottom_old, std::size_t* size_old);
# endif // defined(ASIO_HAS_SANITIZER_COMMON_INTERFACE_DEFS)
#endif // defined(ASIO_HAS_ADDRESS_SANITIZER)

#if defined(ASIO_HAS_VALGRIND)
# include <valgrind/valgrind.h>
#endif // defined(ASIO_HAS_VALGRIND)

// Use versioned names for symbols and labels defined in assembly.
#if defined(__APPLE__)
# define ASIO_DETAIL_ASM_SYMBOL(name) \
    "_" ASIO_DETAIL_STRINGIFY(ASIO_VERSIONED_NAME(detail_ ## name))
# define ASIO_DETAIL_ASM_LABEL(name) \
    "L" ASIO_DETAIL_STRINGIFY(ASIO_VERSIONED_NAME(detail_ ## name))
#else // defined(__APPLE__)
# define ASIO_DETAIL_ASM_SYMBOL(name) \
    ASIO_DETAIL_STRINGIFY(ASIO_VERSIONED_NAME(detail_ ## name))
# define ASIO_DETAIL_ASM_LABEL(name) \
    ".L" ASIO_DETAIL_STRINGIFY(ASIO_VERSIONED_NAME(detail_ ## name))
#endif // defined(__APPLE__)

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

struct fiber_context
{
  void* sp;
  void* data;
};

fiber_context make_fiber_context(void* sp,
    std::size_t size, void (*fn)(fiber_context))
  __asm__(ASIO_DETAIL_ASM_SYMBOL(make_fiber_context));

// Space reserved in a fiber's stack memory for the context.
#if defined(__aarch64__)
enum { fiber_context_block_size = 0xb0 };
#else // defined(__aarch64__)
enum { fiber_context_block_size = 0x40 };
#endif // defined(__aarch64__)

fiber_context swap_fiber_context(fiber_context to)
  __asm__(ASIO_DETAIL_ASM_SYMBOL(swap_fiber_context));

// Emits the assembly definitions whenever the function is used.
ASIO_DECL void asm_fiber_definitions();

// Decorate a fiber with additional context needed for sanitisers.
struct fiber_switch_result
{
  fiber_context context;
#if defined(ASIO_HAS_ADDRESS_SANITIZER)
  const void* from_bottom;
  std::size_t from_size;
#endif // defined(ASIO_HAS_ADDRESS_SANITIZER)
};

class asm_fiber;
template <typename Fn, typename StackAllocator> class fiber_control_block;

// Thrown into a coroutine to unwind it when it is abandoned.
class asm_fiber_forced_unwind
{
public:
  explicit asm_fiber_forced_unwind(const fiber_switch_result& result) noexcept
    : result_(result)
  {
  }

  // Describes the context to resume once the stack has finished unwinding.
  fiber_switch_result result_;
};

inline void* unwind_marker() noexcept
{
  return reinterpret_cast<void*>(1);
}

// Placed at the top of the stack it describes, above the saved registers.
class fiber_control_block_base
{
public:
  fiber_control_block_base() noexcept
  {
#if defined(ASIO_HAS_THREAD_SANITIZER_FIBERS)
    tsan_fiber_ = 0;
#endif // defined(ASIO_HAS_THREAD_SANITIZER_FIBERS)
#if defined(ASIO_HAS_VALGRIND)
    valgrind_stack_id_ = 0;
#endif // defined(ASIO_HAS_VALGRIND)
  }

  virtual ~fiber_control_block_base()
  {
  }

  // Runs the coroutine function. Returns the context to resume once it is done.
  virtual asm_fiber run(asm_fiber&& caller) = 0;

  // Must not be called from a context running on the stack being released.
  virtual void destroy() = 0;

#if defined(ASIO_HAS_THREAD_SANITIZER_FIBERS)
  void* tsan_fiber_;
#endif // defined(ASIO_HAS_THREAD_SANITIZER_FIBERS)

#if defined(ASIO_HAS_VALGRIND)
  unsigned valgrind_stack_id_;
#endif // defined(ASIO_HAS_VALGRIND)
};

// State needed to resume a fiber context.
struct fiber_switch_target
{
  void* sp;
  void* unstarted_block;
#if defined(ASIO_HAS_ADDRESS_SANITIZER)
  const void* bottom;
  std::size_t size;
#endif // defined(ASIO_HAS_ADDRESS_SANITIZER)
};

class asm_fiber
{
public:
  asm_fiber() noexcept
    : sp_(0),
      unstarted_block_(0)
#if defined(ASIO_NO_EXCEPTIONS)
      , block_(0)
#endif // defined(ASIO_NO_EXCEPTIONS)
#if defined(ASIO_HAS_ADDRESS_SANITIZER)
      , stack_bottom_(0),
      stack_size_(0)
#endif // defined(ASIO_HAS_ADDRESS_SANITIZER)
  {
  }

  // The stack allocator used when spawn() is not given one.
  static default_stack_allocator default_stack() noexcept
  {
    return default_stack_allocator(ASIO_DEFAULT_FIBER_STACK_SIZE);
  }

  // Takes any allocator meeting the StackAllocator requirements.
  template <typename StackAllocator, typename Fn>
  asm_fiber(allocator_arg_t, StackAllocator&& stack_allocator, Fn&& fn)
    : sp_(0),
      unstarted_block_(0)
#if defined(ASIO_NO_EXCEPTIONS)
      , block_(0)
#endif // defined(ASIO_NO_EXCEPTIONS)
#if defined(ASIO_HAS_ADDRESS_SANITIZER)
      , stack_bottom_(0),
      stack_size_(0)
#endif // defined(ASIO_HAS_ADDRESS_SANITIZER)
  {
    asm_fiber_definitions();

    using block_type =
      fiber_control_block<decay_t<Fn>, decay_t<StackAllocator>>;
    typename block_type::stack_context_type sctx = stack_allocator.allocate();

    char* stack_bottom = static_cast<char*>(sctx.sp) - sctx.size;
    char* block_addr = static_cast<char*>(sctx.sp) - sizeof(block_type);
    block_addr = reinterpret_cast<char*>(
        reinterpret_cast<std::size_t>(block_addr)
          & ~static_cast<std::size_t>(alignof(block_type) - 1));

    // The stack must be at least large enough to hold a control block.
    ASIO_ASSERT(block_addr > stack_bottom
        && static_cast<std::size_t>(block_addr - stack_bottom)
             > fiber_context_block_size
        && "the stack given to spawn() is too small");

    block_type* block;
#if !defined(ASIO_NO_EXCEPTIONS)
    try
#endif // !defined(ASIO_NO_EXCEPTIONS)
    {
      block = new (block_addr) block_type(sctx,
          static_cast<StackAllocator&&>(stack_allocator),
          static_cast<Fn&&>(fn));
    }
#if !defined(ASIO_NO_EXCEPTIONS)
    catch (...)
    {
      stack_allocator.deallocate(sctx);
      throw;
    }
#endif // !defined(ASIO_NO_EXCEPTIONS)

    sp_ = make_fiber_context(block_addr,
        static_cast<std::size_t>(block_addr - stack_bottom), &fiber_entry).sp;

    unstarted_block_ = block;

#if defined(ASIO_NO_EXCEPTIONS)
    block_ = block;
#endif // defined(ASIO_NO_EXCEPTIONS)

#if defined(ASIO_HAS_ADDRESS_SANITIZER)
    stack_bottom_ = stack_bottom;
    stack_size_ = sctx.size;
#endif // defined(ASIO_HAS_ADDRESS_SANITIZER)
#if defined(ASIO_HAS_THREAD_SANITIZER_FIBERS)
    block->tsan_fiber_ = __tsan_create_fiber(0);
#endif // defined(ASIO_HAS_THREAD_SANITIZER_FIBERS)
#if defined(ASIO_HAS_VALGRIND)
    block->valgrind_stack_id_ = VALGRIND_STACK_REGISTER(stack_bottom, sctx.sp);
#endif // defined(ASIO_HAS_VALGRIND)
  }

  asm_fiber(asm_fiber&& other) noexcept
    : sp_(other.sp_),
      unstarted_block_(other.unstarted_block_)
#if defined(ASIO_NO_EXCEPTIONS)
      , block_(other.block_)
#endif // defined(ASIO_NO_EXCEPTIONS)
#if defined(ASIO_HAS_ADDRESS_SANITIZER)
      , stack_bottom_(other.stack_bottom_),
      stack_size_(other.stack_size_)
#endif // defined(ASIO_HAS_ADDRESS_SANITIZER)
  {
    other.detach();
  }

  asm_fiber& operator=(asm_fiber&& other) noexcept
  {
    if (this != &other)
    {
      {
        asm_fiber discarded(
            static_cast<asm_fiber&&>(*this));
      }
      sp_ = other.sp_;
      unstarted_block_ = other.unstarted_block_;
#if defined(ASIO_NO_EXCEPTIONS)
      block_ = other.block_;
#endif // defined(ASIO_NO_EXCEPTIONS)
#if defined(ASIO_HAS_ADDRESS_SANITIZER)
      stack_bottom_ = other.stack_bottom_;
      stack_size_ = other.stack_size_;
#endif // defined(ASIO_HAS_ADDRESS_SANITIZER)
      other.detach();
    }
    return *this;
  }

  // Unwinds the coroutine if it is still live.
  ~asm_fiber()
  {
    if (!sp_)
      return;

#if defined(ASIO_NO_EXCEPTIONS)
    fiber_control_block_base* block = block_;
#endif // defined(ASIO_NO_EXCEPTIONS)

    fiber_switch_target target = detach();
    if (target.unstarted_block)
    {
      static_cast<fiber_control_block_base*>(target.unstarted_block)->destroy();
      return;
    }

#if defined(ASIO_NO_EXCEPTIONS)
    // Objects still live on the stack are not destroyed.
    if (block)
      block->destroy();
#else // defined(ASIO_NO_EXCEPTIONS)
    // Control is transferred to another stack so this one can then be released.
    fiber_switch_result r = switch_to(target, unwind_marker());
    if (r.context.data && r.context.data != unwind_marker())
      static_cast<fiber_control_block_base*>(r.context.data)->destroy();
#endif // defined(ASIO_NO_EXCEPTIONS)
  }

  ASIO_NEVER_INLINE void resume()
  {
    fiber_switch_target target = detach();
    fiber_switch_result r = switch_to(target, target.unstarted_block);
    if (r.context.data)
    {
      unwind_or_release(r);
      return;
    }
    assign(r);
  }

  asm_fiber(const asm_fiber&) = delete;
  asm_fiber& operator=(const asm_fiber&) = delete;

private:
  template <typename, typename> friend class fiber_control_block;

  static void fiber_entry(fiber_context t) noexcept
  {
    fiber_control_block_base* block =
      static_cast<fiber_control_block_base*>(t.data);

#if defined(ASIO_HAS_THREAD_SANITIZER_FIBERS)
    if (block->tsan_fiber_)
      __tsan_switch_to_fiber(block->tsan_fiber_, 0);
#endif // defined(ASIO_HAS_THREAD_SANITIZER_FIBERS)

    fiber_switch_result caller;
    caller.context = t;
#if defined(ASIO_HAS_ADDRESS_SANITIZER)
    __sanitizer_finish_switch_fiber(0, &caller.from_bottom, &caller.from_size);
#endif // defined(ASIO_HAS_ADDRESS_SANITIZER)

    asm_fiber destination;
#if !defined(ASIO_NO_EXCEPTIONS)
    try
#endif // !defined(ASIO_NO_EXCEPTIONS)
    {
      destination = block->run(adopt(caller));
    }
#if !defined(ASIO_NO_EXCEPTIONS)
    catch (const asm_fiber_forced_unwind& ex)
    {
      // The coroutine was abandoned rather than allowed to finish.
      destination = adopt(ex.result_);
    }
#endif // !defined(ASIO_NO_EXCEPTIONS)

    // The resumed context destroys it, this frame standing on that memory.
    switch_and_die(static_cast<asm_fiber&&>(destination), block);
  }

  ASIO_NEVER_INLINE static void unwind_or_release(
      const fiber_switch_result& r)
  {
#if !defined(ASIO_NO_EXCEPTIONS)
    if (r.context.data == unwind_marker())
    {
      throw asm_fiber_forced_unwind(r);
    }
#endif // !defined(ASIO_NO_EXCEPTIONS)

    static_cast<fiber_control_block_base*>(r.context.data)->destroy();
  }

  void assign(const fiber_switch_result& r) noexcept
  {
    sp_ = r.context.sp;
#if defined(ASIO_HAS_ADDRESS_SANITIZER)
    stack_bottom_ = r.from_bottom;
    stack_size_ = r.from_size;
#endif // defined(ASIO_HAS_ADDRESS_SANITIZER)
  }

  static asm_fiber adopt(const fiber_switch_result& r) noexcept
  {
    asm_fiber result;
    result.assign(r);
    return result;
  }

  // Give up ownership without unwinding.
  fiber_switch_target detach() noexcept
  {
    fiber_switch_target target;
    target.sp = sp_;
    target.unstarted_block = unstarted_block_;
#if defined(ASIO_HAS_ADDRESS_SANITIZER)
    target.bottom = stack_bottom_;
    target.size = stack_size_;
    stack_bottom_ = 0;
    stack_size_ = 0;
#endif // defined(ASIO_HAS_ADDRESS_SANITIZER)
    sp_ = 0;
    unstarted_block_ = 0;
    return target;
  }

  // Perform the switch, telling any sanitiser that is watching.
  static fiber_switch_result switch_to(
      const fiber_switch_target& target, void* data)
  {
#if defined(ASIO_HAS_THREAD_SANITIZER_FIBERS)
    void* self = __tsan_get_current_fiber();
#endif // defined(ASIO_HAS_THREAD_SANITIZER_FIBERS)
#if defined(ASIO_HAS_ADDRESS_SANITIZER)
    void* fake_stack = 0;
    __sanitizer_start_switch_fiber(&fake_stack, target.bottom, target.size);
#endif // defined(ASIO_HAS_ADDRESS_SANITIZER)

    fiber_switch_result r;
    fiber_context dest = { target.sp, data };
    r.context = swap_fiber_context(dest);

#if defined(ASIO_HAS_THREAD_SANITIZER_FIBERS)
    __tsan_switch_to_fiber(self, 0);
#endif // defined(ASIO_HAS_THREAD_SANITIZER_FIBERS)
#if defined(ASIO_HAS_ADDRESS_SANITIZER)
    __sanitizer_finish_switch_fiber(fake_stack, &r.from_bottom, &r.from_size);
#endif // defined(ASIO_HAS_ADDRESS_SANITIZER)

    return r;
  }

  static void switch_and_die(asm_fiber&& to, void* data)
  {
    fiber_switch_target target = to.detach();
#if defined(ASIO_HAS_ADDRESS_SANITIZER)
    __sanitizer_start_switch_fiber(0, target.bottom, target.size);
#endif // defined(ASIO_HAS_ADDRESS_SANITIZER)
    fiber_context dest = { target.sp, data };
    swap_fiber_context(dest);
  }

  void* sp_;
  void* unstarted_block_;

#if defined(ASIO_NO_EXCEPTIONS)
  fiber_control_block_base* block_;
#endif // defined(ASIO_NO_EXCEPTIONS)

#if defined(ASIO_HAS_ADDRESS_SANITIZER)
  const void* stack_bottom_;
  std::size_t stack_size_;
#endif // defined(ASIO_HAS_ADDRESS_SANITIZER)
};

template <typename Fn, typename StackAllocator>
class fiber_control_block
  : public fiber_control_block_base
{
public:
  using stack_context_type = decay_t<decltype(
      declval<StackAllocator&>().allocate())>;

  template <typename A, typename F>
  fiber_control_block(const stack_context_type& sctx, A&& a, F&& f)
    : sctx_(sctx),
      alloc_(static_cast<A&&>(a)),
      fn_(static_cast<F&&>(f))
  {
  }

  asm_fiber run(asm_fiber&& caller)
  {
    return fn_(static_cast<asm_fiber&&>(caller));
  }

  void destroy()
  {
    StackAllocator alloc(static_cast<StackAllocator&&>(alloc_));
    stack_context_type sctx = sctx_;
#if defined(ASIO_HAS_THREAD_SANITIZER_FIBERS)
    void* tsan_fiber = tsan_fiber_;
#endif // defined(ASIO_HAS_THREAD_SANITIZER_FIBERS)
#if defined(ASIO_HAS_VALGRIND)
    unsigned valgrind_stack_id = valgrind_stack_id_;
#endif // defined(ASIO_HAS_VALGRIND)
    this->~fiber_control_block();
#if defined(ASIO_HAS_VALGRIND)
    VALGRIND_STACK_DEREGISTER(valgrind_stack_id);
#endif // defined(ASIO_HAS_VALGRIND)
    alloc.deallocate(sctx);
#if defined(ASIO_HAS_THREAD_SANITIZER_FIBERS)
    if (tsan_fiber)
      __tsan_destroy_fiber(tsan_fiber);
#endif // defined(ASIO_HAS_THREAD_SANITIZER_FIBERS)
  }

private:
  stack_context_type sctx_;
  StackAllocator alloc_;
  Fn fn_;
};

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#if defined(ASIO_HEADER_ONLY)
# include "asio/detail/impl/asm_fiber.ipp"
#endif // defined(ASIO_HEADER_ONLY)

#endif // defined(ASIO_HAS_ASM_FIBER)

#endif // ASIO_DETAIL_ASM_FIBER_HPP
