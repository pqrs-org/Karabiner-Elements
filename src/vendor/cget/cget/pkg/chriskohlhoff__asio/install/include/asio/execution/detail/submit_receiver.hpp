//
// execution/detail/submit_receiver.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_EXECUTION_DETAIL_SUBMIT_RECEIVER_HPP
#define ASIO_EXECUTION_DETAIL_SUBMIT_RECEIVER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/detail/variadic_templates.hpp"
#include "asio/execution/connect.hpp"
#include "asio/execution/receiver.hpp"
#include "asio/execution/set_done.hpp"
#include "asio/execution/set_error.hpp"
#include "asio/execution/set_value.hpp"
#include "asio/traits/set_done_member.hpp"
#include "asio/traits/set_error_member.hpp"
#include "asio/traits/set_value_member.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace execution {
namespace detail {

template <typename Sender, typename Receiver>
struct submit_receiver;

template <typename Sender, typename Receiver>
struct submit_receiver_wrapper
{
  submit_receiver<Sender, Receiver>* p_;

  explicit submit_receiver_wrapper(submit_receiver<Sender, Receiver>* p)
    : p_(p)
  {
  }

#if defined(ASIO_HAS_VARIADIC_TEMPLATES)

  template <typename... Args>
  typename enable_if<is_receiver_of<Receiver, Args...>::value>::type
  set_value(ASIO_MOVE_ARG(Args)... args) ASIO_RVALUE_REF_QUAL
    ASIO_NOEXCEPT_IF((is_nothrow_receiver_of<Receiver, Args...>::value))
  {
    execution::set_value(
        ASIO_MOVE_OR_LVALUE(
          typename remove_cvref<Receiver>::type)(p_->r_),
        ASIO_MOVE_CAST(Args)(args)...);
    delete p_;
  }

#else // defined(ASIO_HAS_VARIADIC_TEMPLATES)

  void set_value() ASIO_RVALUE_REF_QUAL
    ASIO_NOEXCEPT_IF((is_nothrow_receiver_of<Receiver>::value))
  {
    execution::set_value(
        ASIO_MOVE_OR_LVALUE(
          typename remove_cvref<Receiver>::type)(p_->r_));
    delete p_;
  }

#define ASIO_PRIVATE_SUBMIT_RECEIVER_SET_VALUE_DEF(n) \
  template <ASIO_VARIADIC_TPARAMS(n)> \
  typename enable_if<is_receiver_of<Receiver, \
    ASIO_VARIADIC_TARGS(n)>::value>::type \
  set_value(ASIO_VARIADIC_MOVE_PARAMS(n)) ASIO_RVALUE_REF_QUAL \
    ASIO_NOEXCEPT_IF((is_nothrow_receiver_of< \
      Receiver, ASIO_VARIADIC_TARGS(n)>::value)) \
  { \
    execution::set_value( \
        ASIO_MOVE_OR_LVALUE( \
          typename remove_cvref<Receiver>::type)(p_->r_), \
        ASIO_VARIADIC_MOVE_ARGS(n)); \
    delete p_; \
  } \
  /**/
ASIO_VARIADIC_GENERATE(ASIO_PRIVATE_SUBMIT_RECEIVER_SET_VALUE_DEF)
#undef ASIO_PRIVATE_SUBMIT_RECEIVER_SET_VALUE_DEF

#endif // defined(ASIO_HAS_VARIADIC_TEMPLATES)

  template <typename E>
  void set_error(ASIO_MOVE_ARG(E) e)
    ASIO_RVALUE_REF_QUAL ASIO_NOEXCEPT
  {
    execution::set_error(
        ASIO_MOVE_OR_LVALUE(
          typename remove_cvref<Receiver>::type)(p_->r_),
        ASIO_MOVE_CAST(E)(e));
    delete p_;
  }

  void set_done() ASIO_RVALUE_REF_QUAL ASIO_NOEXCEPT
  {
    execution::set_done(
        ASIO_MOVE_OR_LVALUE(
          typename remove_cvref<Receiver>::type)(p_->r_));
    delete p_;
  }
};

template <typename Sender, typename Receiver>
struct submit_receiver
{
  typename remove_cvref<Receiver>::type r_;
#if defined(ASIO_HAS_MOVE)
  typename connect_result<Sender,
      submit_receiver_wrapper<Sender, Receiver> >::type state_;
#else // defined(ASIO_HAS_MOVE)
  typename connect_result<Sender,
      const submit_receiver_wrapper<Sender, Receiver>& >::type state_;
#endif // defined(ASIO_HAS_MOVE)

#if defined(ASIO_HAS_MOVE)
  template <typename S, typename R>
  explicit submit_receiver(ASIO_MOVE_ARG(S) s, ASIO_MOVE_ARG(R) r)
    : r_(ASIO_MOVE_CAST(R)(r)),
      state_(execution::connect(ASIO_MOVE_CAST(S)(s),
            submit_receiver_wrapper<Sender, Receiver>(this)))
  {
  }
#else // defined(ASIO_HAS_MOVE)
  explicit submit_receiver(Sender s, Receiver r)
    : r_(r),
      state_(execution::connect(s,
            submit_receiver_wrapper<Sender, Receiver>(this)))
  {
  }
#endif // defined(ASIO_HAS_MOVE)
};

} // namespace detail
} // namespace execution
namespace traits {

#if !defined(ASIO_HAS_DEDUCED_SET_VALUE_MEMBER_TRAIT)

#if defined(ASIO_HAS_VARIADIC_TEMPLATES)

template <typename Sender, typename Receiver, typename... Args>
struct set_value_member<
    asio::execution::detail::submit_receiver_wrapper<
      Sender, Receiver>,
    void(Args...)>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
    (asio::execution::is_nothrow_receiver_of<Receiver, Args...>::value));
  typedef void result_type;
};

#else // defined(ASIO_HAS_VARIADIC_TEMPLATES)

template <typename Sender, typename Receiver>
struct set_value_member<
    asio::execution::detail::submit_receiver_wrapper<
      Sender, Receiver>,
    void()>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
    asio::execution::is_nothrow_receiver_of<Receiver>::value);
  typedef void result_type;
};

#define ASIO_PRIVATE_SUBMIT_RECEIVER_TRAIT_DEF(n) \
  template <typename Sender, typename Receiver, \
      ASIO_VARIADIC_TPARAMS(n)> \
  struct set_value_member< \
      asio::execution::detail::submit_receiver_wrapper< \
        Sender, Receiver>, \
      void(ASIO_VARIADIC_TARGS(n))> \
  { \
    ASIO_STATIC_CONSTEXPR(bool, is_valid = true); \
    ASIO_STATIC_CONSTEXPR(bool, is_noexcept = \
      (asio::execution::is_nothrow_receiver_of<Receiver, \
        ASIO_VARIADIC_TARGS(n)>::value)); \
    typedef void result_type; \
  }; \
  /**/
ASIO_VARIADIC_GENERATE(ASIO_PRIVATE_SUBMIT_RECEIVER_TRAIT_DEF)
#undef ASIO_PRIVATE_SUBMIT_RECEIVER_TRAIT_DEF

#endif // defined(ASIO_HAS_VARIADIC_TEMPLATES)

#endif // !defined(ASIO_HAS_DEDUCED_SET_VALUE_MEMBER_TRAIT)

#if !defined(ASIO_HAS_DEDUCED_SET_ERROR_MEMBER_TRAIT)

template <typename Sender, typename Receiver, typename E>
struct set_error_member<
    asio::execution::detail::submit_receiver_wrapper<
      Sender, Receiver>, E>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef void result_type;
};

#endif // !defined(ASIO_HAS_DEDUCED_SET_ERROR_MEMBER_TRAIT)

#if !defined(ASIO_HAS_DEDUCED_SET_DONE_MEMBER_TRAIT)

template <typename Sender, typename Receiver>
struct set_done_member<
    asio::execution::detail::submit_receiver_wrapper<
      Sender, Receiver> >
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef void result_type;
};

#endif // !defined(ASIO_HAS_DEDUCED_SET_DONE_MEMBER_TRAIT)

} // namespace traits
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_EXECUTION_DETAIL_SUBMIT_RECEIVER_HPP
