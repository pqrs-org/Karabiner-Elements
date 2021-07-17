//
// traits/equality_comparable.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_TRAITS_EQUALITY_COMPARABLE_HPP
#define ASIO_TRAITS_EQUALITY_COMPARABLE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/type_traits.hpp"

#if defined(ASIO_HAS_DECLTYPE) \
  && defined(ASIO_HAS_NOEXCEPT) \
  && defined(ASIO_HAS_WORKING_EXPRESSION_SFINAE)
# define ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT 1
#endif // defined(ASIO_HAS_DECLTYPE)
       //   && defined(ASIO_HAS_NOEXCEPT)
       //   && defined(ASIO_HAS_WORKING_EXPRESSION_SFINAE)

namespace asio {
namespace traits {

template <typename T, typename = void>
struct equality_comparable_default;

template <typename T, typename = void>
struct equality_comparable;

} // namespace traits
namespace detail {

struct no_equality_comparable
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = false);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
};

#if defined(ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)

template <typename T, typename = void>
struct equality_comparable_trait : no_equality_comparable
{
};

template <typename T>
struct equality_comparable_trait<T,
  typename void_type<
    decltype(
      static_cast<void>(
        static_cast<bool>(declval<const T>() == declval<const T>())
      ),
      static_cast<void>(
        static_cast<bool>(declval<const T>() != declval<const T>())
      )
    )
  >::type>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);

  ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
    noexcept(declval<const T>() == declval<const T>())
      && noexcept(declval<const T>() != declval<const T>()));
};

#else // defined(ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)

template <typename T, typename = void>
struct equality_comparable_trait :
  conditional<
    is_same<T, typename decay<T>::type>::value,
    no_equality_comparable,
    traits::equality_comparable<typename decay<T>::type>
  >::type
{
};

#endif // defined(ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)

} // namespace detail
namespace traits {

template <typename T, typename>
struct equality_comparable_default : detail::equality_comparable_trait<T>
{
};

template <typename T, typename>
struct equality_comparable : equality_comparable_default<T>
{
};

} // namespace traits
} // namespace asio

#endif // ASIO_TRAITS_EQUALITY_COMPARABLE_HPP
