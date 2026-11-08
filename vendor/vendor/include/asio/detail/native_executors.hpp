//
// detail/native_executors.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_NATIVE_EXECUTORS_HPP
#define ASIO_DETAIL_NATIVE_EXECUTORS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#if !defined(ASIO_NO_TYPEID)
# include <typeinfo>
#endif // !defined(ASIO_NO_TYPEID)
#include "asio/detail/type_traits.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN

class io_context;

#if !defined(ASIO_NO_TS_EXECUTORS)

class executor;

#endif // !defined(ASIO_NO_TS_EXECUTORS)

#if !defined(ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

namespace execution {

#if !defined(ASIO_EXECUTION_ANY_EXECUTOR_FWD_DECL)
#define ASIO_EXECUTION_ANY_EXECUTOR_FWD_DECL

template <typename... SupportableProperties>
class any_executor;

#endif // !defined(ASIO_EXECUTION_ANY_EXECUTOR_FWD_DECL)

} // namespace execution

class any_io_executor;

class any_completion_executor;

#endif // !defined(ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

namespace detail {

// Tag types and traits to test whether an executor has native execution.

struct native_execute_kinds
{
  struct native {};
  struct polymorphic {};
  struct polymorphic_typeid {};
  struct none {};
};

// Trait to determine whether an executor has native execution.
template <typename E, typename IoContext = io_context, typename = void>
struct native_execute_kind
{
  using type = native_execute_kinds::none;
};

template <typename E, typename IoContext>
struct native_execute_kind<E, IoContext,
    enable_if_t<
      is_same<E, typename IoContext::executor_type>::value
    >
  >
{
  using type = native_execute_kinds::native;
};

#if !defined(ASIO_NO_TYPEID)

#if !defined(ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

template <typename IoContext, typename... SupportableProperties>
struct native_execute_kind<
    execution::any_executor<SupportableProperties...>, IoContext, void>
{
  using type = native_execute_kinds::polymorphic;
};

template <typename IoContext>
struct native_execute_kind<any_io_executor, IoContext, void>
{
  using type = native_execute_kinds::polymorphic;
};

template <typename IoContext>
struct native_execute_kind<any_completion_executor, IoContext, void>
{
  using type = native_execute_kinds::polymorphic;
};

#endif // !defined(ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

#if !defined(ASIO_NO_TS_EXECUTORS)

template <typename IoContext>
struct native_execute_kind<executor, IoContext, void>
{
  using type = native_execute_kinds::polymorphic_typeid;
};

#endif // !defined(ASIO_NO_TS_EXECUTORS)

#endif // !defined(ASIO_NO_TYPEID)

template <typename E>
using native_execute_kind_t = typename native_execute_kind<E>::type;

template <typename E>
inline bool has_native_execute(const E&, native_execute_kinds::native)
{
  return true;
}

#if !defined(ASIO_NO_TYPEID)

template <typename E>
inline bool has_native_execute(const E& ex, native_execute_kinds::polymorphic)
{
  return target_has_native_execute(ex);
}

template <typename E, typename IoContext = io_context>
inline bool has_native_execute(const E& ex,
    native_execute_kinds::polymorphic_typeid)
{
  return ex.target_type() == typeid(typename IoContext::executor_type);
}

#endif // !defined(ASIO_NO_TYPEID)

template <typename E>
inline bool has_native_execute(const E&, native_execute_kinds::none)
{
  return false;
}

template <typename E>
inline bool has_native_execute(const E& ex)
{
  return (has_native_execute)(ex, native_execute_kind_t<E>());
}

// Tag types and traits to test whether an executor has native work tracking.

struct native_outstanding_work_kinds
{
  struct native {};
  struct polymorphic {};
  struct polymorphic_typeid {};
  struct none {};
};

template <typename E, typename IoContext = io_context, typename = void>
struct native_outstanding_work_kind
{
  using type = native_outstanding_work_kinds::none;
};

template <typename E, typename IoContext>
struct native_outstanding_work_kind<E, IoContext,
    enable_if_t<
      is_same<E, typename IoContext::executor_type>::value
    >
  >
{
  using type = native_outstanding_work_kinds::native;
};

#if !defined(ASIO_NO_TYPEID)

#if !defined(ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

template <typename IoContext, typename... SupportableProperties>
struct native_outstanding_work_kind<
    execution::any_executor<SupportableProperties...>, IoContext, void>
{
  using type = native_outstanding_work_kinds::polymorphic;
};

template <typename IoContext>
struct native_outstanding_work_kind<any_io_executor, IoContext, void>
{
  using type = native_outstanding_work_kinds::polymorphic;
};

template <typename IoContext>
struct native_outstanding_work_kind<any_completion_executor, IoContext, void>
{
  using type = native_outstanding_work_kinds::polymorphic;
};

#endif // !defined(ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

#if !defined(ASIO_NO_TS_EXECUTORS)

template <typename IoContext>
struct native_outstanding_work_kind<executor, IoContext, void>
{
  using type = native_outstanding_work_kinds::polymorphic_typeid;
};

#endif // !defined(ASIO_NO_TS_EXECUTORS)

#endif // !defined(ASIO_NO_TYPEID)

template <typename E>
using native_outstanding_work_kind_t =
  typename native_outstanding_work_kind<E>::type;

template <typename E>
inline bool has_native_outstanding_work(const E&,
    native_outstanding_work_kinds::native)
{
  return true;
}

#if !defined(ASIO_NO_TYPEID)

template <typename E>
inline bool has_native_outstanding_work(const E& ex,
    native_outstanding_work_kinds::polymorphic)
{
  return target_has_native_outstanding_work(ex);
}

template <typename E, typename IoContext = io_context>
inline bool has_native_outstanding_work(const E& ex,
    native_outstanding_work_kinds::polymorphic_typeid)
{
  return ex.target_type() == typeid(typename IoContext::executor_type);
}

#endif // !defined(ASIO_NO_TYPEID)

template <typename E>
inline bool has_native_outstanding_work(const E&,
    native_outstanding_work_kinds::none)
{
  return false;
}

template <typename E>
inline bool has_native_outstanding_work(const E& ex)
{
  return (has_native_outstanding_work)(ex, native_outstanding_work_kind_t<E>());
}

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_NATIVE_EXECUTORS_HPP
