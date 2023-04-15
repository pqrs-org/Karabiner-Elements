//
// traits/submit_free.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2023 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_TRAITS_SUBMIT_FREE_HPP
#define ASIO_TRAITS_SUBMIT_FREE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/type_traits.hpp"

#if defined(ASIO_HAS_DECLTYPE) \
  && defined(ASIO_HAS_NOEXCEPT) \
  && defined(ASIO_HAS_WORKING_EXPRESSION_SFINAE)
# define ASIO_HAS_DEDUCED_SUBMIT_FREE_TRAIT 1
#endif // defined(ASIO_HAS_DECLTYPE)
       //   && defined(ASIO_HAS_NOEXCEPT)
       //   && defined(ASIO_HAS_WORKING_EXPRESSION_SFINAE)

#include "asio/detail/push_options.hpp"

namespace asio {
namespace traits {

template <typename S, typename R, typename = void>
struct submit_free_default;

template <typename S, typename R, typename = void>
struct submit_free;

} // namespace traits
namespace detail {

struct no_submit_free
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = false);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
};

#if defined(ASIO_HAS_DEDUCED_SUBMIT_FREE_TRAIT)

template <typename S, typename R, typename = void>
struct submit_free_trait : no_submit_free
{
};

template <typename S, typename R>
struct submit_free_trait<S, R,
  typename void_type<
    decltype(submit(declval<S>(), declval<R>()))
  >::type>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);

  using result_type = decltype(
    submit(declval<S>(), declval<R>()));

  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = noexcept(
    submit(declval<S>(), declval<R>())));
};

#else // defined(ASIO_HAS_DEDUCED_SUBMIT_FREE_TRAIT)

template <typename S, typename R, typename = void>
struct submit_free_trait :
  conditional<
    is_same<S, typename remove_reference<S>::type>::value
      && is_same<R, typename decay<R>::type>::value,
    typename conditional<
      is_same<S, typename add_const<S>::type>::value,
      no_submit_free,
      traits::submit_free<typename add_const<S>::type, R>
    >::type,
    traits::submit_free<
      typename remove_reference<S>::type,
      typename decay<R>::type>
  >::type
{
};

#endif // defined(ASIO_HAS_DEDUCED_SUBMIT_FREE_TRAIT)

} // namespace detail
namespace traits {

template <typename S, typename R, typename>
struct submit_free_default :
  detail::submit_free_trait<S, R>
{
};

template <typename S, typename R, typename>
struct submit_free :
  submit_free_default<S, R>
{
};

} // namespace traits
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_TRAITS_SUBMIT_FREE_HPP
