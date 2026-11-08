//
// detail/impl/asm_fiber.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_IMPL_ASM_FIBER_IPP
#define ASIO_DETAIL_IMPL_ASM_FIBER_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_ASM_FIBER)

#include "asio/detail/asm_fiber.hpp"

// A context block is stored at the top of the stack it belongs to. It holds the
// registers that the ABI requires each function to preserve, and the address to
// resume at, which sits highest so that a ret uncovers it (an indirect branch
// would need a landing pad, at what is a return address in compiled C++):
//
//        x86-64, 0x40                    AArch64, 0xb0
//   0x38 | resume address |         0xa0 | resume address |
//   0x30 | rbp            |         0x90 | x29, x30       |
//   0x28 | rbx            |         0x80 | x27, x28       |
//   0x20 | r15            |         0x70 | x25, x26       |
//   0x18 | r14            |         0x60 | x23, x24       |
//   0x10 | r13            |         0x50 | x21, x22       |
//   0x08 | r12            |         0x40 | x19, x20       |
//   0x00 | unused         |         0x30 | d14, d15       |
//                                   0x20 | d12, d13       |
//                                   0x10 | d10, d11       |
//                                   0x00 | d8,  d9        |

#if defined(__clang__) && (__clang_major__ < 17)
# define ASIO_DETAIL_ASM_CFI(directive)
#else // defined(__clang__) && (__clang_major__ < 17)
# define ASIO_DETAIL_ASM_CFI(directive) directive
#endif // defined(__clang__) && (__clang_major__ < 17)

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

// The definitions sit inside a function so that just one copy of them survives.
// It is marked noinline to prevent a copy from being generated per caller.
#if defined(__GNUC__) && !defined(__clang__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wattributes"
#endif // defined(__GNUC__) && !defined(__clang__)
ASIO_NEVER_INLINE void asm_fiber_definitions()
{
#if defined(__APPLE__)

# if defined(__x86_64__)

  __asm__(
  ".pushsection __TEXT,__asio_fctx,regular,pure_instructions\n"

  // fiber_context make_fiber_context(void* sp,
  //     std::size_t size, void (*fn)(fiber_context))
  ".globl " ASIO_DETAIL_ASM_SYMBOL(make_fiber_context) "\n"
  ".weak_definition " ASIO_DETAIL_ASM_SYMBOL(make_fiber_context) "\n"
  ".p2align 4, 0x90\n"
  ASIO_DETAIL_ASM_SYMBOL(make_fiber_context) ":\n"
  "    movq %rdi, %rax\n" // rax = requested stack top
  "    andq $-16, %rax\n" // align the top down to 16 bytes
  "    leaq -0x40(%rax), %rax\n" // make space for the context block
  "    movq %rdx, 0x28(%rax)\n" // rbx slot <- fn, restored on entry
  "    xorl %ecx, %ecx\n"
  "    movq %rcx, 0x08(%rax)\n"
  "    movq %rcx, 0x10(%rax)\n"
  "    movq %rcx, 0x18(%rax)\n"
  "    movq %rcx, 0x20(%rax)\n"
  "    movq %rcx, 0x30(%rax)\n" // rbp slot <- 0, terminates the frame
  "    leaq " ASIO_DETAIL_ASM_LABEL(fiber_context_entry) "(%rip), %rcx\n"
  "    movq %rcx, 0x38(%rax)\n" // resume address <- trampoline
  "    ret\n"

  // Reached by the ret at the end of swap_fiber_context, with the fiber_context
  // still in rax:rdx and fn in rbx. rbp arrives zeroed, so a backtrace
  // terminates. Enters fn with a call, so that it has a return address.
  ASIO_DETAIL_ASM_LABEL(fiber_context_entry) ":\n"
  "    movq %rax, %rdi\n"
  "    movq %rdx, %rsi\n"
  "    callq *%rbx\n"
  "    ud2\n" // fn is noexcept and never returns

  // fiber_context swap_fiber_context(fiber_context to)
  ".globl " ASIO_DETAIL_ASM_SYMBOL(swap_fiber_context) "\n"
  ".weak_definition " ASIO_DETAIL_ASM_SYMBOL(swap_fiber_context) "\n"
  ".p2align 4, 0x90\n"
  ASIO_DETAIL_ASM_SYMBOL(swap_fiber_context) ":\n"
  "    leaq -0x38(%rsp), %rsp\n" // our return address now lies at 0x38
  "    movq %r12, 0x08(%rsp)\n"
  "    movq %r13, 0x10(%rsp)\n"
  "    movq %r14, 0x18(%rsp)\n"
  "    movq %r15, 0x20(%rsp)\n"
  "    movq %rbx, 0x28(%rsp)\n"
  "    movq %rbp, 0x30(%rsp)\n"
  "    movq %rsp, %rax\n" // .sp = what we just saved
  "    movq %rdi, %rsp\n" // switch to the target block
  "    movq 0x08(%rsp), %r12\n"
  "    movq 0x10(%rsp), %r13\n"
  "    movq 0x18(%rsp), %r14\n"
  "    movq 0x20(%rsp), %r15\n"
  "    movq 0x28(%rsp), %rbx\n"
  "    movq 0x30(%rsp), %rbp\n"
  "    movq %rsi, %rdx\n" // fiber_context.data
  "    leaq 0x38(%rsp), %rsp\n" // uncover the target's resume address
  "    ret\n"
  ".popsection\n"
  );

# elif defined(__aarch64__)

  __asm__(
  ".pushsection __TEXT,__asio_fctx,regular,pure_instructions\n"

  // fiber_context make_fiber_context(void* sp,
  //     std::size_t size, void (*fn)(fiber_context))
  ".globl " ASIO_DETAIL_ASM_SYMBOL(make_fiber_context) "\n"
  ".weak_definition " ASIO_DETAIL_ASM_SYMBOL(make_fiber_context) "\n"
  ".p2align 2\n"
  ASIO_DETAIL_ASM_SYMBOL(make_fiber_context) ":\n"
  "    and x0, x0, #-16\n" // align the top down to 16 bytes
  "    sub x0, x0, #0xb0\n" // make space for the context block
  "    stp xzr, xzr, [x0, #0x00]\n"
  "    stp xzr, xzr, [x0, #0x10]\n"
  "    stp xzr, xzr, [x0, #0x20]\n"
  "    stp xzr, xzr, [x0, #0x30]\n"
  "    str x2,       [x0, #0x40]\n" // x19 <- fn, restored on entry
  "    stp xzr, xzr, [x0, #0x48]\n"
  "    stp xzr, xzr, [x0, #0x58]\n"
  "    stp xzr, xzr, [x0, #0x68]\n"
  "    stp xzr, xzr, [x0, #0x78]\n"
  "    stp xzr, xzr, [x0, #0x88]\n"
  "    str xzr,      [x0, #0x98]\n" // x29 <- 0, ending the frame chain
  "    adr x3, " ASIO_DETAIL_ASM_LABEL(fiber_context_entry) "\n"
  "    str x3,       [x0, #0xa0]\n" // resume address <- trampoline
  "    ret\n"

  // Reached by the ret at the end of swap_fiber_context, with the fiber_context
  // still in x0:x1 and fn in x19. x29 and x30 arrive zeroed, so a backtrace
  // terminates. blr rather than br, so that fn has a return address.
  ASIO_DETAIL_ASM_LABEL(fiber_context_entry) ":\n"
  "    blr  x19\n"
  "    brk  #0\n" // fn is noexcept and never returns

  // fiber_context swap_fiber_context(fiber_context to)
  ".globl " ASIO_DETAIL_ASM_SYMBOL(swap_fiber_context) "\n"
  ".weak_definition " ASIO_DETAIL_ASM_SYMBOL(swap_fiber_context) "\n"
  ".p2align 2\n"
  ASIO_DETAIL_ASM_SYMBOL(swap_fiber_context) ":\n"
  "    sub sp, sp, #0xb0\n"
  "    stp d8,  d9,  [sp, #0x00]\n"
  "    stp d10, d11, [sp, #0x10]\n"
  "    stp d12, d13, [sp, #0x20]\n"
  "    stp d14, d15, [sp, #0x30]\n"
  "    stp x19, x20, [sp, #0x40]\n"
  "    stp x21, x22, [sp, #0x50]\n"
  "    stp x23, x24, [sp, #0x60]\n"
  "    stp x25, x26, [sp, #0x70]\n"
  "    stp x27, x28, [sp, #0x80]\n"
  "    stp x29, x30, [sp, #0x90]\n"
  "    str x30,      [sp, #0xa0]\n" // resume address = our return address
  "    mov x2, sp\n" // .sp = what we just saved
  "    mov sp, x0\n" // switch to the target block
  "    ldp d8,  d9,  [sp, #0x00]\n"
  "    ldp d10, d11, [sp, #0x10]\n"
  "    ldp d12, d13, [sp, #0x20]\n"
  "    ldp d14, d15, [sp, #0x30]\n"
  "    ldp x19, x20, [sp, #0x40]\n"
  "    ldp x21, x22, [sp, #0x50]\n"
  "    ldp x23, x24, [sp, #0x60]\n"
  "    ldp x25, x26, [sp, #0x70]\n"
  "    ldp x27, x28, [sp, #0x80]\n"
  "    ldp x29, x30, [sp, #0x90]\n"
  "    ldr x3,       [sp, #0xa0]\n" // the target's resume address
  "    add sp, sp, #0xb0\n"
  "    mov x0, x2\n" // .sp; x1 is already .data
  "    ret x3\n"
  ".popsection\n"
  );

# endif // defined(__aarch64__)

#else // defined(__APPLE__)

# if defined(__x86_64__)

  __asm__(

  // fiber_context make_fiber_context(void* sp,
  //     std::size_t size, void (*fn)(fiber_context))
  ".pushsection .text." ASIO_DETAIL_ASM_SYMBOL(make_fiber_context)
      ",\"axG\",@progbits,"
      ASIO_DETAIL_ASM_SYMBOL(make_fiber_context) ",comdat\n"
  ".globl " ASIO_DETAIL_ASM_SYMBOL(make_fiber_context) "\n"
  ".type " ASIO_DETAIL_ASM_SYMBOL(make_fiber_context) ",@function\n"
  ".p2align 4, 0x90\n"
  ASIO_DETAIL_ASM_SYMBOL(make_fiber_context) ":\n"
  ASIO_DETAIL_ASM_CFI(".cfi_startproc\n")
#  if defined(__CET__)
  "    endbr64\n"
#  endif // defined(__CET__)
  "    movq %rdi, %rax\n" // rax = requested stack top
  "    andq $-16, %rax\n" // align the top down to 16 bytes
  "    leaq -0x40(%rax), %rax\n" // make space for the context block
  "    movq %rdx, 0x28(%rax)\n" // rbx slot <- fn, restored on entry
  "    xorl %ecx, %ecx\n"
  "    movq %rcx, 0x08(%rax)\n"
  "    movq %rcx, 0x10(%rax)\n"
  "    movq %rcx, 0x18(%rax)\n"
  "    movq %rcx, 0x20(%rax)\n"
  "    movq %rcx, 0x30(%rax)\n" // rbp slot <- 0, terminates the frame
  "    leaq " ASIO_DETAIL_ASM_LABEL(fiber_context_entry) "(%rip), %rcx\n"
  "    movq %rcx, 0x38(%rax)\n" // resume address <- trampoline
  "    ret\n"

  // Reached by the ret at the end of swap_fiber_context, with the fiber_context
  // still in rax:rdx and fn in rbx. rbp arrives zeroed, so a backtrace
  // terminates. Enters fn with a call, so that it has a return address.
  ASIO_DETAIL_ASM_LABEL(fiber_context_entry) ":\n"
  ASIO_DETAIL_ASM_CFI(".cfi_undefined rip\n")
  "    movq %rax, %rdi\n"
  "    movq %rdx, %rsi\n"
  "    callq *%rbx\n"
  "    ud2\n" // fn is noexcept and never returns
  ASIO_DETAIL_ASM_CFI(".cfi_endproc\n")
  ".size " ASIO_DETAIL_ASM_SYMBOL(make_fiber_context)
      ",.-" ASIO_DETAIL_ASM_SYMBOL(make_fiber_context) "\n"

  // fiber_context swap_fiber_context(fiber_context to)
  ".section .text." ASIO_DETAIL_ASM_SYMBOL(swap_fiber_context)
      ",\"axG\",@progbits,"
      ASIO_DETAIL_ASM_SYMBOL(swap_fiber_context) ",comdat\n"
  ".globl " ASIO_DETAIL_ASM_SYMBOL(swap_fiber_context) "\n"
  ".type " ASIO_DETAIL_ASM_SYMBOL(swap_fiber_context) ",@function\n"
  ".p2align 4, 0x90\n"
  ASIO_DETAIL_ASM_SYMBOL(swap_fiber_context) ":\n"
  ASIO_DETAIL_ASM_CFI(".cfi_startproc\n")
#  if defined(__CET__)
  "    endbr64\n"
#  endif // defined(__CET__)
  "    leaq -0x38(%rsp), %rsp\n" // our return address now lies at 0x38
  "    movq %r12, 0x08(%rsp)\n"
  "    movq %r13, 0x10(%rsp)\n"
  "    movq %r14, 0x18(%rsp)\n"
  "    movq %r15, 0x20(%rsp)\n"
  "    movq %rbx, 0x28(%rsp)\n"
  "    movq %rbp, 0x30(%rsp)\n"
  "    movq %rsp, %rax\n" // .sp = what we just saved
  "    movq %rdi, %rsp\n" // switch to the target block
  "    movq 0x08(%rsp), %r12\n"
  "    movq 0x10(%rsp), %r13\n"
  "    movq 0x18(%rsp), %r14\n"
  "    movq 0x20(%rsp), %r15\n"
  "    movq 0x28(%rsp), %rbx\n"
  "    movq 0x30(%rsp), %rbp\n"
  "    movq %rsi, %rdx\n" // fiber_context.data
  "    leaq 0x38(%rsp), %rsp\n" // uncover the target's resume address
  "    ret\n"
  ASIO_DETAIL_ASM_CFI(".cfi_endproc\n")
  ".size " ASIO_DETAIL_ASM_SYMBOL(swap_fiber_context)
      ",.-" ASIO_DETAIL_ASM_SYMBOL(swap_fiber_context) "\n"
  ".popsection\n"
  );

# elif defined(__aarch64__)

  __asm__(

  // fiber_context make_fiber_context(void* sp,
  //     std::size_t size, //     void (*fn)(fiber_context))
  ".pushsection .text." ASIO_DETAIL_ASM_SYMBOL(make_fiber_context)
      ",\"axG\",@progbits,"
      ASIO_DETAIL_ASM_SYMBOL(make_fiber_context) ",comdat\n"
  ".globl " ASIO_DETAIL_ASM_SYMBOL(make_fiber_context) "\n"
  ".type " ASIO_DETAIL_ASM_SYMBOL(make_fiber_context) ",@function\n"
  ".p2align 2\n"
  ASIO_DETAIL_ASM_SYMBOL(make_fiber_context) ":\n"
  ASIO_DETAIL_ASM_CFI(".cfi_startproc\n")
  "    hint #34\n" // bti c
  "    and x0, x0, #-16\n" // align the top down to 16 bytes
  "    sub x0, x0, #0xb0\n" // make space for the context block
  "    stp xzr, xzr, [x0, #0x00]\n"
  "    stp xzr, xzr, [x0, #0x10]\n"
  "    stp xzr, xzr, [x0, #0x20]\n"
  "    stp xzr, xzr, [x0, #0x30]\n"
  "    str x2,       [x0, #0x40]\n" // x19 <- fn, restored on entry
  "    stp xzr, xzr, [x0, #0x48]\n"
  "    stp xzr, xzr, [x0, #0x58]\n"
  "    stp xzr, xzr, [x0, #0x68]\n"
  "    stp xzr, xzr, [x0, #0x78]\n"
  "    stp xzr, xzr, [x0, #0x88]\n"
  "    str xzr,      [x0, #0x98]\n" // x29 <- 0, ending the frame chain
  "    adr x3, " ASIO_DETAIL_ASM_LABEL(fiber_context_entry) "\n"
  "    str x3,       [x0, #0xa0]\n" // resume address <- trampoline
  "    ret\n"

  // Reached by the ret at the end of swap_fiber_context, with the fiber_context
  // still in x0:x1 and fn in x19. A ret leaves PSTATE.BTYPE clear, so no
  // landing pad is needed even on a guarded page. x29 and x30 arrive zeroed, so
  // a backtrace terminates. Uses blr rather than br, so that fn has a return
  // address.
  ASIO_DETAIL_ASM_LABEL(fiber_context_entry) ":\n"
  ASIO_DETAIL_ASM_CFI(".cfi_undefined 30\n")
  "    blr  x19\n"
  "    brk  #0\n" // fn is noexcept and never returns
  ASIO_DETAIL_ASM_CFI(".cfi_endproc\n")
  ".size " ASIO_DETAIL_ASM_SYMBOL(make_fiber_context)
      ",.-" ASIO_DETAIL_ASM_SYMBOL(make_fiber_context) "\n"

  // fiber_context swap_fiber_context(fiber_context to)
  ".section .text." ASIO_DETAIL_ASM_SYMBOL(swap_fiber_context)
      ",\"axG\",@progbits,"
      ASIO_DETAIL_ASM_SYMBOL(swap_fiber_context) ",comdat\n"
  ".globl " ASIO_DETAIL_ASM_SYMBOL(swap_fiber_context) "\n"
  ".type " ASIO_DETAIL_ASM_SYMBOL(swap_fiber_context) ",@function\n"
  ".p2align 2\n"
  ASIO_DETAIL_ASM_SYMBOL(swap_fiber_context) ":\n"
  ASIO_DETAIL_ASM_CFI(".cfi_startproc\n")
  "    hint #34\n" // bti c
  "    sub sp, sp, #0xb0\n"
  "    stp d8,  d9,  [sp, #0x00]\n"
  "    stp d10, d11, [sp, #0x10]\n"
  "    stp d12, d13, [sp, #0x20]\n"
  "    stp d14, d15, [sp, #0x30]\n"
  "    stp x19, x20, [sp, #0x40]\n"
  "    stp x21, x22, [sp, #0x50]\n"
  "    stp x23, x24, [sp, #0x60]\n"
  "    stp x25, x26, [sp, #0x70]\n"
  "    stp x27, x28, [sp, #0x80]\n"
  "    stp x29, x30, [sp, #0x90]\n"
  "    str x30,      [sp, #0xa0]\n" // resume address = our return address
  "    mov x2, sp\n" // .sp = what we just saved
  "    mov sp, x0\n" // switch to the target block
  "    ldp d8,  d9,  [sp, #0x00]\n"
  "    ldp d10, d11, [sp, #0x10]\n"
  "    ldp d12, d13, [sp, #0x20]\n"
  "    ldp d14, d15, [sp, #0x30]\n"
  "    ldp x19, x20, [sp, #0x40]\n"
  "    ldp x21, x22, [sp, #0x50]\n"
  "    ldp x23, x24, [sp, #0x60]\n"
  "    ldp x25, x26, [sp, #0x70]\n"
  "    ldp x27, x28, [sp, #0x80]\n"
  "    ldp x29, x30, [sp, #0x90]\n"
  "    ldr x3,       [sp, #0xa0]\n" // the target's resume address
  "    add sp, sp, #0xb0\n"
  "    mov x0, x2\n" // .sp; x1 is already .data
  "    ret x3\n"
  ASIO_DETAIL_ASM_CFI(".cfi_endproc\n")
  ".size " ASIO_DETAIL_ASM_SYMBOL(swap_fiber_context)
      ",.-" ASIO_DETAIL_ASM_SYMBOL(swap_fiber_context) "\n"
  ".popsection\n"
  );

# endif // defined(__aarch64__)

#endif // defined(__APPLE__)
}

#if defined(__GNUC__) && !defined(__clang__)
# pragma GCC diagnostic pop
#endif // defined(__GNUC__) && !defined(__clang__)

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#undef ASIO_DETAIL_ASM_CFI

#endif // defined(ASIO_HAS_ASM_FIBER)

#endif // ASIO_DETAIL_IMPL_ASM_FIBER_IPP
