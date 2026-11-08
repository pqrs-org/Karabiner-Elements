//
// detail/triggered_cancel_op.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_TRIGGERED_CANCEL_OP_HPP
#define ASIO_DETAIL_TRIGGERED_CANCEL_OP_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/associated_cancellation_slot.hpp"
#include "asio/associator.hpp"
#include "asio/cancellation_signal.hpp"
#include "asio/detail/atomic_count.hpp"
#include "asio/detail/completion_payload.hpp"
#include "asio/detail/completion_payload_handler.hpp"
#include "asio/detail/handler_alloc_helpers.hpp"
#include "asio/detail/type_traits.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

template <typename Op, typename... Signatures>
class triggered_cancel_op_handler;

template <typename Op>
class triggered_cancel_trigger_handler;

template <typename Handler, typename Trigger, typename... Signatures>
class triggered_cancel_op
{
public:
  using handler_type = Handler;

  ASIO_DEFINE_TAGGED_HANDLER_PTR(
      thread_info_base::timed_cancel_tag, triggered_cancel_op);

  triggered_cancel_op(Handler& handler, Trigger trigger,
      cancellation_type_t cancel_type)
    : ref_count_(2),
      handler_(static_cast<Handler&&>(handler)),
      trigger_(static_cast<Trigger&&>(trigger)),
      cancellation_type_(cancel_type),
      cancel_proxy_(nullptr),
      has_payload_(false),
      has_pending_trigger_(true)
  {
  }

  ~triggered_cancel_op()
  {
    if (has_payload_)
      payload_storage_.payload_.~payload_type();
  }

  cancellation_slot get_cancellation_slot() noexcept
  {
    return cancellation_signal_.slot();
  }

  cancellation_slot get_trigger_cancellation_slot() noexcept
  {
    return trigger_cancellation_signal_.slot();
  }

  template <typename Initiation, typename... Args>
  void start(Initiation&& initiation, Args&&... args)
  {
    using op_handler_type =
      triggered_cancel_op_handler<triggered_cancel_op, Signatures...>;
    op_handler_type op_handler(this);

    using trigger_handler_type =
      triggered_cancel_trigger_handler<triggered_cancel_op>;
    trigger_handler_type trigger_handler(this);

    associated_cancellation_slot_t<Handler> slot
      = (get_associated_cancellation_slot)(handler_);
    if (slot.is_connected())
      cancel_proxy_ = &slot.template emplace<cancel_proxy>(this);

    static_cast<Trigger&&>(trigger_)(
        static_cast<trigger_handler_type&&>(trigger_handler));

#if !defined(ASIO_NO_EXCEPTIONS)
    try
    {
#endif // !defined(ASIO_NO_EXCEPTIONS)
      async_initiate<op_handler_type, Signatures...>(
          static_cast<Initiation&&>(initiation),
          static_cast<op_handler_type&>(op_handler),
          static_cast<Args&&>(args)...);
#if !defined(ASIO_NO_EXCEPTIONS)
    }
    catch (...)
    {
      has_pending_trigger_ = false;
      throw;
    }
#endif // !defined(ASIO_NO_EXCEPTIONS)
  }

  template <typename Message>
  void handle_op(Message&& message)
  {
    if (cancel_proxy_)
      cancel_proxy_->op_ = nullptr;

    new (&payload_storage_.payload_) payload_type(
        static_cast<Message&&>(message));
    has_payload_ = true;

    if (has_pending_trigger_)
    {
      trigger_cancellation_signal_.emit(cancellation_type::terminal);
      complete();
    }
    else
    {
      complete();
    }
  }

  void handle_trigger()
  {
    has_pending_trigger_ = false;

    if (has_payload_)
    {
      complete();
    }
    else
    {
      cancellation_signal_.emit(cancellation_type_);
      if (has_payload_)
        complete();
      else
        release();
    }
  }

  void release()
  {
    if (--ref_count_ == 0)
    {
      ptr p = { asio::detail::addressof(handler_), this, this };
      Handler handler(static_cast<Handler&&>(handler_));
      p.h = asio::detail::addressof(handler);
      p.reset();
    }
  }

  void complete()
  {
    if (--ref_count_ == 0)
    {
      ptr p = { asio::detail::addressof(handler_), this, this };
      completion_payload_handler<payload_type, Handler> handler(
          static_cast<payload_type&&>(payload_storage_.payload_), handler_);
      p.h = asio::detail::addressof(handler.handler());
      p.reset();
      handler();
    }
  }

//private:
  typedef completion_payload<Signatures...> payload_type;

  struct cancel_proxy
  {
    cancel_proxy(triggered_cancel_op* op)
      : op_(op)
    {
    }

    void operator()(cancellation_type_t type)
    {
      if (op_)
        op_->cancellation_signal_.emit(type);
    }

    triggered_cancel_op* op_;
  };

  // The number of handlers that share a reference to the state.
  atomic_count ref_count_;

  // The handler to be called when the operation completes.
  Handler handler_;

  // The trigger used to determine when to cancel the pending operation.
  Trigger trigger_;

  // The cancellation signal and type used to cancel the pending operation.
  cancellation_signal cancellation_signal_;
  cancellation_type_t cancellation_type_;

  // The cancellation signal used to cancel the trigger operation.
  cancellation_signal trigger_cancellation_signal_;

  // A proxy cancel handler used to allow cancellation of the operation.
  cancel_proxy* cancel_proxy_;

  // Arguments to be passed to the completion handler.
  union payload_storage
  {
    payload_storage() {}
    ~payload_storage() {}

    char dummy_;
    payload_type payload_;
  } payload_storage_;

  // Whether the payload storage contains a valid payload.
  bool has_payload_;

  // Whether the asynchronous trigger is still pending
  bool has_pending_trigger_;
};

template <typename Op, typename R, typename... Args>
class triggered_cancel_op_handler<Op, R(Args...)>
{
public:
  using cancellation_slot_type = cancellation_slot;

  explicit triggered_cancel_op_handler(Op* op)
    : op_(op)
  {
  }

  triggered_cancel_op_handler(triggered_cancel_op_handler&& other) noexcept
    : op_(other.op_)
  {
    other.op_ = nullptr;
  }

  ~triggered_cancel_op_handler()
  {
    if (op_)
      op_->release();
  }

  cancellation_slot_type get_cancellation_slot() const noexcept
  {
    return op_->get_cancellation_slot();
  }

  template <typename... Args2>
  enable_if_t<
    is_constructible<completion_message<R(Args...)>, int, Args2...>::value
  > operator()(Args2&&... args)
  {
    Op* op = op_;
    op_ = nullptr;
    typedef completion_message<R(Args...)> message_type;
    op->handle_op(message_type(0, static_cast<Args2&&>(args)...));
  }

//protected:
  Op* op_;
};

template <typename Op, typename R, typename... Args, typename... Signatures>
class triggered_cancel_op_handler<Op, R(Args...), Signatures...> :
  public triggered_cancel_op_handler<Op, Signatures...>
{
public:
  using triggered_cancel_op_handler<
      Op, Signatures...>::triggered_cancel_op_handler;
  using triggered_cancel_op_handler<
      Op, Signatures...>::operator();

  template <typename... Args2>
  enable_if_t<
    is_constructible<completion_message<R(Args...)>, int, Args2...>::value
  > operator()(Args2&&... args)
  {
    Op* op = this->op_;
    this->op_ = nullptr;
    typedef completion_message<R(Args...)> message_type;
    op->handle_op(message_type(0, static_cast<Args2&&>(args)...));
  }
};

template <typename Op>
class triggered_cancel_trigger_handler
{
public:
  using cancellation_slot_type = cancellation_slot;

  explicit triggered_cancel_trigger_handler(Op* op)
    : op_(op)
  {
  }

  triggered_cancel_trigger_handler(
      triggered_cancel_trigger_handler&& other) noexcept
    : op_(other.op_)
  {
    other.op_ = nullptr;
  }

  ~triggered_cancel_trigger_handler()
  {
    if (op_)
      op_->release();
  }

  cancellation_slot_type get_cancellation_slot() const noexcept
  {
    return op_->get_trigger_cancellation_slot();
  }

  template <typename... Args>
  void operator()(Args&&...)
  {
    Op* op = op_;
    op_ = nullptr;
    op->handle_trigger();
  }

//private:
  Op* op_;
};

} // namespace detail

template <template <typename, typename> class Associator,
    typename Op, typename... Signatures, typename DefaultCandidate>
struct associator<Associator,
    detail::triggered_cancel_op_handler<Op, Signatures...>, DefaultCandidate>
  : Associator<typename Op::handler_type, DefaultCandidate>
{
  static typename Associator<typename Op::handler_type, DefaultCandidate>::type
  get(const detail::triggered_cancel_op_handler<Op, Signatures...>& h) noexcept
  {
    return Associator<typename Op::handler_type, DefaultCandidate>::get(
        h.op_->handler_);
  }

  static auto get(
      const detail::triggered_cancel_op_handler<Op, Signatures...>& h,
      const DefaultCandidate& c) noexcept
    -> decltype(Associator<typename Op::handler_type, DefaultCandidate>::get(
        h.op_->handler_, c))
  {
    return Associator<typename Op::handler_type, DefaultCandidate>::get(
        h.op_->handler_, c);
  }
};

template <template <typename, typename> class Associator,
    typename Op, typename DefaultCandidate>
struct associator<Associator,
    detail::triggered_cancel_trigger_handler<Op>, DefaultCandidate>
  : Associator<typename Op::handler_type, DefaultCandidate>
{
  static typename Associator<typename Op::handler_type, DefaultCandidate>::type
  get(const detail::triggered_cancel_trigger_handler<Op>& h) noexcept
  {
    return Associator<typename Op::handler_type, DefaultCandidate>::get(
        h.op_->handler_);
  }

  static auto get(const detail::triggered_cancel_trigger_handler<Op>& h,
      const DefaultCandidate& c) noexcept
    -> decltype(Associator<typename Op::handler_type, DefaultCandidate>::get(
        h.op_->handler_, c))
  {
    return Associator<typename Op::handler_type, DefaultCandidate>::get(
        h.op_->handler_, c);
  }
};

template <typename Op, typename DefaultCandidate>
struct associator<associated_immediate_executor,
    detail::triggered_cancel_trigger_handler<Op>, DefaultCandidate>
{
  using type = DefaultCandidate;

  static DefaultCandidate get(
      const detail::triggered_cancel_trigger_handler<Op>&) noexcept
  {
    return DefaultCandidate();
  }

  static const DefaultCandidate& get(
      const detail::triggered_cancel_trigger_handler<Op>&,
      const DefaultCandidate& c) noexcept
  {
    return c;
  }
};

ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_TRIGGERED_CANCEL_OP_HPP
