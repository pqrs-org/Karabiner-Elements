//
// execution/inline_exception_handling.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2025 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_EXECUTION_INLINE_EXCEPTION_HANDLING_HPP
#define ASIO_EXECUTION_INLINE_EXCEPTION_HANDLING_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/execution/executor.hpp"
#include "asio/is_applicable_property.hpp"
#include "asio/prefer.hpp"
#include "asio/query.hpp"
#include "asio/require.hpp"
#include "asio/traits/execute_member.hpp"
#include "asio/traits/query_free.hpp"
#include "asio/traits/query_member.hpp"
#include "asio/traits/query_static_constexpr_member.hpp"
#include "asio/traits/static_query.hpp"
#include "asio/traits/static_require.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {

#if defined(GENERATING_DOCUMENTATION)

namespace execution {

/// A property to describe what guarantees an executor makes about the treatment
/// of exceptions that are thrown by a submitted function, when that function
/// is executed inline within @c execute.
struct inline_exception_handling_t
{
  /// The inline_exception_handling property applies to executors.
  template <typename T>
  static constexpr bool is_applicable_property_v = is_executor_v<T>;

  /// The top-level inline_exception_handling property cannot be required.
  static constexpr bool is_requirable = false;

  /// The top-level inline_exception_handling property cannot be preferred.
  static constexpr bool is_preferable = false;

  /// The type returned by queries against an @c any_executor.
  typedef inline_exception_handling_t polymorphic_query_result_type;

  /// A sub-property that indicates that invocation of an executor's execution
  /// function will propagate any exceptions that are thrown by the submitted
  /// function object, if that function object is executed inline.
  struct propagate_t
  {
    /// The inline_exception_handling_t::propagate_t property applies to
    /// executors.
    template <typename T>
    static constexpr bool is_applicable_property_v = is_executor_v<T>;

    /// The inline_exception_handling_t::propagate_t property can be required.
    static constexpr bool is_requirable = true;

    /// The inline_exception_handling_t::propagate_t property can be preferred.
    static constexpr bool is_preferable = true;

    /// The type returned by queries against an @c any_executor.
    typedef inline_exception_handling_t polymorphic_query_result_type;

    /// Default constructor.
    constexpr propagate_t();

    /// Get the value associated with a property object.
    /**
     * @returns propagate_t();
     */
    static constexpr inline_exception_handling_t value();
  };

  /// A sub-property that indicates that invocation of an executor's execution
  /// function will capture any exceptions that are thrown by the submitted
  /// function object, if that function object is executed inline. Captured
  /// exceptions are forwarded to an executor-defined handling mechanism.
  struct capture_t
  {
    /// The inline_exception_handling_t::capture_t property applies to
    /// executors.
    template <typename T>
    static constexpr bool is_applicable_property_v = is_executor_v<T>;

    /// The inline_exception_handling_t::capture_t property can be required.
    static constexpr bool is_requirable = true;

    /// The inline_exception_handling_t::capture_t property can be preferred.
    static constexpr bool is_preferable = false;

    /// The type returned by queries against an @c any_executor.
    typedef inline_exception_handling_t polymorphic_query_result_type;

    /// Default constructor.
    constexpr capture_t();

    /// Get the value associated with a property object.
    /**
     * @returns capture_t();
     */
    static constexpr inline_exception_handling_t value();
  };

  /// A sub-property that indicates that invocation of an executor's execution
  /// function will terminate the program if any exceptions that are thrown by
  /// the submitted function object, if that function object is executed inline.
  struct terminate_t
  {
    /// The inline_exception_handling_t::terminate_t property applies to
    /// executors.
    template <typename T>
    static constexpr bool is_applicable_property_v = is_executor_v<T>;

    /// The inline_exception_handling_t::terminate_t property can be required.
    static constexpr bool is_requirable = true;

    /// The inline_exception_handling_t::terminate_t property can be preferred.
    static constexpr bool is_preferable = true;

    /// The type returned by queries against an @c any_executor.
    typedef inline_exception_handling_t polymorphic_query_result_type;

    /// Default constructor.
    constexpr terminate_t();

    /// Get the value associated with a property object.
    /**
     * @returns terminate_t();
     */
    static constexpr inline_exception_handling_t value();
  };

  /// A special value used for accessing the
  /// inline_exception_handling_t::propagate_t property.
  static constexpr propagate_t propagate;

  /// A special value used for accessing the
  /// inline_exception_handling_t::capture_t property.
  static constexpr capture_t capture;

  /// A special value used for accessing the
  /// inline_exception_handling_t::terminate_t property.
  static constexpr terminate_t terminate;

  /// Default constructor.
  constexpr inline_exception_handling_t();

  /// Construct from a sub-property value.
  constexpr inline_exception_handling_t(propagate_t);

  /// Construct from a sub-property value.
  constexpr inline_exception_handling_t(capture_t);

  /// Construct from a sub-property value.
  constexpr inline_exception_handling_t(terminate_t);

  /// Compare property values for equality.
  friend constexpr bool operator==(const inline_exception_handling_t& a,
    const inline_exception_handling_t& b) noexcept;

  /// Compare property values for inequality.
  friend constexpr bool operator!=(const inline_exception_handling_t& a,
    const inline_exception_handling_t& b) noexcept;
};

/// A special value used for accessing the inline_exception_handling property.
constexpr inline_exception_handling_t inline_exception_handling;

} // namespace execution

#else // defined(GENERATING_DOCUMENTATION)

namespace execution {
namespace detail {
namespace inline_exception_handling {

template <int I> struct propagate_t;
template <int I> struct capture_t;
template <int I> struct terminate_t;

} // namespace inline_exception_handling

template <int I = 0>
struct inline_exception_handling_t
{
#if defined(ASIO_HAS_VARIABLE_TEMPLATES)
  template <typename T>
  static constexpr bool is_applicable_property_v = is_executor<T>::value;
#endif // defined(ASIO_HAS_VARIABLE_TEMPLATES)

  static constexpr bool is_requirable = false;
  static constexpr bool is_preferable = false;
  typedef inline_exception_handling_t polymorphic_query_result_type;

  typedef detail::inline_exception_handling::propagate_t<I> propagate_t;
  typedef detail::inline_exception_handling::capture_t<I> capture_t;
  typedef detail::inline_exception_handling::terminate_t<I> terminate_t;

  constexpr inline_exception_handling_t()
    : value_(-1)
  {
  }

  constexpr inline_exception_handling_t(propagate_t)
    : value_(0)
  {
  }

  constexpr inline_exception_handling_t(capture_t)
    : value_(1)
  {
  }

  constexpr inline_exception_handling_t(terminate_t)
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
      auto query(P&& p) const
        noexcept(
          noexcept(
            declval<conditional_t<true, T, P>>().query(static_cast<P&&>(p))
          )
        )
        -> decltype(
          declval<conditional_t<true, T, P>>().query(static_cast<P&&>(p))
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
      static constexpr auto query(P&& p)
        noexcept(
          noexcept(
            conditional_t<true, T, P>::query(static_cast<P&&>(p))
          )
        )
        -> decltype(
          conditional_t<true, T, P>::query(static_cast<P&&>(p))
        )
      {
        return T::query(static_cast<P&&>(p));
      }
    };
#else // defined(ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)
    typedef T type;
#endif // defined(ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)
  };

  template <typename T>
  struct query_member :
    traits::query_member<typename proxy<T>::type,
      inline_exception_handling_t> {};

  template <typename T>
  struct query_static_constexpr_member :
    traits::query_static_constexpr_member<
      typename static_proxy<T>::type, inline_exception_handling_t> {};

#if defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
  template <typename T>
  static constexpr
  typename query_static_constexpr_member<T>::result_type
  static_query()
    noexcept(query_static_constexpr_member<T>::is_noexcept)
  {
    return query_static_constexpr_member<T>::value();
  }

  template <typename T>
  static constexpr
  typename traits::static_query<T, propagate_t>::result_type
  static_query(
      enable_if_t<
        !query_static_constexpr_member<T>::is_valid
      >* = 0,
      enable_if_t<
        !query_member<T>::is_valid
      >* = 0,
      enable_if_t<
        traits::static_query<T, propagate_t>::is_valid
      >* = 0) noexcept
  {
    return traits::static_query<T, propagate_t>::value();
  }

  template <typename T>
  static constexpr
  typename traits::static_query<T, capture_t>::result_type
  static_query(
      enable_if_t<
        !query_static_constexpr_member<T>::is_valid
      >* = 0,
      enable_if_t<
        !query_member<T>::is_valid
      >* = 0,
      enable_if_t<
        !traits::static_query<T, propagate_t>::is_valid
      >* = 0,
      enable_if_t<
        traits::static_query<T, capture_t>::is_valid
      >* = 0) noexcept
  {
    return traits::static_query<T, capture_t>::value();
  }

  template <typename T>
  static constexpr
  typename traits::static_query<T, terminate_t>::result_type
  static_query(
      enable_if_t<
        !query_static_constexpr_member<T>::is_valid
      >* = 0,
      enable_if_t<
        !query_member<T>::is_valid
      >* = 0,
      enable_if_t<
        !traits::static_query<T, propagate_t>::is_valid
      >* = 0,
      enable_if_t<
        !traits::static_query<T, capture_t>::is_valid
      >* = 0,
      enable_if_t<
        traits::static_query<T, terminate_t>::is_valid
      >* = 0) noexcept
  {
    return traits::static_query<T, terminate_t>::value();
  }

  template <typename E,
      typename T = decltype(inline_exception_handling_t::static_query<E>())>
  static constexpr const T static_query_v
    = inline_exception_handling_t::static_query<E>();
#endif // defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

  friend constexpr bool operator==(const inline_exception_handling_t& a,
      const inline_exception_handling_t& b)
  {
    return a.value_ == b.value_;
  }

  friend constexpr bool operator!=(const inline_exception_handling_t& a,
      const inline_exception_handling_t& b)
  {
    return a.value_ != b.value_;
  }

  struct convertible_from_inline_exception_handling_t
  {
    constexpr convertible_from_inline_exception_handling_t(
        inline_exception_handling_t) {}
  };

  template <typename Executor>
  friend constexpr inline_exception_handling_t query(
      const Executor& ex, convertible_from_inline_exception_handling_t,
      enable_if_t<
        can_query<const Executor&, propagate_t>::value
      >* = 0)
#if !defined(__clang__) // Clang crashes if noexcept is used here.
#if defined(ASIO_MSVC) // Visual C++ wants the type to be qualified.
    noexcept(is_nothrow_query<const Executor&,
        inline_exception_handling_t<>::propagate_t>::value)
#else // defined(ASIO_MSVC)
    noexcept(is_nothrow_query<const Executor&, propagate_t>::value)
#endif // defined(ASIO_MSVC)
#endif // !defined(__clang__)
  {
    return asio::query(ex, propagate_t());
  }

  template <typename Executor>
  friend constexpr inline_exception_handling_t query(
      const Executor& ex, convertible_from_inline_exception_handling_t,
      enable_if_t<
        !can_query<const Executor&, propagate_t>::value
      >* = 0,
      enable_if_t<
        can_query<const Executor&, capture_t>::value
      >* = 0)
#if !defined(__clang__) // Clang crashes if noexcept is used here.
#if defined(ASIO_MSVC) // Visual C++ wants the type to be qualified.
    noexcept(is_nothrow_query<const Executor&,
        inline_exception_handling_t<>::capture_t>::value)
#else // defined(ASIO_MSVC)
    noexcept(is_nothrow_query<const Executor&, capture_t>::value)
#endif // defined(ASIO_MSVC)
#endif // !defined(__clang__)
  {
    return asio::query(ex, capture_t());
  }

  template <typename Executor>
  friend constexpr inline_exception_handling_t query(
      const Executor& ex, convertible_from_inline_exception_handling_t,
      enable_if_t<
        !can_query<const Executor&, propagate_t>::value
      >* = 0,
      enable_if_t<
        !can_query<const Executor&, capture_t>::value
      >* = 0,
      enable_if_t<
        can_query<const Executor&, terminate_t>::value
      >* = 0)
#if !defined(__clang__) // Clang crashes if noexcept is used here.
#if defined(ASIO_MSVC) // Visual C++ wants the type to be qualified.
    noexcept(is_nothrow_query<const Executor&,
        inline_exception_handling_t<>::terminate_t>::value)
#else // defined(ASIO_MSVC)
    noexcept(is_nothrow_query<const Executor&, terminate_t>::value)
#endif // defined(ASIO_MSVC)
#endif // !defined(__clang__)
  {
    return asio::query(ex, terminate_t());
  }

  ASIO_STATIC_CONSTEXPR_DEFAULT_INIT(propagate_t, propagate);
  ASIO_STATIC_CONSTEXPR_DEFAULT_INIT(capture_t, capture);
  ASIO_STATIC_CONSTEXPR_DEFAULT_INIT(terminate_t, terminate);

private:
  int value_;
};

#if defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
template <int I> template <typename E, typename T>
const T inline_exception_handling_t<I>::static_query_v;
#endif // defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

template <int I>
const typename inline_exception_handling_t<I>::propagate_t
inline_exception_handling_t<I>::propagate;

template <int I>
const typename inline_exception_handling_t<I>::capture_t
inline_exception_handling_t<I>::capture;

template <int I>
const typename inline_exception_handling_t<I>::terminate_t
inline_exception_handling_t<I>::terminate;

namespace inline_exception_handling {

template <int I = 0>
struct propagate_t
{
#if defined(ASIO_HAS_VARIABLE_TEMPLATES)
  template <typename T>
  static constexpr bool is_applicable_property_v = is_executor<T>::value;
#endif // defined(ASIO_HAS_VARIABLE_TEMPLATES)

  static constexpr bool is_requirable = true;
  static constexpr bool is_preferable = true;
  typedef inline_exception_handling_t<I> polymorphic_query_result_type;

  constexpr propagate_t()
  {
  }

  template <typename T>
  struct query_member :
    traits::query_member<
      typename inline_exception_handling_t<I>::template
        proxy<T>::type, propagate_t> {};

  template <typename T>
  struct query_static_constexpr_member :
    traits::query_static_constexpr_member<
      typename inline_exception_handling_t<I>::template
        static_proxy<T>::type, propagate_t> {};

#if defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
  template <typename T>
  static constexpr
  typename query_static_constexpr_member<T>::result_type
  static_query()
    noexcept(query_static_constexpr_member<T>::is_noexcept)
  {
    return query_static_constexpr_member<T>::value();
  }

  template <typename T>
  static constexpr propagate_t static_query(
      enable_if_t<
        !query_static_constexpr_member<T>::is_valid
      >* = 0,
      enable_if_t<
        !query_member<T>::is_valid
      >* = 0,
      enable_if_t<
        !traits::query_free<T, propagate_t>::is_valid
      >* = 0,
      enable_if_t<
        !can_query<T, capture_t<I>>::value
      >* = 0,
      enable_if_t<
        !can_query<T, terminate_t<I>>::value
      >* = 0) noexcept
  {
    return propagate_t();
  }

  template <typename E, typename T = decltype(propagate_t::static_query<E>())>
  static constexpr const T static_query_v
    = propagate_t::static_query<E>();
#endif // defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
       //   && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

  static constexpr inline_exception_handling_t<I> value()
  {
    return propagate_t();
  }

  friend constexpr bool operator==(
      const propagate_t&, const propagate_t&)
  {
    return true;
  }

  friend constexpr bool operator!=(
      const propagate_t&, const propagate_t&)
  {
    return false;
  }

  friend constexpr bool operator==(
      const propagate_t&, const capture_t<I>&)
  {
    return false;
  }

  friend constexpr bool operator!=(
      const propagate_t&, const capture_t<I>&)
  {
    return true;
  }

  friend constexpr bool operator==(
      const propagate_t&, const terminate_t<I>&)
  {
    return false;
  }

  friend constexpr bool operator!=(
      const propagate_t&, const terminate_t<I>&)
  {
    return true;
  }
};

#if defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
template <int I> template <typename E, typename T>
const T propagate_t<I>::static_query_v;
#endif // defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

template <int I = 0>
struct capture_t
{
#if defined(ASIO_HAS_VARIABLE_TEMPLATES)
  template <typename T>
  static constexpr bool is_applicable_property_v = is_executor<T>::value;
#endif // defined(ASIO_HAS_VARIABLE_TEMPLATES)

  static constexpr bool is_requirable = true;
  static constexpr bool is_preferable = false;
  typedef inline_exception_handling_t<I> polymorphic_query_result_type;

  constexpr capture_t()
  {
  }

  template <typename T>
  struct query_member :
    traits::query_member<
      typename inline_exception_handling_t<I>::template
        proxy<T>::type, capture_t> {};

  template <typename T>
  struct query_static_constexpr_member :
    traits::query_static_constexpr_member<
      typename inline_exception_handling_t<I>::template
        static_proxy<T>::type, capture_t> {};

#if defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
  template <typename T>
  static constexpr typename query_static_constexpr_member<T>::result_type
  static_query()
    noexcept(query_static_constexpr_member<T>::is_noexcept)
  {
    return query_static_constexpr_member<T>::value();
  }

  template <typename E, typename T = decltype(capture_t::static_query<E>())>
  static constexpr const T static_query_v = capture_t::static_query<E>();
#endif // defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
       //   && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

  static constexpr inline_exception_handling_t<I> value()
  {
    return capture_t();
  }

  friend constexpr bool operator==(
      const capture_t&, const capture_t&)
  {
    return true;
  }

  friend constexpr bool operator!=(
      const capture_t&, const capture_t&)
  {
    return false;
  }

  friend constexpr bool operator==(
      const capture_t&, const propagate_t<I>&)
  {
    return false;
  }

  friend constexpr bool operator!=(
      const capture_t&, const propagate_t<I>&)
  {
    return true;
  }

  friend constexpr bool operator==(
      const capture_t&, const terminate_t<I>&)
  {
    return false;
  }

  friend constexpr bool operator!=(
      const capture_t&, const terminate_t<I>&)
  {
    return true;
  }
};

#if defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
template <int I> template <typename E, typename T>
const T capture_t<I>::static_query_v;
#endif // defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

template <int I>
struct terminate_t
{
#if defined(ASIO_HAS_VARIABLE_TEMPLATES)
  template <typename T>
  static constexpr bool is_applicable_property_v = is_executor<T>::value;
#endif // defined(ASIO_HAS_VARIABLE_TEMPLATES)

  static constexpr bool is_requirable = true;
  static constexpr bool is_preferable = true;
  typedef inline_exception_handling_t<I> polymorphic_query_result_type;

  constexpr terminate_t()
  {
  }

  template <typename T>
  struct query_member :
    traits::query_member<
      typename inline_exception_handling_t<I>::template
        proxy<T>::type, terminate_t> {};

  template <typename T>
  struct query_static_constexpr_member :
    traits::query_static_constexpr_member<
      typename inline_exception_handling_t<I>::template
        static_proxy<T>::type, terminate_t> {};

#if defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
  template <typename T>
  static constexpr
  typename query_static_constexpr_member<T>::result_type
  static_query()
    noexcept(query_static_constexpr_member<T>::is_noexcept)
  {
    return query_static_constexpr_member<T>::value();
  }

  template <typename E, typename T = decltype(terminate_t::static_query<E>())>
  static constexpr const T static_query_v
    = terminate_t::static_query<E>();
#endif // defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

  static constexpr inline_exception_handling_t<I> value()
  {
    return terminate_t();
  }

  friend constexpr bool operator==(const terminate_t&, const terminate_t&)
  {
    return true;
  }

  friend constexpr bool operator!=(const terminate_t&, const terminate_t&)
  {
    return false;
  }

  friend constexpr bool operator==(const terminate_t&, const propagate_t<I>&)
  {
    return false;
  }

  friend constexpr bool operator!=(const terminate_t&, const propagate_t<I>&)
  {
    return true;
  }

  friend constexpr bool operator==(const terminate_t&, const capture_t<I>&)
  {
    return false;
  }

  friend constexpr bool operator!=(const terminate_t&, const capture_t<I>&)
  {
    return true;
  }
};

#if defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
template <int I> template <typename E, typename T>
const T terminate_t<I>::static_query_v;
#endif // defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

} // namespace inline_exception_handling
} // namespace detail

typedef detail::inline_exception_handling_t<> inline_exception_handling_t;

ASIO_INLINE_VARIABLE constexpr
inline_exception_handling_t inline_exception_handling;

} // namespace execution

#if !defined(ASIO_HAS_VARIABLE_TEMPLATES)

template <typename T>
struct is_applicable_property<T, execution::inline_exception_handling_t>
  : integral_constant<bool, execution::is_executor<T>::value>
{
};

template <typename T>
struct is_applicable_property<T,
    execution::inline_exception_handling_t::propagate_t>
  : integral_constant<bool, execution::is_executor<T>::value>
{
};

template <typename T>
struct is_applicable_property<T,
    execution::inline_exception_handling_t::capture_t>
  : integral_constant<bool, execution::is_executor<T>::value>
{
};

template <typename T>
struct is_applicable_property<T,
    execution::inline_exception_handling_t::terminate_t>
  : integral_constant<bool, execution::is_executor<T>::value>
{
};

#endif // !defined(ASIO_HAS_VARIABLE_TEMPLATES)

namespace traits {

#if !defined(ASIO_HAS_DEDUCED_QUERY_FREE_TRAIT)

template <typename T>
struct query_free_default<T, execution::inline_exception_handling_t,
  enable_if_t<
    can_query<T, execution::inline_exception_handling_t::propagate_t>::value
  >>
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept = is_nothrow_query<T,
      execution::inline_exception_handling_t::propagate_t>::value;

  typedef execution::inline_exception_handling_t result_type;
};

template <typename T>
struct query_free_default<T, execution::inline_exception_handling_t,
  enable_if_t<
    !can_query<T, execution::inline_exception_handling_t::propagate_t>::value
      && can_query<T, execution::inline_exception_handling_t::capture_t>::value
  >>
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept = is_nothrow_query<T,
      execution::inline_exception_handling_t::capture_t>::value;

  typedef execution::inline_exception_handling_t result_type;
};

template <typename T>
struct query_free_default<T, execution::inline_exception_handling_t,
  enable_if_t<
    !can_query<T,
        execution::inline_exception_handling_t::propagate_t>::value
      && !can_query<T,
        execution::inline_exception_handling_t::capture_t>::value
      && can_query<T,
        execution::inline_exception_handling_t::terminate_t>::value
  >>
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept = is_nothrow_query<T,
      execution::inline_exception_handling_t::terminate_t>::value;

  typedef execution::inline_exception_handling_t result_type;
};

#endif // !defined(ASIO_HAS_DEDUCED_QUERY_FREE_TRAIT)

#if !defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  || !defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

template <typename T>
struct static_query<T, execution::inline_exception_handling_t,
  enable_if_t<
    execution::detail::inline_exception_handling_t<0>::
      query_static_constexpr_member<T>::is_valid
  >>
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept = true;

  typedef typename execution::detail::inline_exception_handling_t<0>::
    query_static_constexpr_member<T>::result_type result_type;

  static constexpr result_type value()
  {
    return execution::inline_exception_handling_t::
      query_static_constexpr_member<T>::value();
  }
};

template <typename T>
struct static_query<T, execution::inline_exception_handling_t,
  enable_if_t<
    !execution::detail::inline_exception_handling_t<0>::
        query_static_constexpr_member<T>::is_valid
      && !execution::detail::inline_exception_handling_t<0>::
        query_member<T>::is_valid
      && traits::static_query<T,
        execution::inline_exception_handling_t::propagate_t>::is_valid
  >>
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept = true;

  typedef typename traits::static_query<T,
    execution::inline_exception_handling_t::propagate_t>::result_type
      result_type;

  static constexpr result_type value()
  {
    return traits::static_query<T,
        execution::inline_exception_handling_t::propagate_t>::value();
  }
};

template <typename T>
struct static_query<T, execution::inline_exception_handling_t,
  enable_if_t<
    !execution::detail::inline_exception_handling_t<0>::
        query_static_constexpr_member<T>::is_valid
      && !execution::detail::inline_exception_handling_t<0>::
        query_member<T>::is_valid
      && !traits::static_query<T,
        execution::inline_exception_handling_t::propagate_t>::is_valid
      && traits::static_query<T,
        execution::inline_exception_handling_t::capture_t>::is_valid
  >>
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept = true;

  typedef typename traits::static_query<T,
    execution::inline_exception_handling_t::capture_t>::result_type result_type;

  static constexpr result_type value()
  {
    return traits::static_query<T,
        execution::inline_exception_handling_t::capture_t>::value();
  }
};

template <typename T>
struct static_query<T, execution::inline_exception_handling_t,
  enable_if_t<
    !execution::detail::inline_exception_handling_t<0>::
        query_static_constexpr_member<T>::is_valid
      && !execution::detail::inline_exception_handling_t<0>::
        query_member<T>::is_valid
      && !traits::static_query<T,
        execution::inline_exception_handling_t::propagate_t>::is_valid
      && !traits::static_query<T,
        execution::inline_exception_handling_t::capture_t>::is_valid
      && traits::static_query<T,
        execution::inline_exception_handling_t::terminate_t>::is_valid
  >>
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept = true;

  typedef typename traits::static_query<T,
    execution::inline_exception_handling_t::terminate_t>::result_type
      result_type;

  static constexpr result_type value()
  {
    return traits::static_query<T,
        execution::inline_exception_handling_t::terminate_t>::value();
  }
};

template <typename T>
struct static_query<T, execution::inline_exception_handling_t::propagate_t,
  enable_if_t<
    execution::detail::inline_exception_handling::propagate_t<0>::
      query_static_constexpr_member<T>::is_valid
  >>
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept = true;

  typedef typename execution::detail::inline_exception_handling::
    propagate_t<0>::query_static_constexpr_member<T>::result_type result_type;

  static constexpr result_type value()
  {
    return execution::detail::inline_exception_handling::propagate_t<0>::
      query_static_constexpr_member<T>::value();
  }
};

template <typename T>
struct static_query<T, execution::inline_exception_handling_t::propagate_t,
  enable_if_t<
    !execution::detail::inline_exception_handling::propagate_t<0>::
        query_static_constexpr_member<T>::is_valid
      && !execution::detail::inline_exception_handling::propagate_t<0>::
        query_member<T>::is_valid
      && !traits::query_free<T,
        execution::inline_exception_handling_t::propagate_t>::is_valid
      && !can_query<T,
        execution::inline_exception_handling_t::capture_t>::value
      && !can_query<T,
        execution::inline_exception_handling_t::terminate_t>::value
  >>
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept = true;

  typedef execution::inline_exception_handling_t::propagate_t result_type;

  static constexpr result_type value()
  {
    return result_type();
  }
};

template <typename T>
struct static_query<T, execution::inline_exception_handling_t::capture_t,
  enable_if_t<
    execution::detail::inline_exception_handling::capture_t<0>::
      query_static_constexpr_member<T>::is_valid
  >>
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept = true;

  typedef typename execution::detail::inline_exception_handling::
    capture_t<0>::query_static_constexpr_member<T>::result_type result_type;

  static constexpr result_type value()
  {
    return execution::detail::inline_exception_handling::capture_t<0>::
      query_static_constexpr_member<T>::value();
  }
};

template <typename T>
struct static_query<T, execution::inline_exception_handling_t::terminate_t,
  enable_if_t<
    execution::detail::inline_exception_handling::terminate_t<0>::
      query_static_constexpr_member<T>::is_valid
  >>
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept = true;

  typedef typename execution::detail::inline_exception_handling::
    terminate_t<0>::query_static_constexpr_member<T>::result_type result_type;

  static constexpr result_type value()
  {
    return execution::detail::inline_exception_handling::terminate_t<0>::
      query_static_constexpr_member<T>::value();
  }
};

#endif // !defined(ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   || !defined(ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

} // namespace traits

#endif // defined(GENERATING_DOCUMENTATION)

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_EXECUTION_INLINE_EXCEPTION_HANDLING_HPP
