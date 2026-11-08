//
// detail/candidate_executors.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_CANDIDATE_EXECUTORS_HPP
#define ASIO_DETAIL_CANDIDATE_EXECUTORS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/associated_executor.hpp"
#include "asio/associated_immediate_executor.hpp"
#include "asio/associator.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/execution/any_executor.hpp"
#include "asio/execution/blocking.hpp"
#include "asio/require.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

template <typename T, typename Candidate>
struct executor_is_candidate_query;

template <typename T, typename Candidate,
    typename = void, typename = void>
struct executor_is_candidate_query_impl
{
  typedef bool type;

  static const bool is_known = false;

  static type get(const T&, const Candidate&) noexcept
  {
    return false;
  }
};

template <typename T, typename Candidate>
struct executor_is_candidate_query_impl<T, Candidate,
    enable_if_t<
      is_base_of<
        associator<associated_executor, T, Candidate>,
        associated_executor<T, Candidate>
      >::value
    >,
    void_t<
      typename associator<executor_is_candidate_query, T, Candidate>::type
    >
  > : associator<executor_is_candidate_query, T, Candidate>
{
};

template <typename T, typename Candidate>
struct executor_is_candidate_query
  : executor_is_candidate_query_impl<T, Candidate>
{
};

template <typename T, typename Candidate>
inline bool executor_is_candidate(
    const T& t, const Candidate& candidate) noexcept
{
  return executor_is_candidate_query<T, Candidate>::get(t, candidate);
}

// Helper function to determine at runtime whether a polymorphic immediate
// executor is the candidate with blocking.never required.
template <typename T, typename Candidate>
enable_if_t<
  is_base_of<execution::detail::any_executor_base,
    associated_immediate_executor_t<T, Candidate>>::value
      && can_require<Candidate, execution::blocking_t::never_t>::value,
  bool
> immediate_executor_is_candidate_object(
    const T& t, const Candidate& candidate) noexcept
{
  return execution::detail::executor_targets_equal(
      (get_associated_immediate_executor)(t, candidate),
      asio::require(candidate, execution::blocking.never));
}

template <typename T, typename Candidate>
enable_if_t<
  !is_base_of<execution::detail::any_executor_base,
    associated_immediate_executor_t<T, Candidate>>::value
      || !can_require<Candidate, execution::blocking_t::never_t>::value,
  bool
> immediate_executor_is_candidate_object(const T&, const Candidate&) noexcept
{
  return false;
}

template <typename T, typename Candidate>
struct immediate_executor_is_candidate_query;

template <typename T, typename Candidate,
    typename = void, typename = void>
struct immediate_executor_is_candidate_query_impl
{
  typedef bool type;

  static type get(const T& t, const Candidate& candidate) noexcept
  {
    return immediate_executor_is_candidate_object(t, candidate);
  }
};

template <typename T, typename Candidate>
struct immediate_executor_is_candidate_query_impl<T, Candidate,
    enable_if_t<
      is_base_of<
        associator<associated_immediate_executor, T, Candidate>,
        associated_immediate_executor<T, Candidate>
      >::value
    >,
    void_t<
      typename associator<immediate_executor_is_candidate_query,
        T, Candidate>::type
    >
  > : associator<immediate_executor_is_candidate_query, T, Candidate>
{
};

template <typename T, typename Candidate>
struct immediate_executor_is_candidate_query
  : immediate_executor_is_candidate_query_impl<T, Candidate>
{
};

template <typename T, typename Candidate>
inline bool immediate_executor_is_candidate(
    const T& t, const Candidate& candidate) noexcept
{
  return immediate_executor_is_candidate_query<T, Candidate>::get(t, candidate);
}

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_CANDIDATE_EXECUTORS_HPP
