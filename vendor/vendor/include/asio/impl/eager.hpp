//
// impl/eager.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_IMPL_EAGER_HPP
#define ASIO_IMPL_EAGER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include <atomic>
#include <cstdint>
#include "asio/associated_allocator.hpp"
#include "asio/associated_cancellation_slot.hpp"
#include "asio/associated_executor.hpp"
#include "asio/async_result.hpp"
#include "asio/cancellation_signal.hpp"
#include "asio/cancellation_type.hpp"
#include "asio/detached.hpp"
#include "asio/detail/assert.hpp"
#include "asio/detail/completion_payload.hpp"
#include "asio/detail/completion_payload_handler.hpp"
#include "asio/detail/eager_wait_handler.hpp"
#include "asio/detail/eager_wait_op.hpp"
#include "asio/detail/handler_tracking.hpp"
#include "asio/detail/memory.hpp"
#include "asio/detail/recycling_allocator.hpp"
#include "asio/detail/thread_info_base.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/detail/utility.hpp"
#include "asio/immediate.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN

template <typename Allocator>
struct eager_t;

template <typename Executor, typename... Signatures>
class basic_eager_async_operation;

namespace detail {

// Unwraps executors adapted by a completion token's executor_with_default.
template <typename Executor, typename = void>
struct eager_executor
{
  typedef Executor type;
};

template <typename Executor>
struct eager_executor<Executor,
    void_t<typename Executor::executor_without_default>>
  : eager_executor<typename Executor::executor_without_default>
{
};

template <typename Executor>
using eager_executor_t = typename eager_executor<Executor>::type;

template <typename Signature>
struct eager_decay_signature;

template <typename R, typename... Args>
struct eager_decay_signature<R(Args...)>
{
  using type = void(decay_t<Args>...);
};

template <typename... Signatures>
struct eager_payload
{
  using type = completion_payload<
      typename eager_decay_signature<Signatures>::type...>;
};

template <typename... Signatures>
class eager_impl
{
public:
  using payload_type = typename eager_payload<Signatures...>::type;

  using wait_op_type = eager_wait_op<payload_type>;

#if !defined(ASIO_HAS_STD_VARIANT)
  static_assert(sizeof...(Signatures) <= 2,
      "eager supports at most two completion signatures without std::variant");
#endif // !defined(ASIO_HAS_STD_VARIANT)

  eager_impl() = default;

  eager_impl(const eager_impl&) = delete;

  ~eager_impl()
  {
    wait_op_type* s = state_.load(std::memory_order_acquire);
    if (s == completion_sentinel())
      reinterpret_cast<payload_type*>(&result_)->~payload_type();
    else if (s != nullptr)
      s->destroy();
  }

  bool ready() const noexcept
  {
    return state_.load(std::memory_order_acquire) == completion_sentinel();
  }

  cancellation_slot get_cancellation_slot() noexcept
  {
    return cancel_.slot();
  }

  void emit_cancel(cancellation_type_t level)
  {
    cancel_.emit(level);
  }

  static wait_op_type* completion_sentinel() noexcept
  {
    return reinterpret_cast<wait_op_type*>(std::uintptr_t(1));
  }

  bool try_install_wait(wait_op_type* op)
  {
    wait_op_type* expected = nullptr;
    return state_.compare_exchange_strong(expected, op,
        std::memory_order_release, std::memory_order_acquire);
  }

  payload_type take_result()
  {
    payload_type* p = reinterpret_cast<payload_type*>(&result_);
    payload_type stored(static_cast<payload_type&&>(*p));
    p->~payload_type();
    state_.store(nullptr, std::memory_order_release);
    return stored;
  }

  void complete(detail::shared_ptr<eager_impl> self, payload_type payload)
  {
    wait_op_type* w = state_.load(std::memory_order_acquire);
    if (w == nullptr)
    {
      new (&result_) payload_type(static_cast<payload_type&&>(payload));
      if (state_.compare_exchange_strong(w, completion_sentinel(),
            std::memory_order_release, std::memory_order_acquire))
        return;
      payload_type stored = take_result();
      self.reset();
      w->complete(static_cast<payload_type&&>(stored));
      return;
    }
    state_.store(nullptr, std::memory_order_release);
    self.reset();
    w->complete(static_cast<payload_type&&>(payload));
  }

private:
  aligned_storage_t<sizeof(payload_type), alignof(payload_type)> result_;
  std::atomic<wait_op_type*> state_{nullptr};
  cancellation_signal cancel_;
};

template <typename Executor, typename Allocator, typename... Signatures>
struct eager_handler
{
  using impl_type = eager_impl<Signatures...>;
  using payload_type = typename impl_type::payload_type;
  using allocator_type = Allocator;
  using cancellation_slot_type = cancellation_slot;
  using executor_type = Executor;

  eager_handler(Allocator allocator, Executor executor)
    : impl_(detail::allocate_shared<impl_type>(
          get_recycling_allocator<Allocator,
            thread_info_base::default_tag>::get(allocator))),
      allocator_(static_cast<Allocator&&>(allocator)),
      executor_(static_cast<Executor&&>(executor))
  {
  }

  allocator_type get_allocator() const noexcept
  {
    return allocator_;
  }

  cancellation_slot_type get_cancellation_slot() const noexcept
  {
    return impl_->get_cancellation_slot();
  }

  const executor_type& get_executor() const noexcept
  {
    return executor_;
  }

  template <typename... Args>
  void operator()(Args&&... args)
  {
    ASIO_ASSERT(impl_);
    detail::shared_ptr<impl_type> impl(
        static_cast<detail::shared_ptr<impl_type>&&>(impl_));
    impl_type* raw = impl.get();
    raw->complete(static_cast<detail::shared_ptr<impl_type>&&>(impl),
        payload_type(completion_message<void(decay_t<Args>...)>(
          0, static_cast<Args&&>(args)...)));
  }

  detail::shared_ptr<impl_type> impl_;
  Allocator allocator_;
  Executor executor_;
};

template <typename ImplType>
struct eager_cancel_handler
{
  detail::weak_ptr<ImplType> self;

  eager_cancel_handler(detail::weak_ptr<ImplType> self)
    : self(static_cast<detail::weak_ptr<ImplType>&&>(self))
  {
  }

  void operator()(cancellation_type_t level) const
  {
    if (auto p = self.lock())
      p->emit_cancel(level);
  }
};

template <typename Executor, typename... Signatures>
class initiate_eager_wait
{
public:
  using executor_type = Executor;

  explicit initiate_eager_wait(const Executor& ex)
    : executor_(ex)
  {
  }

  const executor_type& get_executor() const noexcept
  {
    return executor_;
  }

  // Optimisation for asio::detached.
  void operator()(detached_handler,
      basic_eager_async_operation<Executor, Signatures...>&& eager_op) const
  {
    eager_op.release();
  }

  template <typename WaitHandler>
  void operator()(WaitHandler&& handler,
      basic_eager_async_operation<Executor, Signatures...>&& eager_op) const
  {
    impl_type* impl = eager_op.impl_.get();

    if (impl->ready())
    {
      detail::shared_ptr<impl_type> self = eager_op.release();
      auto exec = (get_associated_executor)(handler, executor_);
      (async_immediate)(exec,
          completion_payload_handler<payload_type, decay_t<WaitHandler>>(
            impl->take_result(), handler));
      return;
    }

    auto cancel = (get_associated_cancellation_slot)(handler);
    if (cancel.is_connected())
      cancel.template emplace<eager_cancel_handler<impl_type>>(eager_op.impl_);

    typedef eager_wait_handler<payload_type, decay_t<WaitHandler>, Executor> op;
    typename op::ptr p = { detail::addressof(handler),
        op::ptr::allocate(handler), 0 };
    p.p = new (p.v) op(static_cast<WaitHandler&&>(handler), executor_);

    ASIO_HANDLER_CREATION((*p.p, "eager", impl, 0, "async_wait"));

    detail::shared_ptr<impl_type> self = eager_op.release();

    if (impl->try_install_wait(p.p))
    {
      p.v = p.p = 0;
      return;
    }

    // The operation completed while we were installing the wait.
    payload_type payload = impl->take_result();
    op* wait_op = p.p;
    p.v = p.p = 0;
    wait_op->immediate(static_cast<payload_type&&>(payload), executor_);
  }

private:
  using impl_type = eager_impl<Signatures...>;
  using payload_type = typename impl_type::payload_type;

  Executor executor_;
};

} // namespace detail

template <typename Executor, typename... Signatures>
void basic_eager_async_operation<Executor, Signatures...>::cancel(
    cancellation_type_t level)
{
  if (impl_ && !impl_->ready())
    impl_->emit_cancel(level);
}

template <typename Executor, typename... Signatures>
bool basic_eager_async_operation<Executor, Signatures...>::ready()
  const noexcept
{
  return impl_ && impl_->ready();
}

#if !defined(GENERATING_DOCUMENTATION)

template <typename Allocator, typename... Signatures>
struct async_result<eager_t<Allocator>, Signatures...>
{
  template <typename Executor>
  static basic_eager_async_operation<Executor, Signatures...> initiate(
      detail::initiate_eager_wait<Executor, Signatures...>, eager_t<Allocator>,
      basic_eager_async_operation<Executor, Signatures...>&& op)
  {
    return static_cast<
        basic_eager_async_operation<Executor, Signatures...>&&>(op);
  }

  template <typename Initiation, typename... InitArgs>
  static auto initiate(Initiation initiation,
      eager_t<Allocator> token, InitArgs... args)
    -> basic_eager_async_operation<
          detail::eager_executor_t<associated_executor_t<Initiation>>,
          Signatures...>
  {
    static_assert(
        detail::has_associated_executor<Initiation, inline_executor>::value,
        "eager requires an operation with an associated I/O executor");

    using executor_type =
      detail::eager_executor_t<associated_executor_t<Initiation>>;
    using handler_type = detail::eager_handler<
      executor_type, Allocator, Signatures...>;

    executor_type executor = (get_associated_executor)(initiation);
    handler_type handler{token.get_allocator(), executor};
    detail::shared_ptr<typename handler_type::impl_type> impl = handler.impl_;

    static_cast<Initiation&&>(initiation)(
        handler, static_cast<InitArgs&&>(args)...);

    return {detail::eager_init_tag{},
        static_cast<detail::shared_ptr<
          typename handler_type::impl_type>&&>(impl),
        static_cast<executor_type&&>(executor)};
  }
};

#endif // !defined(GENERATING_DOCUMENTATION)

ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_IMPL_EAGER_HPP
