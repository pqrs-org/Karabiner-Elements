//
// traits/set_value_member.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_TRAITS_SET_VALUE_MEMBER_HPP
#define ASIO_TRAITS_SET_VALUE_MEMBER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/detail/variadic_templates.hpp"

#if defined(ASIO_HAS_DECLTYPE) \
  && defined(ASIO_HAS_NOEXCEPT) \
  && defined(ASIO_HAS_WORKING_EXPRESSION_SFINAE)
# define ASIO_HAS_DEDUCED_SET_VALUE_MEMBER_TRAIT 1
#endif // defined(ASIO_HAS_DECLTYPE)
       //   && defined(ASIO_HAS_NOEXCEPT)
       //   && defined(ASIO_HAS_WORKING_EXPRESSION_SFINAE)

#include "asio/detail/push_options.hpp"

namespace asio {
namespace traits {

template <typename T, typename Vs, typename = void>
struct set_value_member_default;

template <typename T, typename Vs, typename = void>
struct set_value_member;

} // namespace traits
namespace detail {

struct no_set_value_member
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = false);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
};

#if defined(ASIO_HAS_DEDUCED_SET_VALUE_MEMBER_TRAIT)

template <typename T, typename Vs, typename = void>
struct set_value_member_trait : no_set_value_member
{
};

#if defined(ASIO_HAS_VARIADIC_TEMPLATES)

template <typename T, typename... Vs>
struct set_value_member_trait<T, void(Vs...),
  typename void_type<
    decltype(declval<T>().set_value(declval<Vs>()...))
  >::type>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);

  using result_type = decltype(
    declval<T>().set_value(declval<Vs>()...));

  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = noexcept(
    declval<T>().set_value(declval<Vs>()...)));
};

#else // defined(ASIO_HAS_VARIADIC_TEMPLATES)

template <typename T>
struct set_value_member_trait<T, void(),
  typename void_type<
    decltype(declval<T>().set_value())
  >::type>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);

  using result_type = decltype(declval<T>().set_value());

  ASIO_STATIC_CONSTEXPR(bool,
    is_noexcept = noexcept(declval<T>().set_value()));
};

#define ASIO_PRIVATE_SET_VALUE_MEMBER_TRAIT_DEF(n) \
  template <typename T, ASIO_VARIADIC_TPARAMS(n)> \
  struct set_value_member_trait<T, void(ASIO_VARIADIC_TARGS(n)), \
    typename void_type< \
      decltype(declval<T>().set_value(ASIO_VARIADIC_DECLVAL(n))) \
    >::type> \
  { \
    ASIO_STATIC_CONSTEXPR(bool, is_valid = true); \
  \
    using result_type = decltype( \
      declval<T>().set_value(ASIO_VARIADIC_DECLVAL(n))); \
  \
    ASIO_STATIC_CONSTEXPR(bool, is_noexcept = noexcept( \
      declval<T>().set_value(ASIO_VARIADIC_DECLVAL(n)))); \
  }; \
  /**/
ASIO_VARIADIC_GENERATE(ASIO_PRIVATE_SET_VALUE_MEMBER_TRAIT_DEF)
#undef ASIO_PRIVATE_SET_VALUE_MEMBER_TRAIT_DEF

#endif // defined(ASIO_HAS_VARIADIC_TEMPLATES)

#else // defined(ASIO_HAS_DEDUCED_SET_VALUE_MEMBER_TRAIT)

template <typename T, typename Vs, typename = void>
struct set_value_member_trait;

#if defined(ASIO_HAS_VARIADIC_TEMPLATES)

template <typename T, typename... Vs>
struct set_value_member_trait<T, void(Vs...)> :
  conditional<
    is_same<T, typename remove_reference<T>::type>::value
      && conjunction<is_same<Vs, typename decay<Vs>::type>...>::value,
    typename conditional<
      is_same<T, typename add_const<T>::type>::value,
      no_set_value_member,
      traits::set_value_member<typename add_const<T>::type, void(Vs...)>
    >::type,
    traits::set_value_member<
      typename remove_reference<T>::type,
      void(typename decay<Vs>::type...)>
  >::type
{
};

#else // defined(ASIO_HAS_VARIADIC_TEMPLATES)

template <typename T>
struct set_value_member_trait<T, void()> :
  conditional<
    is_same<T, typename remove_reference<T>::type>::value,
    typename conditional<
      is_same<T, typename add_const<T>::type>::value,
      no_set_value_member,
      traits::set_value_member<typename add_const<T>::type, void()>
    >::type,
    traits::set_value_member<typename remove_reference<T>::type, void()>
  >::type
{
};

#define ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME(n) \
  ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_##n

#define ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_1 \
  && is_same<T1, typename decay<T1>::type>::value
#define ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_2 \
  ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_1 \
  && is_same<T2, typename decay<T2>::type>::value
#define ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_3 \
  ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_2 \
  && is_same<T3, typename decay<T3>::type>::value
#define ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_4 \
  ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_3 \
  && is_same<T4, typename decay<T4>::type>::value
#define ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_5 \
  ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_4 \
  && is_same<T5, typename decay<T5>::type>::value
#define ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_6 \
  ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_5 \
  && is_same<T6, typename decay<T6>::type>::value
#define ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_7 \
  ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_6 \
  && is_same<T7, typename decay<T7>::type>::value
#define ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_8 \
  ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_7 \
  && is_same<T8, typename decay<T8>::type>::value

#define ASIO_PRIVATE_SET_VALUE_MEMBER_TRAIT_DEF(n) \
  template <typename T, ASIO_VARIADIC_TPARAMS(n)> \
  struct set_value_member_trait<T, void(ASIO_VARIADIC_TARGS(n))> : \
    conditional< \
      is_same<T, typename remove_reference<T>::type>::value \
        ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME(n), \
      typename conditional< \
        is_same<T, typename add_const<T>::type>::value, \
        no_set_value_member, \
        traits::set_value_member< \
          typename add_const<T>::type, \
          void(ASIO_VARIADIC_TARGS(n))> \
      >::type, \
      traits::set_value_member< \
        typename remove_reference<T>::type, \
        void(ASIO_VARIADIC_DECAY(n))> \
    >::type \
  { \
  }; \
  /**/
ASIO_VARIADIC_GENERATE(ASIO_PRIVATE_SET_VALUE_MEMBER_TRAIT_DEF)
#undef ASIO_PRIVATE_SET_VALUE_MEMBER_TRAIT_DEF
#undef ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME
#undef ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_1
#undef ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_2
#undef ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_3
#undef ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_4
#undef ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_5
#undef ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_6
#undef ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_7
#undef ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_8

#endif // defined(ASIO_HAS_VARIADIC_TEMPLATES)

#endif // defined(ASIO_HAS_DEDUCED_SET_VALUE_MEMBER_TRAIT)

} // namespace detail
namespace traits {

template <typename T, typename Vs, typename>
struct set_value_member_default :
  detail::set_value_member_trait<T, Vs>
{
};

template <typename T, typename Vs, typename>
struct set_value_member :
  set_value_member_default<T, Vs>
{
};

} // namespace traits
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_TRAITS_SET_VALUE_MEMBER_HPP
