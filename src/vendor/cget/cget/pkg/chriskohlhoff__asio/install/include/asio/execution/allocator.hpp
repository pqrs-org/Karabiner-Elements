//
// execution/allocator.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_EXECUTION_ALLOCATOR_HPP
#define ASIO_EXECUTION_ALLOCATOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/execution/executor.hpp"
#include "asio/execution/scheduler.hpp"
#include "asio/execution/sender.hpp"
#include "asio/is_applicable_property.hpp"
#include "asio/traits/query_static_constexpr_member.hpp"
#include "asio/traits/static_query.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {

#if defined(GENERATING_DOCUMENTATION)

namespace execution {

/// A property to describe which allocator an executor will use to allocate the
/// memory required to store a submitted function object.
template <typename ProtoAllocator>
struct allocator_t
{
  /// The allocator_t property applies to executors, senders, and schedulers.
  template <typename T>
  static constexpr bool is_applicable_property_v =
    is_executor_v<T> || is_sender_v<T> || is_scheduler_v<T>;

  /// The allocator_t property can be required.
  static constexpr bool is_requirable = true;

  /// The allocator_t property can be preferred.
  static constexpr bool is_preferable = true;

  /// Default constructor.
  constexpr allocator_t();

  /// Obtain the allocator stored in the allocator_t property object.
  /**
   * Present only if @c ProtoAllocator is non-void.
   */
  constexpr ProtoAllocator value() const;

  /// Create an allocator_t object with a different allocator.
  /**
   * Present only if @c ProtoAllocator is void.
   */
  template <typename OtherAllocator>
  allocator_t<OtherAllocator operator()(const OtherAllocator& a);
};

/// A special value used for accessing the allocator_t property.
constexpr allocator_t<void> allocator;

} // namespace execution

#else // defined(GENERATING_DOCUMENTATION)

namespace execution {

template <typename ProtoAllocator>
struct allocator_t
{
#if defined(ASIO_HAS_VARIABLE_TEMPLATES)
  template <typename T>
  ASIO_STATIC_CONSTEXPR(bool,
    is_applicable_property_v = is_executor<T>::value
      || is_sender<T>::value || is_scheduler<T>::value);
#endif // defined(ASIO_HAS_VARIABLE_TEMPLATES)

  ASIO_STATIC_CONSTEXPR(bool, is_requirable = true);
  ASIO_STATIC_CONSTEXPR(bool, is_preferable = true);

#if defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
  template <typename T>
  static ASIO_CONSTEXPR
  typename traits::query_static_constexpr_member<T, allocator_t>::result_type
  static_query()
    ASIO_NOEXCEPT_IF((
      traits::query_static_constexpr_member<T, allocator_t>::is_noexcept))
  {
    return traits::query_static_constexpr_member<T, allocator_t>::value();
  }

  template <typename E, typename T = decltype(allocator_t::static_query<E>())>
  static ASIO_CONSTEXPR const T static_query_v
    = allocator_t::static_query<E>();
#endif // defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

  ASIO_CONSTEXPR ProtoAllocator value() const
  {
    return a_;
  }

private:
  friend struct allocator_t<void>;

  explicit ASIO_CONSTEXPR allocator_t(const ProtoAllocator& a)
    : a_(a)
  {
  }

  ProtoAllocator a_;
};

#if defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
template <typename ProtoAllocator> template <typename E, typename T>
const T allocator_t<ProtoAllocator>::static_query_v;
#endif // defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

template <>
struct allocator_t<void>
{
#if defined(ASIO_HAS_VARIABLE_TEMPLATES)
  template <typename T>
  ASIO_STATIC_CONSTEXPR(bool,
    is_applicable_property_v = is_executor<T>::value
      || is_sender<T>::value || is_scheduler<T>::value);
#endif // defined(ASIO_HAS_VARIABLE_TEMPLATES)

  ASIO_STATIC_CONSTEXPR(bool, is_requirable = true);
  ASIO_STATIC_CONSTEXPR(bool, is_preferable = true);

  ASIO_CONSTEXPR allocator_t()
  {
  }

#if defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
  template <typename T>
  static ASIO_CONSTEXPR
  typename traits::query_static_constexpr_member<T, allocator_t>::result_type
  static_query()
    ASIO_NOEXCEPT_IF((
      traits::query_static_constexpr_member<T, allocator_t>::is_noexcept))
  {
    return traits::query_static_constexpr_member<T, allocator_t>::value();
  }

  template <typename E, typename T = decltype(allocator_t::static_query<E>())>
  static ASIO_CONSTEXPR const T static_query_v
    = allocator_t::static_query<E>();
#endif // defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

  template <typename OtherProtoAllocator>
  ASIO_CONSTEXPR allocator_t<OtherProtoAllocator> operator()(
      const OtherProtoAllocator& a) const
  {
    return allocator_t<OtherProtoAllocator>(a);
  }
};

#if defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
template <typename E, typename T>
const T allocator_t<void>::static_query_v;
#endif // defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

#if defined(ASIO_HAS_CONSTEXPR) || defined(GENERATING_DOCUMENTATION)
constexpr allocator_t<void> allocator;
#else // defined(ASIO_HAS_CONSTEXPR) || defined(GENERATING_DOCUMENTATION)
template <typename T>
struct allocator_instance
{
  static allocator_t<T> instance;
};

template <typename T>
allocator_t<T> allocator_instance<T>::instance;

namespace {
static const allocator_t<void>& allocator = allocator_instance<void>::instance;
} // namespace
#endif

} // namespace execution

#if !defined(ASIO_HAS_VARIABLE_TEMPLATES)

template <typename T, typename ProtoAllocator>
struct is_applicable_property<T, execution::allocator_t<ProtoAllocator> >
  : integral_constant<bool,
      execution::is_executor<T>::value
        || execution::is_sender<T>::value
        || execution::is_scheduler<T>::value>
{
};

#endif // !defined(ASIO_HAS_VARIABLE_TEMPLATES)

namespace traits {

#if !defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  || !defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

template <typename T, typename ProtoAllocator>
struct static_query<T, execution::allocator_t<ProtoAllocator>,
  typename enable_if<
    traits::query_static_constexpr_member<T,
      execution::allocator_t<ProtoAllocator> >::is_valid
  >::type>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename traits::query_static_constexpr_member<T,
    execution::allocator_t<ProtoAllocator> >::result_type result_type;

  static ASIO_CONSTEXPR result_type value()
  {
    return traits::query_static_constexpr_member<T,
      execution::allocator_t<ProtoAllocator> >::value();
  }
};

#endif // !defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   || !defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

} // namespace traits

#endif // defined(GENERATING_DOCUMENTATION)

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_EXECUTION_ALLOCATOR_HPP
