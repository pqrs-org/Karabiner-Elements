//
// experimental/impl/append.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_IMPL_EXPERIMENTAL_APPEND_HPP
#define ASIO_IMPL_EXPERIMENTAL_APPEND_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#include "asio/associator.hpp"
#include "asio/async_result.hpp"
#include "asio/detail/handler_alloc_helpers.hpp"
#include "asio/detail/handler_cont_helpers.hpp"
#include "asio/detail/handler_invoke_helpers.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/detail/variadic_templates.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace experimental {
namespace detail {

// Class to adapt a append_t as a completion handler.
template <typename Handler, typename... Values>
class append_handler
{
public:
  typedef void result_type;

  template <typename H>
  append_handler(ASIO_MOVE_ARG(H) handler, std::tuple<Values...> values)
    : handler_(ASIO_MOVE_CAST(H)(handler)),
      values_(ASIO_MOVE_CAST(std::tuple<Values...>)(values))
  {
  }

  template <typename... Args>
  void operator()(ASIO_MOVE_ARG(Args)... args)
  {
    this->invoke(
        std::make_index_sequence<sizeof...(Values)>{},
        ASIO_MOVE_CAST(Args)(args)...);
  }

  template <std::size_t... I, typename... Args>
  void invoke(std::index_sequence<I...>, ASIO_MOVE_ARG(Args)... args)
  {
    ASIO_MOVE_OR_LVALUE(Handler)(handler_)(
        ASIO_MOVE_CAST(Args)(args)...,
        ASIO_MOVE_CAST(Values)(std::get<I>(values_))...);
  }

//private:
  Handler handler_;
  std::tuple<Values...> values_;
};

template <typename Handler>
inline asio_handler_allocate_is_deprecated
asio_handler_allocate(std::size_t size,
    append_handler<Handler>* this_handler)
{
#if defined(ASIO_NO_DEPRECATED)
  asio_handler_alloc_helpers::allocate(size, this_handler->handler_);
  return asio_handler_allocate_is_no_longer_used();
#else // defined(ASIO_NO_DEPRECATED)
  return asio_handler_alloc_helpers::allocate(
      size, this_handler->handler_);
#endif // defined(ASIO_NO_DEPRECATED)
}

template <typename Handler>
inline asio_handler_deallocate_is_deprecated
asio_handler_deallocate(void* pointer, std::size_t size,
    append_handler<Handler>* this_handler)
{
  asio_handler_alloc_helpers::deallocate(
      pointer, size, this_handler->handler_);
#if defined(ASIO_NO_DEPRECATED)
  return asio_handler_deallocate_is_no_longer_used();
#endif // defined(ASIO_NO_DEPRECATED)
}

template <typename Handler>
inline bool asio_handler_is_continuation(
    append_handler<Handler>* this_handler)
{
  return asio_handler_cont_helpers::is_continuation(
        this_handler->handler_);
}

template <typename Function, typename Handler>
inline asio_handler_invoke_is_deprecated
asio_handler_invoke(Function& function,
    append_handler<Handler>* this_handler)
{
  asio_handler_invoke_helpers::invoke(
      function, this_handler->handler_);
#if defined(ASIO_NO_DEPRECATED)
  return asio_handler_invoke_is_no_longer_used();
#endif // defined(ASIO_NO_DEPRECATED)
}

template <typename Function, typename Handler>
inline asio_handler_invoke_is_deprecated
asio_handler_invoke(const Function& function,
    append_handler<Handler>* this_handler)
{
  asio_handler_invoke_helpers::invoke(
      function, this_handler->handler_);
#if defined(ASIO_NO_DEPRECATED)
  return asio_handler_invoke_is_no_longer_used();
#endif // defined(ASIO_NO_DEPRECATED)
}

template <typename Signature, typename... Values>
struct append_signature;

template <typename R, typename... Args, typename... Values>
struct append_signature<R(Args...), Values...>
{
  typedef R type(typename decay<Args>::type..., Values...);
};

} // namespace detail
} // namespace experimental

#if !defined(GENERATING_DOCUMENTATION)

template <typename CompletionToken, typename... Values, typename Signature>
struct async_result<
    experimental::append_t<CompletionToken, Values...>, Signature>
  : async_result<CompletionToken,
      typename experimental::detail::append_signature<
        Signature, Values...>::type>
{
  typedef typename experimental::detail::append_signature<
      Signature, Values...>::type signature;

  template <typename Initiation>
  struct init_wrapper
  {
    init_wrapper(Initiation init)
      : initiation_(ASIO_MOVE_CAST(Initiation)(init))
    {
    }

    template <typename Handler, typename... Args>
    void operator()(
        ASIO_MOVE_ARG(Handler) handler,
        std::tuple<Values...> values,
        ASIO_MOVE_ARG(Args)... args)
    {
      ASIO_MOVE_CAST(Initiation)(initiation_)(
          experimental::detail::append_handler<
            typename decay<Handler>::type, Values...>(
              ASIO_MOVE_CAST(Handler)(handler),
              ASIO_MOVE_CAST(std::tuple<Values...>)(values)),
          ASIO_MOVE_CAST(Args)(args)...);
    }

    Initiation initiation_;
  };

  template <typename Initiation, typename RawCompletionToken, typename... Args>
  static ASIO_INITFN_DEDUCED_RESULT_TYPE(CompletionToken, signature,
      (async_initiate<CompletionToken, signature>(
        declval<init_wrapper<typename decay<Initiation>::type> >(),
        declval<CompletionToken&>(),
        declval<std::tuple<Values...> >(),
        declval<ASIO_MOVE_ARG(Args)>()...)))
  initiate(
      ASIO_MOVE_ARG(Initiation) initiation,
      ASIO_MOVE_ARG(RawCompletionToken) token,
      ASIO_MOVE_ARG(Args)... args)
  {
    return async_initiate<CompletionToken, signature>(
        init_wrapper<typename decay<Initiation>::type>(
          ASIO_MOVE_CAST(Initiation)(initiation)),
        token.token_,
        ASIO_MOVE_CAST(std::tuple<Values...>)(token.values_),
        ASIO_MOVE_CAST(Args)(args)...);
  }
};

template <template <typename, typename> class Associator,
    typename Handler, typename DefaultCandidate>
struct associator<Associator,
    experimental::detail::append_handler<Handler>, DefaultCandidate>
  : Associator<Handler, DefaultCandidate>
{
  static typename Associator<Handler, DefaultCandidate>::type get(
      const experimental::detail::append_handler<Handler>& h,
      const DefaultCandidate& c = DefaultCandidate()) ASIO_NOEXCEPT
  {
    return Associator<Handler, DefaultCandidate>::get(h.handler_, c);
  }
};

#endif // !defined(GENERATING_DOCUMENTATION)

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_IMPL_EXPERIMENTAL_APPEND_HPP
