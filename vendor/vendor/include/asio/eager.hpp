//
// eager.hpp
// ~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_EAGER_HPP
#define ASIO_EAGER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/any_io_executor.hpp"
#include "asio/async_result.hpp"
#include "asio/cancellation_type.hpp"
#include "asio/detail/assert.hpp"
#include "asio/detail/memory.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/piping.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

struct eager_init_tag {};

template <typename... Signatures>
class eager_impl;

template <typename Executor, typename... Signatures>
class initiate_eager_wait;

} // namespace detail

/// An eagerly-initiated asynchronous operation that is awaited later.
/**
 * A @c basic_eager_async_operation is a move-only handle to a single
 * asynchronous operation that has already been started. It is produced by
 * passing the @ref eager completion token to an asynchronous operation, and is
 * more conveniently named using the @ref eager_async_operation alias.
 *
 * The operation is consumed by invoking it as an rvalue, exactly once, which
 * starts an asynchronous wait for its completion:
 *
 * @code
 * asio::eager_async_operation<
 *   void(asio::error_code, std::size_t)> op
 *     = socket.async_read_some(buffer, asio::eager);
 *
 * // ... do other work while the read runs ...
 *
 * std::move(op)([](asio::error_code e, std::size_t n) { ... });
 * @endcode
 *
 * Destroying a handle that has not been waited on detaches from the operation.
 * The operation is not cancelled. It continues to run, and its result is
 * discarded. Although the handle may be moved as necessary, ownership of the
 * objects the operation refers to is retained by the caller, which must
 * guarantee that they remain valid until the operation completes. To cancel
 * the operation, call @c cancel, or bind a cancellation slot to the completion
 * handler passed when waiting.
 *
 * @par Thread Safety
 * @e Distinct @e objects: Safe.@n
 * @e Shared @e objects: Unsafe.@n
 * An eager operation adds no thread safety to the operation that it wraps. A
 * cancellation request is emitted on the thread that requests it, so it is the
 * caller's responsibility to do so from a context that is safe with respect to
 * the operation's execution.
 *
 * @sa @ref overview_eager "Eager operations"
 */
template <typename Executor, typename... Signatures>
class ASIO_NODISCARD basic_eager_async_operation
{
public:
  /// Move constructor.
  /**
   * Transfers ownership of the operation, leaving @c other empty.
   */
  basic_eager_async_operation(basic_eager_async_operation&& other) noexcept
    : impl_(static_cast<detail::shared_ptr<impl_type>&&>(other.impl_)),
      executor_(static_cast<Executor&&>(other.executor_))
  {
  }

  /// Converting move constructor.
  /**
   * Transfers ownership of an operation whose executor is convertible to this
   * object's executor type, leaving @c other empty.
   */
  template <typename OtherExecutor>
  basic_eager_async_operation(
      basic_eager_async_operation<OtherExecutor, Signatures...>&& other,
      constraint_t<
        is_convertible<OtherExecutor, Executor>::value
      > = 0)
    : impl_(static_cast<detail::shared_ptr<impl_type>&&>(other.impl_)),
      executor_(static_cast<OtherExecutor&&>(other.executor_))
  {
  }

  /// Move assignment operator.
  /**
   * Detaches from this object's operation, if it holds one that has not been
   * waited on, then transfers ownership of @c other's operation, leaving
   * @c other empty.
   */
  basic_eager_async_operation& operator=(
      basic_eager_async_operation&& other) noexcept
  {
    if (this != &other)
    {
      impl_ = static_cast<detail::shared_ptr<impl_type>&&>(other.impl_);
      executor_ = static_cast<Executor&&>(other.executor_);
    }
    return *this;
  }

  /// Cancel the operation.
  /**
   * Requests cancellation of the eagerly-initiated operation by emitting a
   * cancellation signal on the calling thread. This function has no effect if
   * the operation has already been waited on or moved from. To cancel after
   * waiting has begun, bind a cancellation slot to the completion handler.
   *
   * @note The caller must ensure that the signal is emitted from a context
   * that is safe with respect to the operation's execution, and that the
   * operation's I/O object still exists.
   */
  void cancel(cancellation_type_t level = cancellation_type::all);

  /// Whether the operation's result is ready to be taken without waiting.
  /**
   * @returns @c true if the operation has completed and its result is
   * available, or @c false if it has not, or if this object has been waited on
   * or moved from.
   */
  bool ready() const noexcept;

  /// Wait for the operation to complete, consuming the eager operation.
  /**
   * The wait consumes the eager operation, so this call operator must be
   * invoked on an rvalue. Ownership passes to the completion token. When
   * the token is a lazy one, such as asio::deferred, destroying the
   * resulting operation without initiating the wait detaches from the eager
   * operation.
   *
   * @note The operation may be waited on at most once. Invoking a consumed or
   * moved-from eager operation results in undefined behaviour.
   */
  template <ASIO_COMPLETION_TOKEN_FOR(Signatures...) CompletionToken>
  auto operator()(CompletionToken&& token) &&
    -> decltype(async_initiate<CompletionToken, Signatures...>(
          declval<detail::initiate_eager_wait<Executor, Signatures...>>(),
          token, declval<basic_eager_async_operation>()))
  {
    ASIO_ASSERT(impl_);
    return async_initiate<CompletionToken, Signatures...>(
        detail::initiate_eager_wait<Executor, Signatures...>(executor_),
        token, static_cast<basic_eager_async_operation&&>(*this));
  }

#if !defined(GENERATING_DOCUMENTATION)
  basic_eager_async_operation(detail::eager_init_tag,
      detail::shared_ptr<detail::eager_impl<Signatures...>> impl,
      Executor executor)
    : impl_(static_cast<detail::shared_ptr<impl_type>&&>(impl)),
      executor_(static_cast<Executor&&>(executor))
  {
  }
#endif // !defined(GENERATING_DOCUMENTATION)

private:
  basic_eager_async_operation(const basic_eager_async_operation&) = delete;

#if !defined(GENERATING_DOCUMENTATION)
  template <typename, typename...>
  friend class basic_eager_async_operation;

  friend class detail::initiate_eager_wait<Executor, Signatures...>;
#endif // !defined(GENERATING_DOCUMENTATION)

  typedef detail::eager_impl<Signatures...> impl_type;

  detail::shared_ptr<impl_type> release() noexcept
  {
    return static_cast<detail::shared_ptr<impl_type>&&>(impl_);
  }

  detail::shared_ptr<impl_type> impl_;
  Executor executor_;
};

#if !defined(GENERATING_DOCUMENTATION)
template <typename Executor, typename... Signatures>
struct supports_piping<
    basic_eager_async_operation<Executor, Signatures...>>
  : true_type
{
};
#endif // !defined(GENERATING_DOCUMENTATION)

/// A handle to an eagerly-initiated asynchronous operation.
/**
 * A convenience alias for @ref basic_eager_async_operation that uses the
 * polymorphic @c any_io_executor. As the type records only the executor and
 * the completion signatures, and not the operation that produced it, eagerly
 * initiated operations of different kinds share this single type, and may be
 * stored together or passed across an interface that is not a template.
 *
 * An operation whose executor is a concrete type, or one that alters behaviour
 * such as a strand, has a distinct type of its own, but converts to this one
 * whenever its executor converts to @c any_io_executor.
 */
template <typename... Signatures>
using eager_async_operation
  = basic_eager_async_operation<any_io_executor, Signatures...>;

/// A @ref completion_token that eagerly initiates an asynchronous operation and
/// yields a @ref basic_eager_async_operation handle to await its result later.
/**
 * The @ref eager object may be passed as the completion token to an
 * asynchronous operation that has an associated I/O executor. The operation
 * is started immediately, and the call returns an eager operation that may be
 * waited on later. For example:
 *
 * @code
 * auto op = socket.async_read_some(buffer, asio::eager);
 *
 * // ... do other work while the read runs ...
 *
 * std::move(op)([](asio::error_code e, std::size_t n) { ... });
 * @endcode
 *
 * An operation with no associated executor, such as one produced by
 * asio::deferred::values, cannot be used with this token, as there
 * would be nowhere to deliver the result of the wait.
 *
 * Discarding the returned handle without waiting on it detaches from the
 * operation. See @ref basic_eager_async_operation for the requirements this
 * places on the caller.
 *
 * The optional @c Allocator is used to allocate the operation's shared state.
 * It is also associated with the handler given to the underlying operation,
 * and so is used for that operation's state as well.
 *
 * Before C++17, where @c std::variant is unavailable, this token supports an
 * operation with at most two completion signatures.
 *
 * The token is idempotent. Applying it to an operation that has already been
 * eagerly initiated yields that same operation, without creating a further
 * shared state, and an allocator specified on the subsequent application is
 * unused.
 *
 * @sa @ref overview_eager "Eager operations"
 */
template <typename Allocator = std::allocator<void>>
struct eager_t
{
  /// The allocator type. The allocator is used when constructing the shared
  /// state for a given asynchronous operation.
  typedef Allocator allocator_type;

  /// Construct using default-constructed allocator.
  constexpr eager_t()
  {
  }

  /// Construct using specified allocator.
  explicit eager_t(const Allocator& allocator)
    : allocator_(allocator)
  {
  }

  /// Obtain allocator.
  allocator_type get_allocator() const noexcept
  {
    return allocator_;
  }

  /// Adapts an executor to add the @c eager_t completion token as the default.
  template <typename InnerExecutor>
  struct executor_with_default : InnerExecutor
  {
    /// Specify @c eager_t as the default completion token type.
    typedef eager_t<Allocator> default_completion_token_type;

    /// The adapted executor type, without the default completion token.
    typedef InnerExecutor executor_without_default;

    /// Construct the adapted executor from the inner executor type.
    template <typename InnerExecutor1>
    executor_with_default(const InnerExecutor1& ex,
        constraint_t<
          conditional_t<
            !is_same<InnerExecutor1, executor_with_default>::value,
            is_convertible<InnerExecutor1, InnerExecutor>,
            false_type
          >::value
        > = 0) noexcept
      : InnerExecutor(ex)
    {
    }
  };

  /// Type alias to adapt an I/O object to use @c eager_t as its default
  /// completion token type.
  template <typename T>
  using as_default_on_t = typename T::template rebind_executor<
      executor_with_default<typename T::executor_type>>::other;

  /// Function helper to adapt an I/O object to use @c eager_t as its default
  /// completion token type.
  template <typename T>
  static typename decay_t<T>::template rebind_executor<
      executor_with_default<typename decay_t<T>::executor_type>
    >::other
  as_default_on(T&& object)
  {
    return typename decay_t<T>::template rebind_executor<
        executor_with_default<typename decay_t<T>::executor_type>
      >::other(static_cast<T&&>(object));
  }

  /// Specify an alternate allocator.
  template <typename OtherAllocator>
  eager_t<OtherAllocator> rebind(const OtherAllocator& allocator) const
  {
    return eager_t<OtherAllocator>(allocator);
  }

private:
  // Helper type to ensure that eager can be constexpr default-constructed even
  // when std::allocator<void> can't be.
  struct std_allocator_void
  {
    constexpr std_allocator_void()
    {
    }

    std_allocator_void(const std::allocator<void>&)
    {
    }

    operator std::allocator<void>() const
    {
      return std::allocator<void>();
    }
  };

  conditional_t<
    is_same<std::allocator<void>, Allocator>::value,
    std_allocator_void, Allocator> allocator_;
};

/// A @ref completion_token object that eagerly initiates an asynchronous
/// operation and yields an @ref eager_async_operation.
/**
 * See the documentation for asio::eager_t for a usage example.
 */
ASIO_INLINE_VARIABLE constexpr eager_t<> eager;

ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#include "asio/impl/eager.hpp"

#endif // ASIO_EAGER_HPP
