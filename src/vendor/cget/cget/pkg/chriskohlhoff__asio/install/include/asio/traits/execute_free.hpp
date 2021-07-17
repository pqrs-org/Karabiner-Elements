//
// traits/execute_free.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_TRAITS_EXECUTE_FREE_HPP
#define ASIO_TRAITS_EXECUTE_FREE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/type_traits.hpp"

#if defined(ASIO_HAS_DECLTYPE) \
  && defined(ASIO_HAS_NOEXCEPT) \
  && defined(ASIO_HAS_WORKING_EXPRESSION_SFINAE)
# define ASIO_HAS_DEDUCED_EXECUTE_FREE_TRAIT 1
#endif // defined(ASIO_HAS_DECLTYPE)
       //   && defined(ASIO_HAS_NOEXCEPT)
       //   && defined(ASIO_HAS_WORKING_EXPRESSION_SFINAE)

#include "asio/detail/push_options.hpp"

namespace asio {
namespace traits {

template <typename T, typename F, typename = void>
struct execute_free_default;

template <typename T, typename F, typename = void>
struct execute_free;

} // namespace traits
namespace detail {

struct no_execute_free
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = false);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
};

#if defined(ASIO_HAS_DEDUCED_EXECUTE_FREE_TRAIT)

template <typename T, typename F, typename = void>
struct execute_free_trait : no_execute_free
{
};

template <typename T, typename F>
struct execute_free_trait<T, F,
  typename void_type<
    decltype(execute(declval<T>(), declval<F>()))
  >::type>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);

  using result_type = decltype(
    execute(declval<T>(), declval<F>()));

  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = noexcept(
    execute(declval<T>(), declval<F>())));
};

#else // defined(ASIO_HAS_DEDUCED_EXECUTE_FREE_TRAIT)

template <typename T, typename F, typename = void>
struct execute_free_trait :
  conditional<
    is_same<T, typename decay<T>::type>::value
      && is_same<F, typename decay<F>::type>::value,
    no_execute_free,
    traits::execute_free<
      typename decay<T>::type,
      typename decay<F>::type>
  >::type
{
};

#endif // defined(ASIO_HAS_DEDUCED_EXECUTE_FREE_TRAIT)

} // namespace detail
namespace traits {

template <typename T, typename F, typename>
struct execute_free_default :
  detail::execute_free_trait<T, F>
{
};

template <typename T, typename F, typename>
struct execute_free :
  execute_free_default<T, F>
{
};

} // namespace traits
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_TRAITS_EXECUTE_FREE_HPP
