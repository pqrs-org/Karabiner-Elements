//
// execution/staging_executor.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_EXECUTION_STAGING_EXECUTOR_HPP
#define ASIO_EXECUTION_STAGING_EXECUTOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include <memory>
#include "asio/detail/executor_function.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/execution/allocator.hpp"
#include "asio/execution/any_executor.hpp"
#include "asio/execution/blocking.hpp"
#include "asio/execution/executor.hpp"
#include "asio/execution/prefer_only.hpp"
#include "asio/execution/relationship.hpp"
#include "asio/prefer.hpp"
#include "asio/query.hpp"
#include "asio/require.hpp"
#include "asio/traits/static_require.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace execution {

template <typename Executor, typename... Properties>
class staging_executor;

#if !defined(GENERATING_DOCUMENTATION)

namespace detail {

// Storage for the target: a value for an owning staging executor, and a
// pointer for one that refers to an executor.

template <typename Executor>
class staging_target
{
public:
  template <typename E>
  explicit staging_target(E&& e)
    : executor_(static_cast<E&&>(e))
  {
  }

  const Executor& get() const noexcept
  {
    return executor_;
  }

  Executor&& take() noexcept
  {
    return static_cast<Executor&&>(executor_);
  }

  bool equals(const staging_target& other) const noexcept
  {
    return executor_ == other.executor_;
  }

private:
  Executor executor_;
};

template <typename Executor>
class staging_target<Executor&>
{
public:
  explicit staging_target(const decay_t<Executor>& e) noexcept
    : executor_(&e)
  {
  }

  const decay_t<Executor>& get() const noexcept
  {
    return *executor_;
  }

  const decay_t<Executor>& take() const noexcept
  {
    return *executor_;
  }

  bool equals(const staging_target& other) const noexcept
  {
    return *executor_ == *other.executor_;
  }

private:
  const decay_t<Executor>* executor_;
};

template <typename T>
struct staging_is_prefer_only : false_type
{
};

template <typename P>
struct staging_is_prefer_only<prefer_only<P>> : true_type
{
};

template <typename P>
struct staging_is_allocator_property : false_type
{
};

template <typename A>
struct staging_is_allocator_property<allocator_t<A>> : true_type
{
};

// The property recorded for a preference, and the entry it adds to the
// sequence. A prefer_only adapter records the property it adapts.

template <typename Property>
struct staging_preferred
{
  typedef Property property_type;
  typedef prefer_only<Property> element_type;

  static const Property& value(const Property& p) noexcept
  {
    return p;
  }
};

template <typename Property>
struct staging_preferred<prefer_only<Property>>
{
  typedef Property property_type;
  typedef prefer_only<Property> element_type;

  static const Property& value(const prefer_only<Property>& p) noexcept
  {
    return p.property;
  }
};

// The key that identifies which property an entry records: its polymorphic
// query result type, or the property itself where it has none.

template <typename Property, typename = void>
struct staging_property_key
{
  typedef decay_t<Property> type;
};

template <typename Property>
struct staging_property_key<Property,
    void_t<typename Property::polymorphic_query_result_type>>
{
  typedef typename Property::polymorphic_query_result_type type;
};

template <typename Element>
using staging_key_t = typename staging_property_key<
    typename staging_preferred<decay_t<Element>>::property_type>::type;

// Holds the property recorded by one link of the chain.

template <typename Property, typename Tail>
struct staged_held : Property
{
  staged_held()
  {
  }

  explicit staged_held(const Property& p)
    : Property(p)
  {
  }

  const Property& staged_value() const noexcept
  {
    return *this;
  }
};

struct staged_end
{
  staged_end()
  {
  }

  template <typename Old, typename New>
  staged_end(const Old&, const New&) noexcept
  {
  }

  template <typename T>
  T&& staged_commit(T&& t) const noexcept
  {
    return static_cast<T&&>(t);
  }
};

// The tail of a link of the existing chain, or the end of the chain itself.

template <typename Old>
inline const typename Old::tail_type& staging_link_tail(
    const Old& old) noexcept
{
  return static_cast<const typename Old::tail_type&>(old);
}

inline staged_end staging_link_tail(const staged_end&) noexcept
{
  return staged_end();
}

// The value a link takes: the property being recorded, or the value its
// counterpart holds. The chain holds at most one link per key.

template <typename Property, typename Old, typename New>
inline const Property& staging_link_value(const Old& old, const New&,
    enable_if_t<
      !is_same<staging_key_t<Property>, staging_key_t<New>>::value
    >* = 0) noexcept
{
  return static_cast<const typename Old::held_type&>(old).staged_value();
}

template <typename Property, typename Old, typename New>
inline const Property& staging_link_value(const Old&, const New& n,
    enable_if_t<
      is_same<staging_key_t<Property>, staging_key_t<New>>::value
    >* = 0) noexcept
{
  return n;
}

// The type that results from applying the properties of a chain, in request
// order, to an expression of type T. Defined only if every step is valid.

template <typename Chain, typename T, typename = void>
struct staging_commit_result
{
};

template <typename T>
struct staging_commit_result<staged_end, T, void>
{
  typedef decay_t<T> type;
};

template <typename Property, typename Tail>
struct staged_require : staged_held<Property, Tail>, Tail
{
  typedef Property property_type;
  typedef Tail tail_type;
  typedef staged_held<Property, Tail> held_type;

  staged_require()
  {
  }

  template <typename Old, typename New>
  staged_require(const Old& old, const New& n)
    : held_type(staging_link_value<Property>(old, n)),
      Tail(staging_link_tail(old), n)
  {
  }

  template <typename T>
  typename staging_commit_result<staged_require, T>::type
  staged_commit(T&& t) const
  {
    return this->Tail::staged_commit(
        asio::require(static_cast<T&&>(t),
          static_cast<const held_type&>(*this).staged_value()));
  }
};

template <typename Property, typename Tail>
struct staged_prefer : staged_held<Property, Tail>, Tail
{
  typedef Property property_type;
  typedef Tail tail_type;
  typedef staged_held<Property, Tail> held_type;

  staged_prefer()
  {
  }

  template <typename Old, typename New>
  staged_prefer(const Old& old, const New& n)
    : held_type(staging_link_value<Property>(old, n)),
      Tail(staging_link_tail(old), n)
  {
  }

  template <typename T>
  typename staging_commit_result<staged_prefer, T>::type
  staged_commit(T&& t) const
  {
    return this->Tail::staged_commit(
        asio::prefer(static_cast<T&&>(t),
          static_cast<const held_type&>(*this).staged_value()));
  }
};

template <typename Property, typename Tail, typename T>
struct staging_commit_result<staged_require<Property, Tail>, T,
    enable_if_t<
      can_require<T, const Property&>::value
    >> :
  staging_commit_result<Tail, require_result_t<T, const Property&>>
{
};

template <typename Property, typename Tail, typename T>
struct staging_commit_result<staged_prefer<Property, Tail>, T,
    enable_if_t<
      can_prefer<T, const Property&>::value
    >> :
  staging_commit_result<Tail, prefer_result_t<T, const Property&>>
{
};

// The chain that records a sequence of properties. A required property is
// held by a staged_require link and a preferred one by a staged_prefer link.

template <typename... Properties>
struct staging_chain;

template <>
struct staging_chain<>
{
  typedef staged_end type;
};

template <typename Property, typename... Rest>
struct staging_chain<Property, Rest...>
{
  typedef staged_require<Property,
      typename staging_chain<Rest...>::type> type;
};

template <typename Property, typename... Rest>
struct staging_chain<prefer_only<Property>, Rest...>
{
  typedef staged_prefer<Property,
      typename staging_chain<Rest...>::type> type;
};

template <typename... Properties>
using staging_chain_t = typename staging_chain<Properties...>::type;

// The most recently recorded link whose property answers a query, or void
// when no recorded property does.

template <typename Property, typename Queried>
struct staging_answers_query :
  integral_constant<bool,
    (is_convertible<Queried, execution::blocking_t>::value
        && is_convertible<Property, execution::blocking_t>::value)
      || (is_convertible<Queried, execution::relationship_t>::value
        && is_convertible<Property, execution::relationship_t>::value)
      || (staging_is_allocator_property<Queried>::value
        && staging_is_allocator_property<Property>::value)>
{
};

template <typename Chain, typename Queried>
struct staging_query_link
{
  typedef void type;
};

template <typename Property, typename Tail, typename Queried>
struct staging_query_link<staged_require<Property, Tail>, Queried>
{
  typedef typename staging_query_link<Tail, Queried>::type tail_link;

  typedef conditional_t<
      !is_void<tail_link>::value,
      tail_link,
      conditional_t<
        staging_answers_query<Property, Queried>::value,
        staged_require<Property, Tail>,
        void
      >
    > type;
};

template <typename Property, typename Tail, typename Queried>
struct staging_query_link<staged_prefer<Property, Tail>, Queried>
{
  typedef typename staging_query_link<Tail, Queried>::type tail_link;

  typedef conditional_t<
      !is_void<tail_link>::value,
      tail_link,
      conditional_t<
        staging_answers_query<Property, Queried>::value,
        staged_prefer<Property, Tail>,
        void
      >
    > type;
};

template <typename Link, typename = void>
struct staging_link_property
{
  typedef void type;
};

template <typename Link>
struct staging_link_property<Link,
    enable_if_t<!is_void<Link>::value>>
{
  typedef typename Link::property_type type;
};

// The allocator held by a recorded allocator property.

template <typename Property>
struct staging_allocator_of
{
  typedef decay_t<decltype(declval<const Property&>().value())> type;

  static type get(const Property& p) noexcept
  {
    return p.value();
  }
};

template <>
struct staging_allocator_of<allocator_t<void>>
{
  typedef std::allocator<void> type;

  static type get(const allocator_t<void>&) noexcept
  {
    return type();
  }
};

template <typename Link, typename = void>
struct staging_allocator_value
{
  typedef void type;
};

template <typename Link>
struct staging_allocator_value<Link,
    enable_if_t<!is_void<Link>::value>>
{
  typedef typename staging_allocator_of<
      typename Link::property_type>::type type;

  template <typename Chain>
  static type get(const Chain& c) noexcept
  {
    return staging_allocator_of<typename Link::property_type>::get(
        static_cast<const typename Link::held_type&>(c).staged_value());
  }
};

template <typename Link>
struct staging_allocator_compare
{
  template <typename Chain>
  static bool equal(const Chain& a, const Chain& b) noexcept
  {
    return staging_allocator_value<Link>::get(a)
      == staging_allocator_value<Link>::get(b);
  }
};

template <>
struct staging_allocator_compare<void>
{
  template <typename Chain>
  static bool equal(const Chain&, const Chain&) noexcept
  {
    return true;
  }
};

struct staging_rebind_t
{
};

template <typename Staging>
struct staging_is_valid;

template <typename Executor, typename... Properties>
struct staging_is_valid<staging_executor<Executor, Properties...>>
{
  template <typename T, typename = void>
  struct check : false_type
  {
  };

  template <typename T>
  struct check<T,
      void_t<
        typename staging_commit_result<
          staging_chain_t<Properties...>, T>::type
      >> :
    true_type
  {
  };

  static constexpr bool value = check<const decay_t<Executor>&>::value;
};

// Whether a sequence already holds an entry with a given key.

template <typename Key, typename... Properties>
struct staging_holds_key : false_type
{
};

template <typename Key, typename Head, typename... Rest>
struct staging_holds_key<Key, Head, Rest...> :
  integral_constant<bool,
    is_same<Key, staging_key_t<Head>>::value
      || staging_holds_key<Key, Rest...>::value>
{
};

// The entry a position of a sequence holds once an element is recorded: the
// element where the keys match, and what was there before otherwise.

template <typename Element, typename Property>
struct staging_entry_after
{
  typedef conditional_t<
      is_same<staging_key_t<Element>, staging_key_t<Property>>::value,
      Element,
      Property
    > type;
};

// The staging executor resulting from recording a further property. A
// sequence holds one entry per key at most, so a match is replaced in place.

template <typename Staging, typename Element, typename = void>
struct staging_recorded;

template <typename Executor, typename... Properties, typename Element>
struct staging_recorded<staging_executor<Executor, Properties...>, Element,
    enable_if_t<
      staging_holds_key<staging_key_t<Element>, Properties...>::value
    >>
{
  typedef staging_executor<Executor,
      typename staging_entry_after<Element, Properties>::type...> type;
};

template <typename Executor, typename... Properties, typename Element>
struct staging_recorded<staging_executor<Executor, Properties...>, Element,
    enable_if_t<
      !staging_holds_key<staging_key_t<Element>, Properties...>::value
    >>
{
  typedef staging_executor<Executor, Properties..., Element> type;
};

// The implementation of require() for a staging executor. Defines type only
// when the operation is offered.

template <typename Staging, typename Property, typename = void>
struct staging_require
{
};

template <typename Executor, typename... Properties, typename Property>
struct staging_require<staging_executor<Executor, Properties...>, Property,
    enable_if_t<
      Property::is_requirable
        && staging_is_valid<
          typename staging_recorded<
            staging_executor<Executor, Properties...>, Property>::type
        >::value
    >>
{
  typedef staging_executor<Executor, Properties...> self_type;
  typedef typename staging_recorded<self_type, Property>::type type;

  static type copy(const self_type& s, const Property& p)
  {
    return type(staging_rebind_t(), s.target_.get(), s, p);
  }

  static type move(self_type&& s, const Property& p)
  {
    return type(staging_rebind_t(), s.target_.take(), s, p);
  }
};

// Whether asio::prefer would act upon a property for an expression of
// type T.

template <typename T, typename Property>
struct staging_prefer_acts_upon :
  integral_constant<bool,
    ASIO_VERSIONED_NAME(prefer_fn)::call_traits<
      ASIO_VERSIONED_NAME(prefer_fn)::impl, T, void(const Property&)
    >::overload != ASIO_VERSIONED_NAME(prefer_fn)::identity>
{
};

// The implementation of prefer() for a staging executor. Defines type only
// when the operation is offered.

template <typename Staging, typename Property, typename = void>
struct staging_prefer
{
};

template <typename Executor, typename... Properties, typename Property>
struct staging_prefer<staging_executor<Executor, Properties...>, Property,
    enable_if_t<
      Property::is_preferable
        && staging_prefer_acts_upon<
          const typename staging_executor<Executor,
            Properties...>::committed_executor_type&,
          typename staging_preferred<Property>::property_type
        >::value
        && staging_is_valid<
          typename staging_recorded<
            staging_executor<Executor, Properties...>,
            typename staging_preferred<Property>::element_type>::type
        >::value
    >>
{
  typedef staging_executor<Executor, Properties...> self_type;
  typedef staging_preferred<Property> preferred_type;
  typedef typename staging_recorded<self_type,
      typename preferred_type::element_type>::type type;

  static type copy(const self_type& s, const Property& p)
  {
    return type(staging_rebind_t(), s.target_.get(), s,
        preferred_type::value(p));
  }

  static type move(self_type&& s, const Property& p)
  {
    return type(staging_rebind_t(), s.target_.take(), s,
        preferred_type::value(p));
  }
};

template <typename Executor, typename... Properties, typename Property>
struct staging_prefer<staging_executor<Executor, Properties...>, Property,
    enable_if_t<
      Property::is_preferable
        && !staging_prefer_acts_upon<
          const typename staging_executor<Executor,
            Properties...>::committed_executor_type&,
          typename staging_preferred<Property>::property_type
        >::value
    >>
{
  typedef staging_executor<Executor, Properties...> self_type;
  typedef self_type type;

  static type copy(const self_type& s, const Property&)
  {
    return s;
  }

  static type move(self_type&& s, const Property&)
  {
    return static_cast<self_type&&>(s);
  }
};

// Determines whether a query is forwarded to the target.

template <typename Staging, typename Property>
struct staging_forwards_query;

template <typename Executor, typename... Properties, typename Property>
struct staging_forwards_query<staging_executor<Executor, Properties...>,
    Property> :
  integral_constant<bool,
    is_void<
      typename staging_query_link<
        staging_chain_t<Properties...>, Property>::type
    >::value
      && can_query<const decay_t<Executor>&, const Property&>::value>
{
};

// Determines whether a staging executor may be converted to another type by
// committing.

template <typename T>
struct staging_is_staging_executor : false_type
{
};

template <typename Executor, typename... Properties>
struct staging_is_staging_executor<staging_executor<Executor, Properties...>> :
  true_type
{
};

template <typename Staging, typename T, typename = void>
struct staging_converts_to : false_type
{
};

template <typename Staging, typename T>
struct staging_converts_to<Staging, T,
    enable_if_t<
      !is_reference<T>::value
        && !is_const<T>::value
        && !staging_is_staging_executor<T>::value
        && is_executor<T>::value
    >> :
  is_constructible<T, typename Staging::committed_executor_type>
{
};

// The combination a polymorphic executor may apply directly to its target: a
// required blocking.never and at most one relationship preference.

enum staging_entry_kind
{
  staging_entry_other,
  staging_entry_never,
  staging_entry_fork,
  staging_entry_continuation
};

template <typename Element>
struct staging_entry_of :
  integral_constant<staging_entry_kind, staging_entry_other>
{
};

template <>
struct staging_entry_of<execution::blocking_t::never_t> :
  integral_constant<staging_entry_kind, staging_entry_never>
{
};

template <>
struct staging_entry_of<prefer_only<execution::relationship_t::fork_t>> :
  integral_constant<staging_entry_kind, staging_entry_fork>
{
};

template <>
struct staging_entry_of<
    prefer_only<execution::relationship_t::continuation_t>> :
  integral_constant<staging_entry_kind, staging_entry_continuation>
{
};

template <staging_entry_kind Kind, typename... Properties>
struct staging_entry_count : integral_constant<std::size_t, 0>
{
};

template <staging_entry_kind Kind, typename Head, typename... Rest>
struct staging_entry_count<Kind, Head, Rest...> :
  integral_constant<std::size_t,
    (staging_entry_of<Head>::value == Kind ? 1 : 0)
      + staging_entry_count<Kind, Rest...>::value>
{
};

template <typename... Properties>
struct staging_pack_kind :
  integral_constant<staged_execution_kind,
    (staging_entry_count<staging_entry_other, Properties...>::value == 0
      && staging_entry_count<staging_entry_never, Properties...>::value == 1
      && staging_entry_count<staging_entry_fork, Properties...>::value
        + staging_entry_count<staging_entry_continuation,
          Properties...>::value <= 1)
    ? (staging_entry_count<staging_entry_fork, Properties...>::value == 1
        ? staged_execution_kind::never_fork
        : staging_entry_count<staging_entry_continuation,
            Properties...>::value == 1
          ? staged_execution_kind::never_continuation
          : staged_execution_kind::never)
    : staged_execution_kind::none>
{
};

template <typename Executor, typename... Properties>
struct staging_execution_kind_of :
  integral_constant<staged_execution_kind,
    is_base_of<any_executor_base, decay_t<Executor>>::value
      ? staging_pack_kind<Properties...>::value
      : staged_execution_kind::none>
{
};

// Whether a staging executor may be constructed from an expression. One that
// refers to its target binds an lvalue, and one that owns it constructs it.

template <typename Executor, typename OtherExecutor>
struct staging_refers_to :
  integral_constant<bool,
    is_lvalue_reference<OtherExecutor>::value
      && is_convertible<remove_reference_t<OtherExecutor>*,
        const decay_t<Executor>*>::value>
{
};

template <typename Executor, typename OtherExecutor>
struct staging_owns :
  is_constructible<decay_t<Executor>, OtherExecutor>
{
};

template <typename Executor, typename OtherExecutor, typename = void>
struct staging_constructible_from : false_type
{
};

template <typename Executor, typename OtherExecutor>
struct staging_constructible_from<Executor, OtherExecutor,
    enable_if_t<
      !staging_is_staging_executor<decay_t<OtherExecutor>>::value
    >> :
  conditional_t<
    is_reference<Executor>::value,
    staging_refers_to<Executor, OtherExecutor>,
    staging_owns<Executor, OtherExecutor>
  >
{
};

template <typename Executor>
struct staging_deduced
{
  typedef conditional_t<
      is_lvalue_reference<Executor>::value,
      const decay_t<Executor>&,
      decay_t<Executor>
    > type;
};

// A staging executor is never erased as it stands. Conversion to a
// polymorphic executor instead commits it.

template <typename Executor, typename... Properties, typename Props>
struct is_valid_target_executor<
    staging_executor<Executor, Properties...>, Props> :
  false_type
{
};

} // namespace detail

#endif // !defined(GENERATING_DOCUMENTATION)

/// An executor adapter that records properties rather than applying them.
/**
 * A @c staging_executor refers to, or owns, a target executor. Calls to
 * <tt>asio::require</tt> and <tt>asio::prefer</tt> on a staging
 * executor do not touch the target. Instead, they return a new staging
 * executor that records the property at the end of a sequence. The
 * recorded properties are applied together, to the target itself, only
 * when a function object is submitted using @c execute(), or when the
 * staging executor is committed to an ordinary executor.
 *
 * This avoids producing a sequence of intermediate executors when several
 * properties are applied before a function object is submitted. The saving
 * is greatest when the target is a polymorphic executor such as
 * @c any_io_executor, where each intermediate result would otherwise be a
 * newly constructed polymorphic wrapper.
 *
 * @par Ownership
 * The @c Executor template parameter determines whether the target is
 * owned:
 *
 * @li <tt>staging_executor<const E&></tt> refers to an existing executor
 * object of type @c E, which is not copied. It must not outlive that
 * object. This is the type returned by execution::stage() for an lvalue.
 *
 * @li <tt>staging_executor<E></tt> owns its target. This is the type
 * returned by execution::stage() for an rvalue, which is moved rather than
 * copied, so that an expression such as
 * <tt>execution::stage(sock.get_executor()).require(...)</tt> is valid
 * within a single statement.
 *
 * @par Recorded properties
 * The @c Properties template parameter pack holds the recorded properties, in
 * the order in which they were requested. A property that was required
 * appears as the property's own type, such as
 * <tt>execution::blocking_t::never_t</tt>, and a property @c P that was
 * preferred appears as <tt>execution::prefer_only<P></tt>. The recorded
 * properties are applied in request order: the first is applied to the
 * target, and each one after it to the result of the one before.
 *
 * Each property is recorded under a key, which is the type the property is
 * queried as where it has one, such as @c execution::blocking_t for
 * @c execution::blocking_t::never_t, and the property's own type otherwise.
 * A preference keys to the same entry as the corresponding requirement.
 * Recording a property replaces the entry already holding its key, in the
 * position that entry was first recorded in, so the sequence holds at most
 * one entry per key. Only the last of the properties sharing a key is
 * therefore applied, and the order of the entries is the order in which
 * their keys were first requested.
 *
 * A consequence is that the set of staging executor types that recording
 * properties can reach is closed, which matters where a staging executor is
 * reachable behind a polymorphic executor: such an executor applies every
 * property it supports to every target it adopts, and adopts each result in
 * turn.
 *
 * A @c require is offered only when the resulting sequence is valid when
 * applied to the target in that order. A preference that the target would
 * not act upon, whether because it already has the property or because it
 * does not support it, returns the staging executor unchanged.
 *
 * @par Queries
 * A query that a recorded property answers is answered from the most
 * recently recorded such property. All other queries are forwarded to the
 * target.
 *
 * @par Direct execution
 * The combinations that a polymorphic executor can apply to the executor it
 * holds without constructing another polymorphic executor are recognised
 * without regard to the order in which their entries were recorded, as the
 * polymorphic executor applies the blocking requirement before the
 * relationship preference in either case.
 *
 * @par Example
 * @code void post_to(const asio::any_io_executor& ex, F f)
 * {
 *   // No intermediate any_io_executor objects are created.
 *   asio::prefer(
 *       asio::require(execution::stage(ex), execution::blocking.never),
 *       execution::relationship.fork
 *     ).execute(std::move(f));
 * } @endcode
 */
template <typename Executor, typename... Properties>
class staging_executor
#if !defined(GENERATING_DOCUMENTATION)
  : private detail::staging_chain_t<Properties...>
#endif // !defined(GENERATING_DOCUMENTATION)
{
#if !defined(GENERATING_DOCUMENTATION)
private:
  typedef detail::staging_chain_t<Properties...> chain_type;
  typedef detail::staging_target<Executor> target_type;
#endif // !defined(GENERATING_DOCUMENTATION)

public:
  /// The type of the target executor.
  typedef decay_t<Executor> inner_executor_type;

  /// The type of executor that results from committing.
  /**
   * This is the type obtained by applying the recorded properties, in the
   * order the sequence holds them, to an lvalue of type
   * @c inner_executor_type.
   */
#if defined(GENERATING_DOCUMENTATION)
  typedef automatically_determined committed_executor_type;
#else // defined(GENERATING_DOCUMENTATION)
  typedef typename detail::staging_commit_result<
      chain_type, const inner_executor_type&
    >::type committed_executor_type;
#endif // defined(GENERATING_DOCUMENTATION)

  /// Construct a staging executor for the specified target.
  /**
   * When @c Executor is a reference type, @c e must be an lvalue, and the
   * staging executor refers to it. Otherwise, the staging executor owns a
   * target that is constructed from @c e, which is moved from when it is an
   * rvalue.
   */
#if defined(GENERATING_DOCUMENTATION)
  template <typename OtherExecutor>
  explicit staging_executor(OtherExecutor&& e);
#else // defined(GENERATING_DOCUMENTATION)
  template <typename OtherExecutor>
  explicit staging_executor(OtherExecutor&& e,
      constraint_t<
        sizeof...(Properties) == 0
          && detail::staging_constructible_from<
            Executor, OtherExecutor>::value
      > = 0)
    : target_(static_cast<OtherExecutor&&>(e))
  {
  }
#endif // defined(GENERATING_DOCUMENTATION)

  /// Copy constructor.
  staging_executor(const staging_executor& other) = default;

  /// Move constructor.
  staging_executor(staging_executor&& other) = default;

  /// Obtain a reference to the target executor.
  const inner_executor_type& get_inner_executor() const noexcept
  {
    return target_.get();
  }

  /// Apply the recorded properties to the target.
  /**
   * @returns The executor that results from applying the recorded
   * properties, in request order, to a copy of the target.
   */
  committed_executor_type commit() const &
  {
    return this->chain_type::staged_commit(target_.get());
  }

  /// Apply the recorded properties to the target.
  /**
   * @returns The executor that results from applying the recorded
   * properties, in request order, to the target. An owned target is moved
   * from.
   */
  committed_executor_type commit() &&
  {
    return this->chain_type::staged_commit(target_.take());
  }

  /// Convert to an owning executor by committing.
  /**
   * This conversion is available for any executor type @c OtherExecutor that
   * may be constructed from @c committed_executor_type.
   */
#if defined(GENERATING_DOCUMENTATION)
  template <typename OtherExecutor>
  operator OtherExecutor() const;
#else // defined(GENERATING_DOCUMENTATION)
  template <typename OtherExecutor,
      typename = constraint_t<
        detail::staging_converts_to<staging_executor, OtherExecutor>::value
      >>
  operator OtherExecutor() const &
  {
    return OtherExecutor(this->commit());
  }

  template <typename OtherExecutor,
      typename = constraint_t<
        detail::staging_converts_to<staging_executor, OtherExecutor>::value
      >>
  operator OtherExecutor() &&
  {
    return OtherExecutor(static_cast<staging_executor&&>(*this).commit());
  }
#endif // defined(GENERATING_DOCUMENTATION)

  /// Record a property.
  /**
   * Do not call this function directly. It is intended for use with the
   * asio::require and asio::prefer customisation points.
   *
   * Returns a staging executor with the property recorded: in place of the
   * entry already holding the property's key, where the sequence holds one,
   * and at the end of the sequence otherwise.
   */
#if defined(GENERATING_DOCUMENTATION)
  template <typename Property>
  automatically_determined require(const Property& p) const;
#else // defined(GENERATING_DOCUMENTATION)
  template <typename Property>
  typename detail::staging_require<staging_executor, Property>::type
  require(const Property& p) const &
  {
    return detail::staging_require<staging_executor, Property>::copy(*this, p);
  }

  template <typename Property>
  typename detail::staging_require<staging_executor, Property>::type
  require(const Property& p) &&
  {
    return detail::staging_require<staging_executor, Property>::move(
        static_cast<staging_executor&&>(*this), p);
  }
#endif // defined(GENERATING_DOCUMENTATION)

  /// Record a preference.
  /**
   * Do not call this function directly. It is intended for use with the
   * asio::prefer customisation point.
   *
   * Returns a staging executor with the preference recorded, in place of the
   * entry already holding the property's key or at the end of the sequence,
   * or unchanged if the target would not act upon the preference.
   */
#if defined(GENERATING_DOCUMENTATION)
  template <typename Property>
  automatically_determined prefer(const Property& p) const;
#else // defined(GENERATING_DOCUMENTATION)
  template <typename Property>
  typename detail::staging_prefer<staging_executor, Property>::type
  prefer(const Property& p) const &
  {
    return detail::staging_prefer<staging_executor, Property>::copy(*this, p);
  }

  template <typename Property>
  typename detail::staging_prefer<staging_executor, Property>::type
  prefer(const Property& p) &&
  {
    return detail::staging_prefer<staging_executor, Property>::move(
        static_cast<staging_executor&&>(*this), p);
  }
#endif // defined(GENERATING_DOCUMENTATION)

#if !defined(GENERATING_DOCUMENTATION)
private:
  friend struct ASIO_VERSIONED_NAME(query_fn)::impl;
  friend struct asio::execution::detail::blocking_t<0>;
  friend struct asio::execution::detail::relationship_t<0>;
#endif // !defined(GENERATING_DOCUMENTATION)

  /// Query the recorded @c blocking property.
  /**
   * Do not call this function directly. It is intended for use with the
   * asio::query customisation point.
   */
#if defined(GENERATING_DOCUMENTATION)
  static constexpr execution::blocking_t query(execution::blocking_t);
#else // defined(GENERATING_DOCUMENTATION)
  template <typename Link = typename detail::staging_query_link<
      detail::staging_chain_t<Properties...>, blocking_t>::type>
  static constexpr constraint_t<!is_void<Link>::value, blocking_t>
  query(blocking_t) noexcept
  {
    return typename Link::property_type();
  }
#endif // defined(GENERATING_DOCUMENTATION)

  /// Query the recorded @c relationship property.
  /**
   * Do not call this function directly. It is intended for use with the
   * asio::query customisation point.
   */
#if defined(GENERATING_DOCUMENTATION)
  static constexpr execution::relationship_t query(
      execution::relationship_t);
#else // defined(GENERATING_DOCUMENTATION)
  template <typename Link = typename detail::staging_query_link<
      detail::staging_chain_t<Properties...>, relationship_t>::type>
  static constexpr constraint_t<!is_void<Link>::value, relationship_t>
  query(relationship_t) noexcept
  {
    return typename Link::property_type();
  }
#endif // defined(GENERATING_DOCUMENTATION)

  /// Query the recorded @c allocator property.
  /**
   * Do not call this function directly. It is intended for use with the
   * asio::query customisation point.
   */
#if defined(GENERATING_DOCUMENTATION)
  template <typename OtherAllocator>
  automatically_determined query(
      execution::allocator_t<OtherAllocator>) const;
#else // defined(GENERATING_DOCUMENTATION)
  template <typename OtherAllocator,
      typename Link = typename detail::staging_query_link<
        detail::staging_chain_t<Properties...>,
        allocator_t<OtherAllocator>>::type>
  constraint_t<
    !is_void<Link>::value,
    typename detail::staging_allocator_value<Link>::type
  >
  query(allocator_t<OtherAllocator>) const noexcept
  {
    return detail::staging_allocator_value<Link>::get(
        static_cast<const chain_type&>(*this));
  }
#endif // defined(GENERATING_DOCUMENTATION)

  /// Forward a query to the target executor.
  /**
   * Do not call this function directly. It is intended for use with the
   * asio::query customisation point.
   */
#if defined(GENERATING_DOCUMENTATION)
  template <typename Property>
  automatically_determined query(const Property& p) const;
#else // defined(GENERATING_DOCUMENTATION)
  template <typename Property>
  constraint_t<
    detail::staging_forwards_query<staging_executor, Property>::value,
    query_result_t<const inner_executor_type&, const Property&>
  >
  query(const Property& p) const
    noexcept(
      is_nothrow_query<const inner_executor_type&, const Property&>::value)
  {
    return asio::query(target_.get(), p);
  }
#endif // defined(GENERATING_DOCUMENTATION)

public:
  /// Apply the recorded properties and submit a function object.
  /**
   * When the target is not a polymorphic executor, this is equivalent to
   * <tt>commit().execute(f)</tt>, except that the target is not copied
   * unless a recorded property yields a new executor.
   *
   * When the target is an execution::any_executor, and the recorded
   * properties are one of <tt>blocking.never</tt>; <tt>blocking.never</tt>
   * and <tt>relationship.fork</tt>; or <tt>blocking.never</tt> and
   * <tt>relationship.continuation</tt>, the properties are applied directly
   * to the concrete executor held by the polymorphic executor, and the
   * function object is submitted to the result. No intermediate polymorphic
   * executor is constructed. Other sequences commit and then execute.
   */
  template <typename Function>
  void execute(Function&& f) const
  {
    this->execute_impl(static_cast<Function&&>(f),
        integral_constant<bool,
          detail::staging_execution_kind_of<Executor, Properties...>::value
            != detail::staged_execution_kind::none>());
  }

#if !defined(GENERATING_DOCUMENTATION)
  /// Apply the recorded properties and submit a ready-made operation.
  /**
   * Equivalent to the overload above, except that a function object that
   * already owns the operation holding it is handed to the target as it
   * stands, rather than being wrapped in an operation of its own.
   */
  void execute(asio::detail::executor_function&& f) const
  {
    this->execute_function(
        static_cast<asio::detail::executor_function&&>(f),
        integral_constant<bool,
          detail::staging_execution_kind_of<Executor, Properties...>::value
            != detail::staged_execution_kind::none>());
  }
#endif // !defined(GENERATING_DOCUMENTATION)

  /// Compare two staging executors for equality.
  /**
   * Two staging executors are equal if their targets are equal and their
   * recorded properties are equal.
   */
  friend bool operator==(const staging_executor& a,
      const staging_executor& b) noexcept
  {
    return a.target_.equals(b.target_)
      && detail::staging_allocator_compare<
        typename detail::staging_query_link<
          chain_type, allocator_t<void>>::type
      >::equal(static_cast<const chain_type&>(a),
          static_cast<const chain_type&>(b));
  }

  /// Compare two staging executors for inequality.
  /**
   * Two staging executors are equal if their targets are equal and their
   * recorded properties are equal.
   */
  friend bool operator!=(const staging_executor& a,
      const staging_executor& b) noexcept
  {
    return !(a == b);
  }

#if !defined(GENERATING_DOCUMENTATION)
private:
  template <typename, typename...>
  friend class staging_executor;

  template <typename, typename, typename>
  friend struct detail::staging_require;

  template <typename, typename, typename>
  friend struct detail::staging_prefer;

  template <typename Function>
  void execute_impl(Function&& f, false_type) const
  {
    this->commit().execute(static_cast<Function&&>(f));
  }

  template <typename Function>
  void execute_impl(Function&& f, true_type) const
  {
    const inner_executor_type& target = target_.get();
    if (target.can_execute_staged())
    {
      asio::detail::executor_function fn(
          static_cast<Function&&>(f), std::allocator<void>());
      if (!target.execute_staged(fn,
            detail::staging_execution_kind_of<
              Executor, Properties...>::value))
      {
        this->commit().execute(
            static_cast<asio::detail::executor_function&&>(fn));
      }
    }
    else
    {
      this->commit().execute(static_cast<Function&&>(f));
    }
  }

  void execute_function(
      asio::detail::executor_function&& f, false_type) const
  {
    this->commit().execute(
        static_cast<asio::detail::executor_function&&>(f));
  }

  void execute_function(
      asio::detail::executor_function&& f, true_type) const
  {
    // The operation is already the one that holds the function, so the target
    // is handed it as it stands, where the staged entries would otherwise have
    // wrapped it in a second one.
    const inner_executor_type& target = target_.get();
    if (target.can_execute_staged())
    {
      if (!target.execute_staged(f,
            detail::staging_execution_kind_of<
              Executor, Properties...>::value))
      {
        this->commit().execute(
            static_cast<asio::detail::executor_function&&>(f));
      }
    }
    else
    {
      this->commit().execute(
          static_cast<asio::detail::executor_function&&>(f));
    }
  }

  template <typename Target, typename Other, typename New>
  staging_executor(detail::staging_rebind_t, Target&& target,
      const Other& other, const New& n)
    : chain_type(other.staged_chain(), n),
      target_(static_cast<Target&&>(target))
  {
  }

  const chain_type& staged_chain() const noexcept
  {
    return *this;
  }

  target_type target_;
#endif // !defined(GENERATING_DOCUMENTATION)
};

#if !defined(GENERATING_DOCUMENTATION)
#if defined(__cpp_deduction_guides) && (__cpp_deduction_guides >= 201606)

template <typename Executor>
staging_executor(Executor&&)
  -> staging_executor<typename detail::staging_deduced<Executor>::type>;

#endif // defined(__cpp_deduction_guides) && (__cpp_deduction_guides >= 201606)
#endif // !defined(GENERATING_DOCUMENTATION)

/// Create a staging executor that refers to an executor.
/**
 * @param ex The target executor. It is not copied, and the returned staging
 * executor must not outlive it.
 *
 * @returns <tt>staging_executor<const Executor&>(ex)</tt>.
 */
#if defined(GENERATING_DOCUMENTATION)
template <typename Executor>
staging_executor<const Executor&> stage(const Executor& ex) noexcept;
#else // defined(GENERATING_DOCUMENTATION)
template <typename Executor>
inline staging_executor<const Executor&> stage(const Executor& ex,
    constraint_t<is_executor<Executor>::value> = 0) noexcept
{
  return staging_executor<const Executor&>(ex);
}
#endif // defined(GENERATING_DOCUMENTATION)

/// Create a staging executor that owns an executor.
/**
 * @param ex The target executor, which is moved into the returned staging
 * executor.
 *
 * @returns <tt>staging_executor<Executor>(std::move(ex))</tt>.
 */
#if defined(GENERATING_DOCUMENTATION)
template <typename Executor>
staging_executor<Executor> stage(Executor&& ex);
#else // defined(GENERATING_DOCUMENTATION)
template <typename Executor>
inline staging_executor<decay_t<Executor>> stage(Executor&& ex,
    constraint_t<
      !is_lvalue_reference<Executor>::value
        && is_executor<decay_t<Executor>>::value
    > = 0)
{
  return staging_executor<decay_t<Executor>>(static_cast<Executor&&>(ex));
}
#endif // defined(GENERATING_DOCUMENTATION)

} // namespace execution

#if !defined(GENERATING_DOCUMENTATION)

namespace traits {

#if !defined(ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)

template <typename Executor, typename... Properties>
struct equality_comparable<
    execution::staging_executor<Executor, Properties...>>
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept = true;
};

#endif // !defined(ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)

#if !defined(ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)

template <typename Executor, typename... Properties, typename Function>
struct execute_member<execution::staging_executor<Executor, Properties...>,
    Function,
    enable_if_t<
      traits::execute_member<
        const typename execution::staging_executor<
          Executor, Properties...>::committed_executor_type&,
        Function
      >::is_valid
    >>
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept = false;
  typedef void result_type;
};

#endif // !defined(ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)

#if !defined(ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)

template <typename Executor, typename... Properties, typename Property>
struct query_static_constexpr_member<
    execution::staging_executor<Executor, Properties...>, Property,
    enable_if_t<
      (is_convertible<Property, execution::blocking_t>::value
        || is_convertible<Property, execution::relationship_t>::value)
      && !is_void<
        typename execution::detail::staging_query_link<
          execution::detail::staging_chain_t<Properties...>, Property
        >::type
      >::value
    >>
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept = true;
  typedef typename execution::detail::staging_link_property<
      typename execution::detail::staging_query_link<
        execution::detail::staging_chain_t<Properties...>, Property
      >::type
    >::type result_type;

  static constexpr result_type value() noexcept
  {
    return result_type();
  }
};

#endif // !defined(ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)

#if !defined(ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)

// The recorded allocator answers its own query. Any other query not already
// answered by a recorded property (see staging_forwards_query) is forwarded
// to the target.

template <typename Executor, typename... Properties, typename OtherAllocator>
struct query_member<execution::staging_executor<Executor, Properties...>,
    execution::allocator_t<OtherAllocator>,
    enable_if_t<
      !is_void<
        typename execution::detail::staging_query_link<
          execution::detail::staging_chain_t<Properties...>,
          execution::allocator_t<OtherAllocator>
        >::type
      >::value
    >>
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept = true;
  typedef typename execution::detail::staging_allocator_value<
      typename execution::detail::staging_query_link<
        execution::detail::staging_chain_t<Properties...>,
        execution::allocator_t<OtherAllocator>
      >::type
    >::type result_type;
};

template <typename Executor, typename... Properties, typename Property>
struct query_member<execution::staging_executor<Executor, Properties...>,
    Property,
    enable_if_t<
      execution::detail::staging_forwards_query<
        execution::staging_executor<Executor, Properties...>, Property
      >::value
    >>
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept =
    is_nothrow_query<const decay_t<Executor>&, Property>::value;
  typedef query_result_t<const decay_t<Executor>&, Property> result_type;
};

#endif // !defined(ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)

#if !defined(ASIO_HAS_DEDUCED_REQUIRE_MEMBER_TRAIT)

template <typename Executor, typename... Properties, typename Property>
struct require_member<execution::staging_executor<Executor, Properties...>,
    Property,
    void_t<
      typename execution::detail::staging_require<
        execution::staging_executor<Executor, Properties...>,
        decay_t<Property>>::type
    >>
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept = false;
  typedef typename execution::detail::staging_require<
      execution::staging_executor<Executor, Properties...>,
      decay_t<Property>>::type result_type;
};

#endif // !defined(ASIO_HAS_DEDUCED_REQUIRE_MEMBER_TRAIT)

#if !defined(ASIO_HAS_DEDUCED_PREFER_MEMBER_TRAIT)

template <typename Executor, typename... Properties, typename Property>
struct prefer_member<execution::staging_executor<Executor, Properties...>,
    Property,
    void_t<
      typename execution::detail::staging_prefer<
        execution::staging_executor<Executor, Properties...>,
        decay_t<Property>>::type
    >>
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept = false;
  typedef typename execution::detail::staging_prefer<
      execution::staging_executor<Executor, Properties...>,
      decay_t<Property>>::type result_type;
};

#endif // !defined(ASIO_HAS_DEDUCED_PREFER_MEMBER_TRAIT)

} // namespace traits

#endif // !defined(GENERATING_DOCUMENTATION)

ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_EXECUTION_STAGING_EXECUTOR_HPP
