//
// traits/static_query.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_TRAITS_STATIC_QUERY_HPP
#define ASIO_TRAITS_STATIC_QUERY_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/type_traits.hpp"

#if defined(ASIO_HAS_DECLTYPE) \
  && defined(ASIO_HAS_NOEXCEPT) \
  && defined(ASIO_HAS_CONSTEXPR) \
  && defined(ASIO_HAS_VARIABLE_TEMPLATES) \
  && defined(ASIO_HAS_WORKING_EXPRESSION_SFINAE)
# define ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT 1
#endif // defined(ASIO_HAS_DECLTYPE)
       //   && defined(ASIO_HAS_NOEXCEPT)
       //   && defined(ASIO_HAS_CONSTEXPR)
       //   && defined(ASIO_HAS_VARIABLE_TEMPLATES)
       //   && defined(ASIO_HAS_WORKING_EXPRESSION_SFINAE)

#include "asio/detail/push_options.hpp"

namespace asio {
namespace traits {

template <typename T, typename Property, typename = void>
struct static_query_default;

template <typename T, typename Property, typename = void>
struct static_query;

} // namespace traits
namespace detail {

struct no_static_query
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = false);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
};

template <typename T, typename Property, typename = void>
struct static_query_trait :
  conditional<
    is_same<T, typename decay<T>::type>::value
      && is_same<Property, typename decay<Property>::type>::value,
    no_static_query,
    traits::static_query<
      typename decay<T>::type,
      typename decay<Property>::type>
  >::type
{
};

#if defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)

template <typename T, typename Property>
struct static_query_trait<T, Property,
  typename void_type<
    decltype(decay<Property>::type::template static_query_v<T>)
  >::type>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);

  using result_type = decltype(
      decay<Property>::type::template static_query_v<T>);

  ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
    noexcept(decay<Property>::type::template static_query_v<T>));

  static ASIO_CONSTEXPR result_type value() noexcept(is_noexcept)
  {
    return decay<Property>::type::template static_query_v<T>;
  }
};

#endif // defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)

} // namespace detail
namespace traits {

template <typename T, typename Property, typename>
struct static_query_default : detail::static_query_trait<T, Property>
{
};

template <typename T, typename Property, typename>
struct static_query : static_query_default<T, Property>
{
};

} // namespace traits
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_TRAITS_STATIC_QUERY_HPP
