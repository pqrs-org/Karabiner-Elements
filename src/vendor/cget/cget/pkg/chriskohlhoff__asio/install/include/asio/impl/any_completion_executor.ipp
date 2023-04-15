//
// impl/any_completion_executor.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2023 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_IMPL_ANY_COMPLETION_EXECUTOR_IPP
#define ASIO_IMPL_ANY_COMPLETION_EXECUTOR_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if !defined(ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

#include "asio/any_completion_executor.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {

any_completion_executor::any_completion_executor() ASIO_NOEXCEPT
  : base_type()
{
}

any_completion_executor::any_completion_executor(nullptr_t) ASIO_NOEXCEPT
  : base_type(nullptr_t())
{
}

any_completion_executor::any_completion_executor(
    const any_completion_executor& e) ASIO_NOEXCEPT
  : base_type(static_cast<const base_type&>(e))
{
}

any_completion_executor::any_completion_executor(std::nothrow_t,
    const any_completion_executor& e) ASIO_NOEXCEPT
  : base_type(static_cast<const base_type&>(e))
{
}

#if defined(ASIO_HAS_MOVE)
any_completion_executor::any_completion_executor(
    any_completion_executor&& e) ASIO_NOEXCEPT
  : base_type(static_cast<base_type&&>(e))
{
}

any_completion_executor::any_completion_executor(std::nothrow_t,
    any_completion_executor&& e) ASIO_NOEXCEPT
  : base_type(static_cast<base_type&&>(e))
{
}
#endif // defined(ASIO_HAS_MOVE)

any_completion_executor& any_completion_executor::operator=(
    const any_completion_executor& e) ASIO_NOEXCEPT
{
  base_type::operator=(static_cast<const base_type&>(e));
  return *this;
}

#if defined(ASIO_HAS_MOVE)
any_completion_executor& any_completion_executor::operator=(
    any_completion_executor&& e) ASIO_NOEXCEPT
{
  base_type::operator=(static_cast<base_type&&>(e));
  return *this;
}
#endif // defined(ASIO_HAS_MOVE)

any_completion_executor& any_completion_executor::operator=(nullptr_t)
{
  base_type::operator=(nullptr_t());
  return *this;
}

any_completion_executor::~any_completion_executor()
{
}

void any_completion_executor::swap(
    any_completion_executor& other) ASIO_NOEXCEPT
{
  static_cast<base_type&>(*this).swap(static_cast<base_type&>(other));
}

template <>
any_completion_executor any_completion_executor::prefer(
    const execution::outstanding_work_t::tracked_t& p, int) const
{
  return static_cast<const base_type&>(*this).prefer(p);
}

template <>
any_completion_executor any_completion_executor::prefer(
    const execution::outstanding_work_t::untracked_t& p, int) const
{
  return static_cast<const base_type&>(*this).prefer(p);
}

template <>
any_completion_executor any_completion_executor::prefer(
    const execution::relationship_t::fork_t& p, int) const
{
  return static_cast<const base_type&>(*this).prefer(p);
}

template <>
any_completion_executor any_completion_executor::prefer(
    const execution::relationship_t::continuation_t& p, int) const
{
  return static_cast<const base_type&>(*this).prefer(p);
}

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // !defined(ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

#endif // ASIO_IMPL_ANY_COMPLETION_EXECUTOR_IPP
