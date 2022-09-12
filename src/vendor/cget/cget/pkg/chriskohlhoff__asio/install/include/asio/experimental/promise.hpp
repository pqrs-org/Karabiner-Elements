//
// experimental/promise.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2021-2022 Klemens D. Morgenstern
//                         (klemens dot morgenstern at gmx dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_EXPERIMENTAL_PROMISE_HPP
#define ASIO_EXPERIMENTAL_PROMISE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/any_io_executor.hpp"
#include "asio/associated_cancellation_slot.hpp"
#include "asio/bind_executor.hpp"
#include "asio/cancellation_signal.hpp"
#include "asio/experimental/detail/completion_handler_erasure.hpp"
#include "asio/experimental/impl/promise.hpp"
#include "asio/post.hpp"

#include <algorithm>
#include <variant>

#include "asio/detail/push_options.hpp"

namespace asio {
namespace experimental {

template <typename Executor = any_io_executor>
struct use_promise_t {};

constexpr use_promise_t<> use_promise;

template <typename T>
struct is_promise : std::false_type {};

template <typename ... Ts>
struct is_promise<promise<Ts...>> : std::true_type {};

template <typename T>
constexpr bool is_promise_v = is_promise<T>::value;

template <typename ... Ts>
struct promise_value_type
{
  using type = std::tuple<Ts...>;
};

template <typename T>
struct promise_value_type<T>
{
  using type = T;
};

template <>
struct promise_value_type<>
{
  using type = std::monostate;
};

#if defined(GENERATING_DOCUMENTATION)
/// The primary template is not defined.
template<typename Signature = void(), typename Executor = any_io_executor>
struct promise
{
};
#endif // defined(GENERATING_DOCUMENTATION)

template <typename ... Ts, typename Executor>
struct promise<void(Ts...), Executor>
{
  using value_type = typename promise_value_type<Ts...>::type;
  using tuple_type = std::tuple<Ts...>;
  using executor_type = Executor;

  /// Rebinds the promise type to another executor.
  template <typename Executor1>
  struct rebind_executor
  {
    /// The file type when rebound to the specified executor.
    typedef promise<void(Ts...), Executor1> other;
  };

  /// Get the executor of the promise
  executor_type get_executor() const
  {
    if (impl_)
      return impl_->executor;
    else
      return {};
  }

  /// Cancel the promise. Usually done through the destructor.
  void cancel(cancellation_type level = cancellation_type::all)
  {
    if (impl_ && !impl_->done)
    {
      asio::dispatch(impl_->executor,
          [level, impl = impl_]{impl->cancel.emit(level);});
    }
  }

  /// Check if the promise is completed already.
  bool complete() const noexcept
  {
    return impl_ && impl_->done;
  }

  /// Wait for the promise to become ready.
  template <
    ASIO_COMPLETION_TOKEN_FOR(void(Ts...)) CompletionToken
      ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(executor_type)>
  auto async_wait(CompletionToken&& token
      ASIO_DEFAULT_COMPLETION_TOKEN(executor_type))
  {
    assert(impl_);

    return async_initiate<CompletionToken, void(Ts...)>(
        initiate_async_wait{impl_}, token);
  }

  promise() = delete;
  promise(const promise& ) = delete;
  promise(promise&& ) noexcept = default;

  ~promise() { cancel(); }


private:
#if !defined(GENERATING_DOCUMENTATION)
  template <typename, typename> friend struct promise;
  friend struct detail::promise_handler<void(Ts...)>;
#endif // !defined(GENERATING_DOCUMENTATION)

  std::shared_ptr<detail::promise_impl<void(Ts...), Executor>> impl_;
  promise(std::shared_ptr<detail::promise_impl<void(Ts...), Executor>> impl)
    : impl_(impl)
  {
  }

  struct initiate_async_wait
  {
    std::shared_ptr<detail::promise_impl<void(Ts...), Executor>> self_;

    template <typename WaitHandler>
    void operator()(WaitHandler&& handler) const
    {
      const auto exec = get_associated_executor(handler, self_->executor);
      auto cancel = get_associated_cancellation_slot(handler);
      if (self_->done)
      {
        asio::post(exec,
            [self = self_, h = std::forward<WaitHandler>(handler)]() mutable
            {
              std::apply(std::forward<WaitHandler>(h),
                  std::move(*self->result));
            });
      }
      else
      {
        if (cancel.is_connected())
        {
          struct cancel_handler
          {
            std::weak_ptr<detail::promise_impl<void(Ts...), Executor>> self;

            cancel_handler(
                std::weak_ptr<detail::promise_impl<void(Ts...), Executor>> self)
              : self(std::move(self))
            {
            }

            void operator()(cancellation_type level) const
            {
              if (auto p = self.lock(); p != nullptr)
                p->cancel.emit(level);

            }
          };

          cancel.template emplace<cancel_handler>(self_);
        }

        self_->completion = {exec, std::forward<WaitHandler>(handler)};
      }
    }
  };
};

} // namespace experimental

#if !defined(GENERATING_DOCUMENTATION)

template <typename Executor, typename R, typename... Args>
struct async_result<experimental::use_promise_t<Executor>, R(Args...)>
{
  using handler_type = experimental::detail::promise_handler<
    void(typename decay<Args>::type...), Executor>;

  using return_type = experimental::promise<
    void(typename decay<Args>::type...), Executor>;

  template <typename Initiation, typename... InitArgs>
  static auto initiate(Initiation initiation,
      experimental::use_promise_t<Executor>, InitArgs... args)
    -> typename handler_type::promise_type
  {
    handler_type ht{get_associated_executor(initiation)};
    std::move(initiation)(ht, std::move(args)...);
    return ht.make_promise();
  }
};

#endif // !defined(GENERATING_DOCUMENTATION)

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_EXPERIMENTAL_PROMISE_HPP
