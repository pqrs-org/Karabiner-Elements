//
// traits/query_static_constexpr_member.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_TRAITS_QUERY_STATIC_CONSTEXPR_MEMBER_HPP
#define ASIO_TRAITS_QUERY_STATIC_CONSTEXPR_MEMBER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/type_traits.hpp"

#if defined(ASIO_HAS_DECLTYPE) \
  && defined(ASIO_HAS_NOEXCEPT) \
  && defined(ASIO_HAS_CONSTEXPR) \
  && defined(ASIO_HAS_CONSTANT_EXPRESSION_SFINAE) \
  && defined(ASIO_HAS_WORKING_EXPRESSION_SFINAE)
# define ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT 1
#endif // defined(ASIO_HAS_DECLTYPE)
       //   && defined(ASIO_HAS_NOEXCEPT)
       //   && defined(ASIO_HAS_CONSTEXPR)
       //   && defined(ASIO_HAS_CONSTANT_EXPRESSION_SFINAE)
       //   && defined(ASIO_HAS_WORKING_EXPRESSION_SFINAE)

#include "asio/detail/push_options.hpp"

namespace asio {
namespace traits {

template <typename T, typename Property, typename = void>
struct query_static_constexpr_member_default;

template <typename T, typename Property, typename = void>
struct query_static_constexpr_member;

} // namespace traits
namespace detail {

struct no_query_static_constexpr_member
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = false);
};

template <typename T, typename Property, typename = void>
struct query_static_constexpr_member_trait :
  conditional<
    is_same<T, typename decay<T>::type>::value
      && is_same<Property, typename decay<Property>::type>::value,
    no_query_static_constexpr_member,
    traits::query_static_constexpr_member<
      typename decay<T>::type,
      typename decay<Property>::type>
  >::type
{
};

#if defined(ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)

template <typename T, typename Property>
struct query_static_constexpr_member_trait<T, Property,
  typename enable_if<
    (static_cast<void>(T::query(Property{})), true)
  >::type>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);

  using result_type = decltype(T::query(Property{}));

  ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
    noexcept(T::query(Property{})));

  static ASIO_CONSTEXPR result_type value() noexcept(is_noexcept)
  {
    return T::query(Property{});
  }
};

#endif // defined(ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)

} // namespace detail
namespace traits {

template <typename T, typename Property, typename>
struct query_static_constexpr_member_default :
  detail::query_static_constexpr_member_trait<T, Property>
{
};

template <typename T, typename Property, typename>
struct query_static_constexpr_member :
  query_static_constexpr_member_default<T, Property>
{
};

} // namespace traits
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_TRAITS_QUERY_STATIC_CONSTEXPR_MEMBER_HPP
