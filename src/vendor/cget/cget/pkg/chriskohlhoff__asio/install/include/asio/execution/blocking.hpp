//
// execution/blocking.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2022 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_EXECUTION_BLOCKING_HPP
#define ASIO_EXECUTION_BLOCKING_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/execution/execute.hpp"
#include "asio/execution/executor.hpp"
#include "asio/execution/scheduler.hpp"
#include "asio/execution/sender.hpp"
#include "asio/is_applicable_property.hpp"
#include "asio/prefer.hpp"
#include "asio/query.hpp"
#include "asio/require.hpp"
#include "asio/traits/query_free.hpp"
#include "asio/traits/query_member.hpp"
#include "asio/traits/query_static_constexpr_member.hpp"
#include "asio/traits/static_query.hpp"
#include "asio/traits/static_require.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {

#if defined(GENERATING_DOCUMENTATION)

namespace execution {

/// A property to describe what guarantees an executor makes about the blocking
/// behaviour of their execution functions.
struct blocking_t
{
  /// The blocking_t property applies to executors, senders, and schedulers.
  template <typename T>
  static constexpr bool is_applicable_property_v =
    is_executor_v<T> || is_sender_v<T> || is_scheduler_v<T>;

  /// The top-level blocking_t property cannot be required.
  static constexpr bool is_requirable = false;

  /// The top-level blocking_t property cannot be preferred.
  static constexpr bool is_preferable = false;

  /// The type returned by queries against an @c any_executor.
  typedef blocking_t polymorphic_query_result_type;

  /// A sub-property that indicates that invocation of an executor's execution
  /// function may block pending completion of one or more invocations of the
  /// submitted function object.
  struct possibly_t
  {
    /// The blocking_t::possibly_t property applies to executors, senders, and
    /// schedulers.
    template <typename T>
    static constexpr bool is_applicable_property_v =
      is_executor_v<T> || is_sender_v<T> || is_scheduler_v<T>;

    /// The blocking_t::possibly_t property can be required.
    static constexpr bool is_requirable = true;

    /// The blocking_t::possibly_t property can be preferred.
    static constexpr bool is_preferable = true;

    /// The type returned by queries against an @c any_executor.
    typedef blocking_t polymorphic_query_result_type;

    /// Default constructor.
    constexpr possibly_t();

    /// Get the value associated with a property object.
    /**
     * @returns possibly_t();
     */
    static constexpr blocking_t value();
  };

  /// A sub-property that indicates that invocation of an executor's execution
  /// function shall block until completion of all invocations of the submitted
  /// function object.
  struct always_t
  {
    /// The blocking_t::always_t property applies to executors, senders, and
    /// schedulers.
    template <typename T>
    static constexpr bool is_applicable_property_v =
      is_executor_v<T> || is_sender_v<T> || is_scheduler_v<T>;

    /// The blocking_t::always_t property can be required.
    static constexpr bool is_requirable = true;

    /// The blocking_t::always_t property can be preferred.
    static constexpr bool is_preferable = false;

    /// The type returned by queries against an @c any_executor.
    typedef blocking_t polymorphic_query_result_type;

    /// Default constructor.
    constexpr always_t();

    /// Get the value associated with a property object.
    /**
     * @returns always_t();
     */
    static constexpr blocking_t value();
  };

  /// A sub-property that indicates that invocation of an executor's execution
  /// function shall not block pending completion of the invocations of the
  /// submitted function object.
  struct never_t
  {
    /// The blocking_t::never_t property applies to executors, senders, and
    /// schedulers.
    template <typename T>
    static constexpr bool is_applicable_property_v =
      is_executor_v<T> || is_sender_v<T> || is_scheduler_v<T>;

    /// The blocking_t::never_t property can be required.
    static constexpr bool is_requirable = true;

    /// The blocking_t::never_t property can be preferred.
    static constexpr bool is_preferable = true;

    /// The type returned by queries against an @c any_executor.
    typedef blocking_t polymorphic_query_result_type;

    /// Default constructor.
    constexpr never_t();

    /// Get the value associated with a property object.
    /**
     * @returns never_t();
     */
    static constexpr blocking_t value();
  };

  /// A special value used for accessing the blocking_t::possibly_t property.
  static constexpr possibly_t possibly;

  /// A special value used for accessing the blocking_t::always_t property.
  static constexpr always_t always;

  /// A special value used for accessing the blocking_t::never_t property.
  static constexpr never_t never;

  /// Default constructor.
  constexpr blocking_t();

  /// Construct from a sub-property value.
  constexpr blocking_t(possibly_t);

  /// Construct from a sub-property value.
  constexpr blocking_t(always_t);

  /// Construct from a sub-property value.
  constexpr blocking_t(never_t);

  /// Compare property values for equality.
  friend constexpr bool operator==(
      const blocking_t& a, const blocking_t& b) noexcept;

  /// Compare property values for inequality.
  friend constexpr bool operator!=(
      const blocking_t& a, const blocking_t& b) noexcept;
};

/// A special value used for accessing the blocking_t property.
constexpr blocking_t blocking;

} // namespace execution

#else // defined(GENERATING_DOCUMENTATION)

namespace execution {
namespace detail {
namespace blocking {

template <int I> struct possibly_t;
template <int I> struct always_t;
template <int I> struct never_t;

} // namespace blocking
namespace blocking_adaptation {

template <int I> struct allowed_t;

template <typename Executor, typename Function>
void blocking_execute(
    ASIO_MOVE_ARG(Executor) ex,
    ASIO_MOVE_ARG(Function) func);

} // namespace blocking_adaptation

template <int I = 0>
struct blocking_t
{
#if defined(ASIO_HAS_VARIABLE_TEMPLATES)
  template <typename T>
  ASIO_STATIC_CONSTEXPR(bool,
    is_applicable_property_v = (
      is_executor<T>::value
        || conditional<
            is_executor<T>::value,
            false_type,
            is_sender<T>
          >::type::value
        || conditional<
            is_executor<T>::value,
            false_type,
            is_scheduler<T>
          >::type::value));
#endif // defined(ASIO_HAS_VARIABLE_TEMPLATES)

  ASIO_STATIC_CONSTEXPR(bool, is_requirable = false);
  ASIO_STATIC_CONSTEXPR(bool, is_preferable = false);
  typedef blocking_t polymorphic_query_result_type;

  typedef detail::blocking::possibly_t<I> possibly_t;
  typedef detail::blocking::always_t<I> always_t;
  typedef detail::blocking::never_t<I> never_t;

  ASIO_CONSTEXPR blocking_t()
    : value_(-1)
  {
  }

  ASIO_CONSTEXPR blocking_t(possibly_t)
    : value_(0)
  {
  }

  ASIO_CONSTEXPR blocking_t(always_t)
    : value_(1)
  {
  }

  ASIO_CONSTEXPR blocking_t(never_t)
    : value_(2)
  {
  }

  template <typename T>
  struct proxy
  {
#if defined(ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)
    struct type
    {
      template <typename P>
      auto query(ASIO_MOVE_ARG(P) p) const
        noexcept(
          noexcept(
            declval<typename conditional<true, T, P>::type>().query(
              ASIO_MOVE_CAST(P)(p))
          )
        )
        -> decltype(
          declval<typename conditional<true, T, P>::type>().query(
            ASIO_MOVE_CAST(P)(p))
        );
    };
#else // defined(ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)
    typedef T type;
#endif // defined(ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)
  };

  template <typename T>
  struct static_proxy
  {
#if defined(ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)
    struct type
    {
      template <typename P>
      static constexpr auto query(ASIO_MOVE_ARG(P) p)
        noexcept(
          noexcept(
            conditional<true, T, P>::type::query(ASIO_MOVE_CAST(P)(p))
          )
        )
        -> decltype(
          conditional<true, T, P>::type::query(ASIO_MOVE_CAST(P)(p))
        )
      {
        return T::query(ASIO_MOVE_CAST(P)(p));
      }
    };
#else // defined(ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)
    typedef T type;
#endif // defined(ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)
  };

  template <typename T>
  struct query_member :
    traits::query_member<typename proxy<T>::type, blocking_t> {};

  template <typename T>
  struct query_static_constexpr_member :
    traits::query_static_constexpr_member<
      typename static_proxy<T>::type, blocking_t> {};

#if defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
  template <typename T>
  static ASIO_CONSTEXPR
  typename query_static_constexpr_member<T>::result_type
  static_query()
    ASIO_NOEXCEPT_IF((
      query_static_constexpr_member<T>::is_noexcept))
  {
    return query_static_constexpr_member<T>::value();
  }

  template <typename T>
  static ASIO_CONSTEXPR
  typename traits::static_query<T, possibly_t>::result_type
  static_query(
      typename enable_if<
        !query_static_constexpr_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !query_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        traits::static_query<T, possibly_t>::is_valid
      >::type* = 0) ASIO_NOEXCEPT
  {
    return traits::static_query<T, possibly_t>::value();
  }

  template <typename T>
  static ASIO_CONSTEXPR
  typename traits::static_query<T, always_t>::result_type
  static_query(
      typename enable_if<
        !query_static_constexpr_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !query_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !traits::static_query<T, possibly_t>::is_valid
      >::type* = 0,
      typename enable_if<
        traits::static_query<T, always_t>::is_valid
      >::type* = 0) ASIO_NOEXCEPT
  {
    return traits::static_query<T, always_t>::value();
  }

  template <typename T>
  static ASIO_CONSTEXPR
  typename traits::static_query<T, never_t>::result_type
  static_query(
      typename enable_if<
        !query_static_constexpr_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !query_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !traits::static_query<T, possibly_t>::is_valid
      >::type* = 0,
      typename enable_if<
        !traits::static_query<T, always_t>::is_valid
      >::type* = 0,
      typename enable_if<
        traits::static_query<T, never_t>::is_valid
      >::type* = 0) ASIO_NOEXCEPT
  {
    return traits::static_query<T, never_t>::value();
  }

  template <typename E, typename T = decltype(blocking_t::static_query<E>())>
  static ASIO_CONSTEXPR const T static_query_v
    = blocking_t::static_query<E>();
#endif // defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

  friend ASIO_CONSTEXPR bool operator==(
      const blocking_t& a, const blocking_t& b)
  {
    return a.value_ == b.value_;
  }

  friend ASIO_CONSTEXPR bool operator!=(
      const blocking_t& a, const blocking_t& b)
  {
    return a.value_ != b.value_;
  }

  struct convertible_from_blocking_t
  {
    ASIO_CONSTEXPR convertible_from_blocking_t(blocking_t) {}
  };

  template <typename Executor>
  friend ASIO_CONSTEXPR blocking_t query(
      const Executor& ex, convertible_from_blocking_t,
      typename enable_if<
        can_query<const Executor&, possibly_t>::value
      >::type* = 0)
#if !defined(__clang__) // Clang crashes if noexcept is used here.
#if defined(ASIO_MSVC) // Visual C++ wants the type to be qualified.
    ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&, blocking_t<>::possibly_t>::value))
#else // defined(ASIO_MSVC)
    ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&, possibly_t>::value))
#endif // defined(ASIO_MSVC)
#endif // !defined(__clang__)
  {
    return asio::query(ex, possibly_t());
  }

  template <typename Executor>
  friend ASIO_CONSTEXPR blocking_t query(
      const Executor& ex, convertible_from_blocking_t,
      typename enable_if<
        !can_query<const Executor&, possibly_t>::value
      >::type* = 0,
      typename enable_if<
        can_query<const Executor&, always_t>::value
      >::type* = 0)
#if !defined(__clang__) // Clang crashes if noexcept is used here.
#if defined(ASIO_MSVC) // Visual C++ wants the type to be qualified.
    ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&, blocking_t<>::always_t>::value))
#else // defined(ASIO_MSVC)
    ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&, always_t>::value))
#endif // defined(ASIO_MSVC)
#endif // !defined(__clang__)
  {
    return asio::query(ex, always_t());
  }

  template <typename Executor>
  friend ASIO_CONSTEXPR blocking_t query(
      const Executor& ex, convertible_from_blocking_t,
      typename enable_if<
        !can_query<const Executor&, possibly_t>::value
      >::type* = 0,
      typename enable_if<
        !can_query<const Executor&, always_t>::value
      >::type* = 0,
      typename enable_if<
        can_query<const Executor&, never_t>::value
      >::type* = 0)
#if !defined(__clang__) // Clang crashes if noexcept is used here.
#if defined(ASIO_MSVC) // Visual C++ wants the type to be qualified.
    ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&, blocking_t<>::never_t>::value))
#else // defined(ASIO_MSVC)
    ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&, never_t>::value))
#endif // defined(ASIO_MSVC)
#endif // !defined(__clang__)
  {
    return asio::query(ex, never_t());
  }

  ASIO_STATIC_CONSTEXPR_DEFAULT_INIT(possibly_t, possibly);
  ASIO_STATIC_CONSTEXPR_DEFAULT_INIT(always_t, always);
  ASIO_STATIC_CONSTEXPR_DEFAULT_INIT(never_t, never);

#if !defined(ASIO_HAS_CONSTEXPR)
  static const blocking_t instance;
#endif // !defined(ASIO_HAS_CONSTEXPR)

private:
  int value_;
};

#if defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
template <int I> template <typename E, typename T>
const T blocking_t<I>::static_query_v;
#endif // defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

#if !defined(ASIO_HAS_CONSTEXPR)
template <int I>
const blocking_t<I> blocking_t<I>::instance;
#endif

template <int I>
const typename blocking_t<I>::possibly_t blocking_t<I>::possibly;

template <int I>
const typename blocking_t<I>::always_t blocking_t<I>::always;

template <int I>
const typename blocking_t<I>::never_t blocking_t<I>::never;

namespace blocking {

template <int I = 0>
struct possibly_t
{
#if defined(ASIO_HAS_VARIABLE_TEMPLATES)
  template <typename T>
  ASIO_STATIC_CONSTEXPR(bool,
    is_applicable_property_v = (
      is_executor<T>::value
        || conditional<
            is_executor<T>::value,
            false_type,
            is_sender<T>
          >::type::value
        || conditional<
            is_executor<T>::value,
            false_type,
            is_scheduler<T>
          >::type::value));
#endif // defined(ASIO_HAS_VARIABLE_TEMPLATES)

  ASIO_STATIC_CONSTEXPR(bool, is_requirable = true);
  ASIO_STATIC_CONSTEXPR(bool, is_preferable = true);
  typedef blocking_t<I> polymorphic_query_result_type;

  ASIO_CONSTEXPR possibly_t()
  {
  }

  template <typename T>
  struct query_member :
    traits::query_member<
      typename blocking_t<I>::template proxy<T>::type, possibly_t> {};

  template <typename T>
  struct query_static_constexpr_member :
    traits::query_static_constexpr_member<
      typename blocking_t<I>::template static_proxy<T>::type, possibly_t> {};

#if defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
  template <typename T>
  static ASIO_CONSTEXPR
  typename query_static_constexpr_member<T>::result_type
  static_query()
    ASIO_NOEXCEPT_IF((
      query_static_constexpr_member<T>::is_noexcept))
  {
    return query_static_constexpr_member<T>::value();
  }

  template <typename T>
  static ASIO_CONSTEXPR possibly_t static_query(
      typename enable_if<
        !query_static_constexpr_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !query_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !traits::query_free<T, possibly_t>::is_valid
      >::type* = 0,
      typename enable_if<
        !can_query<T, always_t<I> >::value
      >::type* = 0,
      typename enable_if<
        !can_query<T, never_t<I> >::value
      >::type* = 0) ASIO_NOEXCEPT
  {
    return possibly_t();
  }

  template <typename E, typename T = decltype(possibly_t::static_query<E>())>
  static ASIO_CONSTEXPR const T static_query_v
    = possibly_t::static_query<E>();
#endif // defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
       //   && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

  static ASIO_CONSTEXPR blocking_t<I> value()
  {
    return possibly_t();
  }

  friend ASIO_CONSTEXPR bool operator==(
      const possibly_t&, const possibly_t&)
  {
    return true;
  }

  friend ASIO_CONSTEXPR bool operator!=(
      const possibly_t&, const possibly_t&)
  {
    return false;
  }

  friend ASIO_CONSTEXPR bool operator==(
      const possibly_t&, const always_t<I>&)
  {
    return false;
  }

  friend ASIO_CONSTEXPR bool operator!=(
      const possibly_t&, const always_t<I>&)
  {
    return true;
  }

  friend ASIO_CONSTEXPR bool operator==(
      const possibly_t&, const never_t<I>&)
  {
    return false;
  }

  friend ASIO_CONSTEXPR bool operator!=(
      const possibly_t&, const never_t<I>&)
  {
    return true;
  }
};

#if defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
template <int I> template <typename E, typename T>
const T possibly_t<I>::static_query_v;
#endif // defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

template <typename Executor>
class adapter
{
public:
  adapter(int, const Executor& e) ASIO_NOEXCEPT
    : executor_(e)
  {
  }

  adapter(const adapter& other) ASIO_NOEXCEPT
    : executor_(other.executor_)
  {
  }

#if defined(ASIO_HAS_MOVE)
  adapter(adapter&& other) ASIO_NOEXCEPT
    : executor_(ASIO_MOVE_CAST(Executor)(other.executor_))
  {
  }
#endif // defined(ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)

  template <int I>
  static ASIO_CONSTEXPR always_t<I> query(
      blocking_t<I>) ASIO_NOEXCEPT
  {
    return always_t<I>();
  }

  template <int I>
  static ASIO_CONSTEXPR always_t<I> query(
      possibly_t<I>) ASIO_NOEXCEPT
  {
    return always_t<I>();
  }

  template <int I>
  static ASIO_CONSTEXPR always_t<I> query(
      always_t<I>) ASIO_NOEXCEPT
  {
    return always_t<I>();
  }

  template <int I>
  static ASIO_CONSTEXPR always_t<I> query(
      never_t<I>) ASIO_NOEXCEPT
  {
    return always_t<I>();
  }

  template <typename Property>
  typename enable_if<
    can_query<const Executor&, Property>::value,
    typename query_result<const Executor&, Property>::type
  >::type query(const Property& p) const
    ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&, Property>::value))
  {
    return asio::query(executor_, p);
  }

  template <int I>
  typename enable_if<
    can_require<const Executor&, possibly_t<I> >::value,
    typename require_result<const Executor&, possibly_t<I> >::type
  >::type require(possibly_t<I>) const ASIO_NOEXCEPT
  {
    return asio::require(executor_, possibly_t<I>());
  }

  template <int I>
  typename enable_if<
    can_require<const Executor&, never_t<I> >::value,
    typename require_result<const Executor&, never_t<I> >::type
  >::type require(never_t<I>) const ASIO_NOEXCEPT
  {
    return asio::require(executor_, never_t<I>());
  }

  template <typename Property>
  typename enable_if<
    can_require<const Executor&, Property>::value,
    adapter<typename decay<
      typename require_result<const Executor&, Property>::type
    >::type>
  >::type require(const Property& p) const
    ASIO_NOEXCEPT_IF((
      is_nothrow_require<const Executor&, Property>::value))
  {
    return adapter<typename decay<
      typename require_result<const Executor&, Property>::type
        >::type>(0, asio::require(executor_, p));
  }

  template <typename Property>
  typename enable_if<
    can_prefer<const Executor&, Property>::value,
    adapter<typename decay<
      typename prefer_result<const Executor&, Property>::type
    >::type>
  >::type prefer(const Property& p) const
    ASIO_NOEXCEPT_IF((
      is_nothrow_prefer<const Executor&, Property>::value))
  {
    return adapter<typename decay<
      typename prefer_result<const Executor&, Property>::type
        >::type>(0, asio::prefer(executor_, p));
  }

  template <typename Function>
  typename enable_if<
    execution::can_execute<const Executor&, Function>::value
  >::type execute(ASIO_MOVE_ARG(Function) f) const
  {
    blocking_adaptation::blocking_execute(
        executor_, ASIO_MOVE_CAST(Function)(f));
  }

  friend bool operator==(const adapter& a, const adapter& b) ASIO_NOEXCEPT
  {
    return a.executor_ == b.executor_;
  }

  friend bool operator!=(const adapter& a, const adapter& b) ASIO_NOEXCEPT
  {
    return a.executor_ != b.executor_;
  }

private:
  Executor executor_;
};

template <int I = 0>
struct always_t
{
#if defined(ASIO_HAS_VARIABLE_TEMPLATES)
  template <typename T>
  ASIO_STATIC_CONSTEXPR(bool,
    is_applicable_property_v = (
      is_executor<T>::value
        || conditional<
            is_executor<T>::value,
            false_type,
            is_sender<T>
          >::type::value
        || conditional<
            is_executor<T>::value,
            false_type,
            is_scheduler<T>
          >::type::value));
#endif // defined(ASIO_HAS_VARIABLE_TEMPLATES)

  ASIO_STATIC_CONSTEXPR(bool, is_requirable = true);
  ASIO_STATIC_CONSTEXPR(bool, is_preferable = false);
  typedef blocking_t<I> polymorphic_query_result_type;

  ASIO_CONSTEXPR always_t()
  {
  }

  template <typename T>
  struct query_member :
    traits::query_member<
      typename blocking_t<I>::template proxy<T>::type, always_t> {};

  template <typename T>
  struct query_static_constexpr_member :
    traits::query_static_constexpr_member<
      typename blocking_t<I>::template static_proxy<T>::type, always_t> {};

#if defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
  template <typename T>
  static ASIO_CONSTEXPR
  typename query_static_constexpr_member<T>::result_type
  static_query()
    ASIO_NOEXCEPT_IF((
      query_static_constexpr_member<T>::is_noexcept))
  {
    return query_static_constexpr_member<T>::value();
  }

  template <typename E, typename T = decltype(always_t::static_query<E>())>
  static ASIO_CONSTEXPR const T static_query_v
    = always_t::static_query<E>();
#endif // defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
       //   && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

  static ASIO_CONSTEXPR blocking_t<I> value()
  {
    return always_t();
  }

  friend ASIO_CONSTEXPR bool operator==(
      const always_t&, const always_t&)
  {
    return true;
  }

  friend ASIO_CONSTEXPR bool operator!=(
      const always_t&, const always_t&)
  {
    return false;
  }

  friend ASIO_CONSTEXPR bool operator==(
      const always_t&, const possibly_t<I>&)
  {
    return false;
  }

  friend ASIO_CONSTEXPR bool operator!=(
      const always_t&, const possibly_t<I>&)
  {
    return true;
  }

  friend ASIO_CONSTEXPR bool operator==(
      const always_t&, const never_t<I>&)
  {
    return false;
  }

  friend ASIO_CONSTEXPR bool operator!=(
      const always_t&, const never_t<I>&)
  {
    return true;
  }

  template <typename Executor>
  friend adapter<Executor> require(
      const Executor& e, const always_t&,
      typename enable_if<
        is_executor<Executor>::value
      >::type* = 0,
      typename enable_if<
        traits::static_require<
          const Executor&,
          blocking_adaptation::allowed_t<0>
        >::is_valid
      >::type* = 0)
  {
    return adapter<Executor>(0, e);
  }
};

#if defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
template <int I> template <typename E, typename T>
const T always_t<I>::static_query_v;
#endif // defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

template <int I>
struct never_t
{
#if defined(ASIO_HAS_VARIABLE_TEMPLATES)
  template <typename T>
  ASIO_STATIC_CONSTEXPR(bool,
    is_applicable_property_v = (
      is_executor<T>::value
        || conditional<
            is_executor<T>::value,
            false_type,
            is_sender<T>
          >::type::value
        || conditional<
            is_executor<T>::value,
            false_type,
            is_scheduler<T>
          >::type::value));
#endif // defined(ASIO_HAS_VARIABLE_TEMPLATES)

  ASIO_STATIC_CONSTEXPR(bool, is_requirable = true);
  ASIO_STATIC_CONSTEXPR(bool, is_preferable = true);
  typedef blocking_t<I> polymorphic_query_result_type;

  ASIO_CONSTEXPR never_t()
  {
  }

  template <typename T>
  struct query_member :
    traits::query_member<
      typename blocking_t<I>::template proxy<T>::type, never_t> {};

  template <typename T>
  struct query_static_constexpr_member :
    traits::query_static_constexpr_member<
      typename blocking_t<I>::template static_proxy<T>::type, never_t> {};

#if defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
  template <typename T>
  static ASIO_CONSTEXPR
  typename query_static_constexpr_member<T>::result_type
  static_query()
    ASIO_NOEXCEPT_IF((
      query_static_constexpr_member<T>::is_noexcept))
  {
    return query_static_constexpr_member<T>::value();
  }

  template <typename E, typename T = decltype(never_t::static_query<E>())>
  static ASIO_CONSTEXPR const T static_query_v
    = never_t::static_query<E>();
#endif // defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

  static ASIO_CONSTEXPR blocking_t<I> value()
  {
    return never_t();
  }

  friend ASIO_CONSTEXPR bool operator==(
      const never_t&, const never_t&)
  {
    return true;
  }

  friend ASIO_CONSTEXPR bool operator!=(
      const never_t&, const never_t&)
  {
    return false;
  }

  friend ASIO_CONSTEXPR bool operator==(
      const never_t&, const possibly_t<I>&)
  {
    return false;
  }

  friend ASIO_CONSTEXPR bool operator!=(
      const never_t&, const possibly_t<I>&)
  {
    return true;
  }

  friend ASIO_CONSTEXPR bool operator==(
      const never_t&, const always_t<I>&)
  {
    return false;
  }

  friend ASIO_CONSTEXPR bool operator!=(
      const never_t&, const always_t<I>&)
  {
    return true;
  }
};

#if defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
template <int I> template <typename E, typename T>
const T never_t<I>::static_query_v;
#endif // defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

} // namespace blocking
} // namespace detail

typedef detail::blocking_t<> blocking_t;

#if defined(ASIO_HAS_CONSTEXPR) || defined(GENERATING_DOCUMENTATION)
constexpr blocking_t blocking;
#else // defined(ASIO_HAS_CONSTEXPR) || defined(GENERATING_DOCUMENTATION)
namespace { static const blocking_t& blocking = blocking_t::instance; }
#endif

} // namespace execution

#if !defined(ASIO_HAS_VARIABLE_TEMPLATES)

template <typename T>
struct is_applicable_property<T, execution::blocking_t>
  : integral_constant<bool,
      execution::is_executor<T>::value
        || conditional<
            execution::is_executor<T>::value,
            false_type,
            execution::is_sender<T>
          >::type::value
        || conditional<
            execution::is_executor<T>::value,
            false_type,
            execution::is_scheduler<T>
          >::type::value>
{
};

template <typename T>
struct is_applicable_property<T, execution::blocking_t::possibly_t>
  : integral_constant<bool,
      execution::is_executor<T>::value
        || conditional<
            execution::is_executor<T>::value,
            false_type,
            execution::is_sender<T>
          >::type::value
        || conditional<
            execution::is_executor<T>::value,
            false_type,
            execution::is_scheduler<T>
          >::type::value>
{
};

template <typename T>
struct is_applicable_property<T, execution::blocking_t::always_t>
  : integral_constant<bool,
      execution::is_executor<T>::value
        || conditional<
            execution::is_executor<T>::value,
            false_type,
            execution::is_sender<T>
          >::type::value
        || conditional<
            execution::is_executor<T>::value,
            false_type,
            execution::is_scheduler<T>
          >::type::value>
{
};

template <typename T>
struct is_applicable_property<T, execution::blocking_t::never_t>
  : integral_constant<bool,
      execution::is_executor<T>::value
        || conditional<
            execution::is_executor<T>::value,
            false_type,
            execution::is_sender<T>
          >::type::value
        || conditional<
            execution::is_executor<T>::value,
            false_type,
            execution::is_scheduler<T>
          >::type::value>
{
};

#endif // !defined(ASIO_HAS_VARIABLE_TEMPLATES)

namespace traits {

#if !defined(ASIO_HAS_DEDUCED_QUERY_FREE_TRAIT)

template <typename T>
struct query_free_default<T, execution::blocking_t,
  typename enable_if<
    can_query<T, execution::blocking_t::possibly_t>::value
  >::type>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
    (is_nothrow_query<T, execution::blocking_t::possibly_t>::value));

  typedef execution::blocking_t result_type;
};

template <typename T>
struct query_free_default<T, execution::blocking_t,
  typename enable_if<
    !can_query<T, execution::blocking_t::possibly_t>::value
      && can_query<T, execution::blocking_t::always_t>::value
  >::type>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
    (is_nothrow_query<T, execution::blocking_t::always_t>::value));

  typedef execution::blocking_t result_type;
};

template <typename T>
struct query_free_default<T, execution::blocking_t,
  typename enable_if<
    !can_query<T, execution::blocking_t::possibly_t>::value
      && !can_query<T, execution::blocking_t::always_t>::value
      && can_query<T, execution::blocking_t::never_t>::value
  >::type>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
    (is_nothrow_query<T, execution::blocking_t::never_t>::value));

  typedef execution::blocking_t result_type;
};

#endif // !defined(ASIO_HAS_DEDUCED_QUERY_FREE_TRAIT)

#if !defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  || !defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

template <typename T>
struct static_query<T, execution::blocking_t,
  typename enable_if<
    execution::detail::blocking_t<0>::
      query_static_constexpr_member<T>::is_valid
  >::type>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename execution::detail::blocking_t<0>::
    query_static_constexpr_member<T>::result_type result_type;

  static ASIO_CONSTEXPR result_type value()
  {
    return execution::blocking_t::query_static_constexpr_member<T>::value();
  }
};

template <typename T>
struct static_query<T, execution::blocking_t,
  typename enable_if<
    !execution::detail::blocking_t<0>::
        query_static_constexpr_member<T>::is_valid
      && !execution::detail::blocking_t<0>::
        query_member<T>::is_valid
      && traits::static_query<T, execution::blocking_t::possibly_t>::is_valid
  >::type>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename traits::static_query<T,
    execution::blocking_t::possibly_t>::result_type result_type;

  static ASIO_CONSTEXPR result_type value()
  {
    return traits::static_query<T, execution::blocking_t::possibly_t>::value();
  }
};

template <typename T>
struct static_query<T, execution::blocking_t,
  typename enable_if<
    !execution::detail::blocking_t<0>::
        query_static_constexpr_member<T>::is_valid
      && !execution::detail::blocking_t<0>::
        query_member<T>::is_valid
      && !traits::static_query<T, execution::blocking_t::possibly_t>::is_valid
      && traits::static_query<T, execution::blocking_t::always_t>::is_valid
  >::type>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename traits::static_query<T,
    execution::blocking_t::always_t>::result_type result_type;

  static ASIO_CONSTEXPR result_type value()
  {
    return traits::static_query<T, execution::blocking_t::always_t>::value();
  }
};

template <typename T>
struct static_query<T, execution::blocking_t,
  typename enable_if<
    !execution::detail::blocking_t<0>::
        query_static_constexpr_member<T>::is_valid
      && !execution::detail::blocking_t<0>::
        query_member<T>::is_valid
      && !traits::static_query<T, execution::blocking_t::possibly_t>::is_valid
      && !traits::static_query<T, execution::blocking_t::always_t>::is_valid
      && traits::static_query<T, execution::blocking_t::never_t>::is_valid
  >::type>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename traits::static_query<T,
    execution::blocking_t::never_t>::result_type result_type;

  static ASIO_CONSTEXPR result_type value()
  {
    return traits::static_query<T, execution::blocking_t::never_t>::value();
  }
};

template <typename T>
struct static_query<T, execution::blocking_t::possibly_t,
  typename enable_if<
    execution::detail::blocking::possibly_t<0>::
      query_static_constexpr_member<T>::is_valid
  >::type>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename execution::detail::blocking::possibly_t<0>::
    query_static_constexpr_member<T>::result_type result_type;

  static ASIO_CONSTEXPR result_type value()
  {
    return execution::detail::blocking::possibly_t<0>::
      query_static_constexpr_member<T>::value();
  }
};

template <typename T>
struct static_query<T, execution::blocking_t::possibly_t,
  typename enable_if<
    !execution::detail::blocking::possibly_t<0>::
        query_static_constexpr_member<T>::is_valid
      && !execution::detail::blocking::possibly_t<0>::
        query_member<T>::is_valid
      && !traits::query_free<T, execution::blocking_t::possibly_t>::is_valid
      && !can_query<T, execution::blocking_t::always_t>::value
      && !can_query<T, execution::blocking_t::never_t>::value
  >::type>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef execution::blocking_t::possibly_t result_type;

  static ASIO_CONSTEXPR result_type value()
  {
    return result_type();
  }
};

template <typename T>
struct static_query<T, execution::blocking_t::always_t,
  typename enable_if<
    execution::detail::blocking::always_t<0>::
      query_static_constexpr_member<T>::is_valid
  >::type>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename execution::detail::blocking::always_t<0>::
    query_static_constexpr_member<T>::result_type result_type;

  static ASIO_CONSTEXPR result_type value()
  {
    return execution::detail::blocking::always_t<0>::
      query_static_constexpr_member<T>::value();
  }
};

template <typename T>
struct static_query<T, execution::blocking_t::never_t,
  typename enable_if<
    execution::detail::blocking::never_t<0>::
      query_static_constexpr_member<T>::is_valid
  >::type>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename execution::detail::blocking::never_t<0>::
    query_static_constexpr_member<T>::result_type result_type;

  static ASIO_CONSTEXPR result_type value()
  {
    return execution::detail::blocking::never_t<0>::
      query_static_constexpr_member<T>::value();
  }
};

#endif // !defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   || !defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

#if !defined(ASIO_HAS_DEDUCED_STATIC_REQUIRE_TRAIT)

template <typename T>
struct static_require<T, execution::blocking_t::possibly_t,
  typename enable_if<
    static_query<T, execution::blocking_t::possibly_t>::is_valid
  >::type>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid =
    (is_same<typename static_query<T,
      execution::blocking_t::possibly_t>::result_type,
        execution::blocking_t::possibly_t>::value));
};

template <typename T>
struct static_require<T, execution::blocking_t::always_t,
  typename enable_if<
    static_query<T, execution::blocking_t::always_t>::is_valid
  >::type>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid =
    (is_same<typename static_query<T,
      execution::blocking_t::always_t>::result_type,
        execution::blocking_t::always_t>::value));
};

template <typename T>
struct static_require<T, execution::blocking_t::never_t,
  typename enable_if<
    static_query<T, execution::blocking_t::never_t>::is_valid
  >::type>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid =
    (is_same<typename static_query<T,
      execution::blocking_t::never_t>::result_type,
        execution::blocking_t::never_t>::value));
};

#endif // !defined(ASIO_HAS_DEDUCED_STATIC_REQUIRE_TRAIT)

#if !defined(ASIO_HAS_DEDUCED_REQUIRE_FREE_TRAIT)

template <typename T>
struct require_free_default<T, execution::blocking_t::always_t,
  typename enable_if<
    is_same<T, typename decay<T>::type>::value
      && execution::is_executor<T>::value
      && traits::static_require<
          const T&,
          execution::detail::blocking_adaptation::allowed_t<0>
        >::is_valid
  >::type>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef execution::detail::blocking::adapter<T> result_type;
};

#endif // !defined(ASIO_HAS_DEDUCED_REQUIRE_FREE_TRAIT)

#if !defined(ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)

template <typename Executor>
struct equality_comparable<
  execution::detail::blocking::adapter<Executor> >
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
};

#endif // !defined(ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)

#if !defined(ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)

template <typename Executor, typename Function>
struct execute_member<
  execution::detail::blocking::adapter<Executor>, Function>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef void result_type;
};

#endif // !defined(ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)

#if !defined(ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)

template <typename Executor, int I>
struct query_static_constexpr_member<
  execution::detail::blocking::adapter<Executor>,
  execution::detail::blocking_t<I> >
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef execution::blocking_t::always_t result_type;

  static ASIO_CONSTEXPR result_type value() ASIO_NOEXCEPT
  {
    return result_type();
  }
};

template <typename Executor, int I>
struct query_static_constexpr_member<
  execution::detail::blocking::adapter<Executor>,
  execution::detail::blocking::always_t<I> >
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef execution::blocking_t::always_t result_type;

  static ASIO_CONSTEXPR result_type value() ASIO_NOEXCEPT
  {
    return result_type();
  }
};

template <typename Executor, int I>
struct query_static_constexpr_member<
  execution::detail::blocking::adapter<Executor>,
  execution::detail::blocking::possibly_t<I> >
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef execution::blocking_t::always_t result_type;

  static ASIO_CONSTEXPR result_type value() ASIO_NOEXCEPT
  {
    return result_type();
  }
};

template <typename Executor, int I>
struct query_static_constexpr_member<
  execution::detail::blocking::adapter<Executor>,
  execution::detail::blocking::never_t<I> >
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef execution::blocking_t::always_t result_type;

  static ASIO_CONSTEXPR result_type value() ASIO_NOEXCEPT
  {
    return result_type();
  }
};

#endif // !defined(ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)

#if !defined(ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)

template <typename Executor, typename Property>
struct query_member<
  execution::detail::blocking::adapter<Executor>, Property,
  typename enable_if<
    can_query<const Executor&, Property>::value
  >::type>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
      (is_nothrow_query<Executor, Property>::value));
  typedef typename query_result<Executor, Property>::type result_type;
};

#endif // !defined(ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)

#if !defined(ASIO_HAS_DEDUCED_REQUIRE_MEMBER_TRAIT)

template <typename Executor, int I>
struct require_member<
  execution::detail::blocking::adapter<Executor>,
  execution::detail::blocking::possibly_t<I>,
  typename enable_if<
    can_require<
      const Executor&,
      execution::detail::blocking::possibly_t<I>
    >::value
  >::type>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
      (is_nothrow_require<const Executor&,
        execution::detail::blocking::possibly_t<I> >::value));
  typedef typename require_result<const Executor&,
    execution::detail::blocking::possibly_t<I> >::type result_type;
};

template <typename Executor, int I>
struct require_member<
  execution::detail::blocking::adapter<Executor>,
  execution::detail::blocking::never_t<I>,
  typename enable_if<
    can_require<
      const Executor&,
      execution::detail::blocking::never_t<I>
    >::value
  >::type>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
      (is_nothrow_require<const Executor&,
        execution::detail::blocking::never_t<I> >::value));
  typedef typename require_result<const Executor&,
    execution::detail::blocking::never_t<I> >::type result_type;
};

template <typename Executor, typename Property>
struct require_member<
  execution::detail::blocking::adapter<Executor>, Property,
  typename enable_if<
    can_require<const Executor&, Property>::value
  >::type>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
      (is_nothrow_require<Executor, Property>::value));
  typedef execution::detail::blocking::adapter<typename decay<
    typename require_result<Executor, Property>::type
      >::type> result_type;
};

#endif // !defined(ASIO_HAS_DEDUCED_REQUIRE_MEMBER_TRAIT)

#if !defined(ASIO_HAS_DEDUCED_PREFER_MEMBER_TRAIT)

template <typename Executor, typename Property>
struct prefer_member<
  execution::detail::blocking::adapter<Executor>, Property,
  typename enable_if<
    can_prefer<const Executor&, Property>::value
  >::type>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
      (is_nothrow_prefer<Executor, Property>::value));
  typedef execution::detail::blocking::adapter<typename decay<
    typename prefer_result<Executor, Property>::type
      >::type> result_type;
};

#endif // !defined(ASIO_HAS_DEDUCED_PREFER_MEMBER_TRAIT)

} // namespace traits

#endif // defined(GENERATING_DOCUMENTATION)

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_EXECUTION_BLOCKING_HPP
