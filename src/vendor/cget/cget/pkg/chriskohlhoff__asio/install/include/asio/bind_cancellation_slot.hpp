//
// bind_cancellation_slot.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2023 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_BIND_CANCELLATION_SLOT_HPP
#define ASIO_BIND_CANCELLATION_SLOT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/associated_cancellation_slot.hpp"
#include "asio/associator.hpp"
#include "asio/async_result.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

// Helper to automatically define nested typedef result_type.

template <typename T, typename = void>
struct cancellation_slot_binder_result_type
{
protected:
  typedef void result_type_or_void;
};

template <typename T>
struct cancellation_slot_binder_result_type<T, void_t<typename T::result_type>>
{
  typedef typename T::result_type result_type;
protected:
  typedef result_type result_type_or_void;
};

template <typename R>
struct cancellation_slot_binder_result_type<R(*)()>
{
  typedef R result_type;
protected:
  typedef result_type result_type_or_void;
};

template <typename R>
struct cancellation_slot_binder_result_type<R(&)()>
{
  typedef R result_type;
protected:
  typedef result_type result_type_or_void;
};

template <typename R, typename A1>
struct cancellation_slot_binder_result_type<R(*)(A1)>
{
  typedef R result_type;
protected:
  typedef result_type result_type_or_void;
};

template <typename R, typename A1>
struct cancellation_slot_binder_result_type<R(&)(A1)>
{
  typedef R result_type;
protected:
  typedef result_type result_type_or_void;
};

template <typename R, typename A1, typename A2>
struct cancellation_slot_binder_result_type<R(*)(A1, A2)>
{
  typedef R result_type;
protected:
  typedef result_type result_type_or_void;
};

template <typename R, typename A1, typename A2>
struct cancellation_slot_binder_result_type<R(&)(A1, A2)>
{
  typedef R result_type;
protected:
  typedef result_type result_type_or_void;
};

// Helper to automatically define nested typedef argument_type.

template <typename T, typename = void>
struct cancellation_slot_binder_argument_type {};

template <typename T>
struct cancellation_slot_binder_argument_type<T,
    void_t<typename T::argument_type>>
{
  typedef typename T::argument_type argument_type;
};

template <typename R, typename A1>
struct cancellation_slot_binder_argument_type<R(*)(A1)>
{
  typedef A1 argument_type;
};

template <typename R, typename A1>
struct cancellation_slot_binder_argument_type<R(&)(A1)>
{
  typedef A1 argument_type;
};

// Helper to automatically define nested typedefs first_argument_type and
// second_argument_type.

template <typename T, typename = void>
struct cancellation_slot_binder_argument_types {};

template <typename T>
struct cancellation_slot_binder_argument_types<T,
    void_t<typename T::first_argument_type>>
{
  typedef typename T::first_argument_type first_argument_type;
  typedef typename T::second_argument_type second_argument_type;
};

template <typename R, typename A1, typename A2>
struct cancellation_slot_binder_argument_type<R(*)(A1, A2)>
{
  typedef A1 first_argument_type;
  typedef A2 second_argument_type;
};

template <typename R, typename A1, typename A2>
struct cancellation_slot_binder_argument_type<R(&)(A1, A2)>
{
  typedef A1 first_argument_type;
  typedef A2 second_argument_type;
};

} // namespace detail

/// A call wrapper type to bind a cancellation slot of type @c CancellationSlot
/// to an object of type @c T.
template <typename T, typename CancellationSlot>
class cancellation_slot_binder
#if !defined(GENERATING_DOCUMENTATION)
  : public detail::cancellation_slot_binder_result_type<T>,
    public detail::cancellation_slot_binder_argument_type<T>,
    public detail::cancellation_slot_binder_argument_types<T>
#endif // !defined(GENERATING_DOCUMENTATION)
{
public:
  /// The type of the target object.
  typedef T target_type;

  /// The type of the associated cancellation slot.
  typedef CancellationSlot cancellation_slot_type;

#if defined(GENERATING_DOCUMENTATION)
  /// The return type if a function.
  /**
   * The type of @c result_type is based on the type @c T of the wrapper's
   * target object:
   *
   * @li if @c T is a pointer to function type, @c result_type is a synonym for
   * the return type of @c T;
   *
   * @li if @c T is a class type with a member type @c result_type, then @c
   * result_type is a synonym for @c T::result_type;
   *
   * @li otherwise @c result_type is not defined.
   */
  typedef see_below result_type;

  /// The type of the function's argument.
  /**
   * The type of @c argument_type is based on the type @c T of the wrapper's
   * target object:
   *
   * @li if @c T is a pointer to a function type accepting a single argument,
   * @c argument_type is a synonym for the return type of @c T;
   *
   * @li if @c T is a class type with a member type @c argument_type, then @c
   * argument_type is a synonym for @c T::argument_type;
   *
   * @li otherwise @c argument_type is not defined.
   */
  typedef see_below argument_type;

  /// The type of the function's first argument.
  /**
   * The type of @c first_argument_type is based on the type @c T of the
   * wrapper's target object:
   *
   * @li if @c T is a pointer to a function type accepting two arguments, @c
   * first_argument_type is a synonym for the return type of @c T;
   *
   * @li if @c T is a class type with a member type @c first_argument_type,
   * then @c first_argument_type is a synonym for @c T::first_argument_type;
   *
   * @li otherwise @c first_argument_type is not defined.
   */
  typedef see_below first_argument_type;

  /// The type of the function's second argument.
  /**
   * The type of @c second_argument_type is based on the type @c T of the
   * wrapper's target object:
   *
   * @li if @c T is a pointer to a function type accepting two arguments, @c
   * second_argument_type is a synonym for the return type of @c T;
   *
   * @li if @c T is a class type with a member type @c first_argument_type,
   * then @c second_argument_type is a synonym for @c T::second_argument_type;
   *
   * @li otherwise @c second_argument_type is not defined.
   */
  typedef see_below second_argument_type;
#endif // defined(GENERATING_DOCUMENTATION)

  /// Construct a cancellation slot wrapper for the specified object.
  /**
   * This constructor is only valid if the type @c T is constructible from type
   * @c U.
   */
  template <typename U>
  cancellation_slot_binder(const cancellation_slot_type& s, U&& u)
    : slot_(s),
      target_(static_cast<U&&>(u))
  {
  }

  /// Copy constructor.
  cancellation_slot_binder(const cancellation_slot_binder& other)
    : slot_(other.get_cancellation_slot()),
      target_(other.get())
  {
  }

  /// Construct a copy, but specify a different cancellation slot.
  cancellation_slot_binder(const cancellation_slot_type& s,
      const cancellation_slot_binder& other)
    : slot_(s),
      target_(other.get())
  {
  }

  /// Construct a copy of a different cancellation slot wrapper type.
  /**
   * This constructor is only valid if the @c CancellationSlot type is
   * constructible from type @c OtherCancellationSlot, and the type @c T is
   * constructible from type @c U.
   */
  template <typename U, typename OtherCancellationSlot>
  cancellation_slot_binder(
      const cancellation_slot_binder<U, OtherCancellationSlot>& other)
    : slot_(other.get_cancellation_slot()),
      target_(other.get())
  {
  }

  /// Construct a copy of a different cancellation slot wrapper type, but
  /// specify a different cancellation slot.
  /**
   * This constructor is only valid if the type @c T is constructible from type
   * @c U.
   */
  template <typename U, typename OtherCancellationSlot>
  cancellation_slot_binder(const cancellation_slot_type& s,
      const cancellation_slot_binder<U, OtherCancellationSlot>& other)
    : slot_(s),
      target_(other.get())
  {
  }

  /// Move constructor.
  cancellation_slot_binder(cancellation_slot_binder&& other)
    : slot_(static_cast<cancellation_slot_type&&>(
          other.get_cancellation_slot())),
      target_(static_cast<T&&>(other.get()))
  {
  }

  /// Move construct the target object, but specify a different cancellation
  /// slot.
  cancellation_slot_binder(const cancellation_slot_type& s,
      cancellation_slot_binder&& other)
    : slot_(s),
      target_(static_cast<T&&>(other.get()))
  {
  }

  /// Move construct from a different cancellation slot wrapper type.
  template <typename U, typename OtherCancellationSlot>
  cancellation_slot_binder(
      cancellation_slot_binder<U, OtherCancellationSlot>&& other)
    : slot_(static_cast<OtherCancellationSlot&&>(
          other.get_cancellation_slot())),
      target_(static_cast<U&&>(other.get()))
  {
  }

  /// Move construct from a different cancellation slot wrapper type, but
  /// specify a different cancellation slot.
  template <typename U, typename OtherCancellationSlot>
  cancellation_slot_binder(const cancellation_slot_type& s,
      cancellation_slot_binder<U, OtherCancellationSlot>&& other)
    : slot_(s),
      target_(static_cast<U&&>(other.get()))
  {
  }

  /// Destructor.
  ~cancellation_slot_binder()
  {
  }

  /// Obtain a reference to the target object.
  target_type& get() noexcept
  {
    return target_;
  }

  /// Obtain a reference to the target object.
  const target_type& get() const noexcept
  {
    return target_;
  }

  /// Obtain the associated cancellation slot.
  cancellation_slot_type get_cancellation_slot() const noexcept
  {
    return slot_;
  }

  /// Forwarding function call operator.
  template <typename... Args>
  result_of_t<T(Args...)> operator()(Args&&... args)
  {
    return target_(static_cast<Args&&>(args)...);
  }

  /// Forwarding function call operator.
  template <typename... Args>
  result_of_t<T(Args...)> operator()(Args&&... args) const
  {
    return target_(static_cast<Args&&>(args)...);
  }

private:
  CancellationSlot slot_;
  T target_;
};

/// Associate an object of type @c T with a cancellation slot of type
/// @c CancellationSlot.
template <typename CancellationSlot, typename T>
ASIO_NODISCARD inline
cancellation_slot_binder<decay_t<T>, CancellationSlot>
bind_cancellation_slot(const CancellationSlot& s, T&& t)
{
  return cancellation_slot_binder<decay_t<T>, CancellationSlot>(
      s, static_cast<T&&>(t));
}

#if !defined(GENERATING_DOCUMENTATION)

namespace detail {

template <typename TargetAsyncResult,
    typename CancellationSlot, typename = void>
class cancellation_slot_binder_completion_handler_async_result
{
public:
  template <typename T>
  explicit cancellation_slot_binder_completion_handler_async_result(T&)
  {
  }
};

template <typename TargetAsyncResult, typename CancellationSlot>
class cancellation_slot_binder_completion_handler_async_result<
    TargetAsyncResult, CancellationSlot,
    void_t<typename TargetAsyncResult::completion_handler_type>>
{
public:
  typedef cancellation_slot_binder<
    typename TargetAsyncResult::completion_handler_type, CancellationSlot>
      completion_handler_type;

  explicit cancellation_slot_binder_completion_handler_async_result(
      typename TargetAsyncResult::completion_handler_type& handler)
    : target_(handler)
  {
  }

  typename TargetAsyncResult::return_type get()
  {
    return target_.get();
  }

private:
  TargetAsyncResult target_;
};

template <typename TargetAsyncResult, typename = void>
struct cancellation_slot_binder_async_result_return_type
{
};

template <typename TargetAsyncResult>
struct cancellation_slot_binder_async_result_return_type<
    TargetAsyncResult, void_t<typename TargetAsyncResult::return_type>>
{
  typedef typename TargetAsyncResult::return_type return_type;
};

} // namespace detail

template <typename T, typename CancellationSlot, typename Signature>
class async_result<cancellation_slot_binder<T, CancellationSlot>, Signature> :
  public detail::cancellation_slot_binder_completion_handler_async_result<
      async_result<T, Signature>, CancellationSlot>,
  public detail::cancellation_slot_binder_async_result_return_type<
      async_result<T, Signature>>
{
public:
  explicit async_result(cancellation_slot_binder<T, CancellationSlot>& b)
    : detail::cancellation_slot_binder_completion_handler_async_result<
        async_result<T, Signature>, CancellationSlot>(b.get())
  {
  }

  template <typename Initiation>
  struct init_wrapper
  {
    template <typename Init>
    init_wrapper(const CancellationSlot& slot, Init&& init)
      : slot_(slot),
        initiation_(static_cast<Init&&>(init))
    {
    }

    template <typename Handler, typename... Args>
    void operator()(Handler&& handler, Args&&... args)
    {
      static_cast<Initiation&&>(initiation_)(
          cancellation_slot_binder<decay_t<Handler>, CancellationSlot>(
              slot_, static_cast<Handler&&>(handler)),
          static_cast<Args&&>(args)...);
    }

    template <typename Handler, typename... Args>
    void operator()(Handler&& handler, Args&&... args) const
    {
      initiation_(
          cancellation_slot_binder<decay_t<Handler>, CancellationSlot>(
              slot_, static_cast<Handler&&>(handler)),
          static_cast<Args&&>(args)...);
    }

    CancellationSlot slot_;
    Initiation initiation_;
  };

  template <typename Initiation, typename RawCompletionToken, typename... Args>
  static auto initiate(Initiation&& initiation,
      RawCompletionToken&& token, Args&&... args)
    -> decltype(
      async_initiate<T, Signature>(
        declval<init_wrapper<decay_t<Initiation>>>(),
        token.get(), static_cast<Args&&>(args)...))
  {
    return async_initiate<T, Signature>(
        init_wrapper<decay_t<Initiation>>(
          token.get_cancellation_slot(),
          static_cast<Initiation&&>(initiation)),
        token.get(), static_cast<Args&&>(args)...);
  }

private:
  async_result(const async_result&) = delete;
  async_result& operator=(const async_result&) = delete;

  async_result<T, Signature> target_;
};

template <template <typename, typename> class Associator,
    typename T, typename CancellationSlot, typename DefaultCandidate>
struct associator<Associator,
    cancellation_slot_binder<T, CancellationSlot>,
    DefaultCandidate>
  : Associator<T, DefaultCandidate>
{
  static typename Associator<T, DefaultCandidate>::type get(
      const cancellation_slot_binder<T, CancellationSlot>& b) noexcept
  {
    return Associator<T, DefaultCandidate>::get(b.get());
  }

  static auto get(const cancellation_slot_binder<T, CancellationSlot>& b,
      const DefaultCandidate& c) noexcept
    -> decltype(Associator<T, DefaultCandidate>::get(b.get(), c))
  {
    return Associator<T, DefaultCandidate>::get(b.get(), c);
  }
};

template <typename T, typename CancellationSlot, typename CancellationSlot1>
struct associated_cancellation_slot<
    cancellation_slot_binder<T, CancellationSlot>,
    CancellationSlot1>
{
  typedef CancellationSlot type;

  static auto get(const cancellation_slot_binder<T, CancellationSlot>& b,
      const CancellationSlot1& = CancellationSlot1()) noexcept
    -> decltype(b.get_cancellation_slot())
  {
    return b.get_cancellation_slot();
  }
};

#endif // !defined(GENERATING_DOCUMENTATION)

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_BIND_CANCELLATION_SLOT_HPP
