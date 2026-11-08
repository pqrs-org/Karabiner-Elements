//
// is_shallow_mutable_sequence.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_IS_SHALLOW_MUTABLE_SEQUENCE_HPP
#define ASIO_IS_SHALLOW_MUTABLE_SEQUENCE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/functional.hpp"
#include "asio/detail/type_traits.hpp"

#if defined(ASIO_HAS_STD_RANGES)
# include <ranges>
#endif // defined(ASIO_HAS_STD_RANGES)

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN

namespace detail {

template <typename T>
struct is_reference_wrapper : false_type
{
};

template <typename T>
struct is_reference_wrapper<reference_wrapper<T>> : true_type
{
};

template <typename T>
struct unwrapped_sequence
{
  typedef T& type;
};

template <typename T>
struct unwrapped_sequence<reference_wrapper<T>>
{
  typedef T& type;
};

template <typename Element, typename = decay_t<Element>>
struct is_writable_element
  : integral_constant<bool,
      is_lvalue_reference<Element>::value
        && !is_const<remove_reference_t<Element>>::value>
{
};

template <typename Element, typename T>
struct is_writable_element<Element, reference_wrapper<T>>
  : integral_constant<bool, !is_const<T>::value>
{
};

template <typename T, typename = void, typename = void>
struct is_range_impl : false_type
{
};

template <typename T>
struct is_range_impl<T,
    void_t<decltype(declval<typename unwrapped_sequence<T>::type>().begin())>,
    void_t<decltype(declval<typename unwrapped_sequence<T>::type>().end())>>
  : true_type
{
};

template <typename T>
struct is_range : is_range_impl<remove_cvref_t<T>>
{
};

template <typename T, typename = void>
struct is_shallow_mutable_sequence_impl : false_type
{
};

template <typename T>
struct is_shallow_mutable_sequence_impl<T,
    void_t<decltype(*declval<typename unwrapped_sequence<T>::type>().begin())>>
  : integral_constant<bool,
      (is_reference_wrapper<T>::value
        || is_reference_wrapper<decay_t<decltype(*declval<
          typename unwrapped_sequence<T>::type>().begin())>>::value
# if defined(ASIO_HAS_STD_RANGES)
        || std::ranges::borrowed_range<T>
# endif // defined(ASIO_HAS_STD_RANGES)
      )
      && is_writable_element<decltype(*declval<
        typename unwrapped_sequence<T>::type>().begin())>::value>
{
};

} // namespace detail

/// The is_shallow_mutable_sequence class is a traits class that may be used to
/// determine whether a type is a range whose copies refer to the same writable
/// elements.
/**
 * By default, the trait is true for a @c std::reference_wrapper to a range, a
 * range of @c std::reference_wrapper elements, and a borrowed range such as
 * @c std::span, provided that the elements are writable. Owning containers
 * such as @c std::vector are not shallow mutable sequences.
 *
 * A program may specialise this traits type if the @c T template parameter in
 * the specialisation is a user-defined type.
 */
template <typename T>
struct is_shallow_mutable_sequence :
#if defined(GENERATING_DOCUMENTATION)
  integral_constant<bool, automatically_determined>
#else // defined(GENERATING_DOCUMENTATION)
  detail::is_shallow_mutable_sequence_impl<remove_cvref_t<T>>
#endif // defined(GENERATING_DOCUMENTATION)
{
};

ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_IS_SHALLOW_MUTABLE_SEQUENCE_HPP
