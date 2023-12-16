
// impl/redirect_error.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2023 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_IMPL_REDIRECT_ERROR_HPP
#define ASIO_IMPL_REDIRECT_ERROR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/associator.hpp"
#include "asio/async_result.hpp"
#include "asio/detail/handler_cont_helpers.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/system_error.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

// Class to adapt a redirect_error_t as a completion handler.
template <typename Handler>
class redirect_error_handler
{
public:
  typedef void result_type;

  template <typename CompletionToken>
  redirect_error_handler(redirect_error_t<CompletionToken> e)
    : ec_(e.ec_),
      handler_(static_cast<CompletionToken&&>(e.token_))
  {
  }

  template <typename RedirectedHandler>
  redirect_error_handler(asio::error_code& ec,
      RedirectedHandler&& h)
    : ec_(ec),
      handler_(static_cast<RedirectedHandler&&>(h))
  {
  }

  void operator()()
  {
    static_cast<Handler&&>(handler_)();
  }

  template <typename Arg, typename... Args>
  enable_if_t<
    !is_same<decay_t<Arg>, asio::error_code>::value
  >
  operator()(Arg&& arg, Args&&... args)
  {
    static_cast<Handler&&>(handler_)(
        static_cast<Arg&&>(arg),
        static_cast<Args&&>(args)...);
  }

  template <typename... Args>
  void operator()(const asio::error_code& ec, Args&&... args)
  {
    ec_ = ec;
    static_cast<Handler&&>(handler_)(static_cast<Args&&>(args)...);
  }

//private:
  asio::error_code& ec_;
  Handler handler_;
};

template <typename Handler>
inline bool asio_handler_is_continuation(
    redirect_error_handler<Handler>* this_handler)
{
  return asio_handler_cont_helpers::is_continuation(
        this_handler->handler_);
}

template <typename Signature>
struct redirect_error_signature
{
  typedef Signature type;
};

template <typename R, typename... Args>
struct redirect_error_signature<R(asio::error_code, Args...)>
{
  typedef R type(Args...);
};

template <typename R, typename... Args>
struct redirect_error_signature<R(const asio::error_code&, Args...)>
{
  typedef R type(Args...);
};

template <typename R, typename... Args>
struct redirect_error_signature<R(asio::error_code, Args...) &>
{
  typedef R type(Args...) &;
};

template <typename R, typename... Args>
struct redirect_error_signature<R(const asio::error_code&, Args...) &>
{
  typedef R type(Args...) &;
};

template <typename R, typename... Args>
struct redirect_error_signature<R(asio::error_code, Args...) &&>
{
  typedef R type(Args...) &&;
};

template <typename R, typename... Args>
struct redirect_error_signature<R(const asio::error_code&, Args...) &&>
{
  typedef R type(Args...) &&;
};

#if defined(ASIO_HAS_NOEXCEPT_FUNCTION_TYPE)

template <typename R, typename... Args>
struct redirect_error_signature<
  R(asio::error_code, Args...) noexcept>
{
  typedef R type(Args...) & noexcept;
};

template <typename R, typename... Args>
struct redirect_error_signature<
  R(const asio::error_code&, Args...) noexcept>
{
  typedef R type(Args...) & noexcept;
};

template <typename R, typename... Args>
struct redirect_error_signature<
  R(asio::error_code, Args...) & noexcept>
{
  typedef R type(Args...) & noexcept;
};

template <typename R, typename... Args>
struct redirect_error_signature<
  R(const asio::error_code&, Args...) & noexcept>
{
  typedef R type(Args...) & noexcept;
};

template <typename R, typename... Args>
struct redirect_error_signature<
  R(asio::error_code, Args...) && noexcept>
{
  typedef R type(Args...) && noexcept;
};

template <typename R, typename... Args>
struct redirect_error_signature<
  R(const asio::error_code&, Args...) && noexcept>
{
  typedef R type(Args...) && noexcept;
};

#endif // defined(ASIO_HAS_NOEXCEPT_FUNCTION_TYPE)

} // namespace detail

#if !defined(GENERATING_DOCUMENTATION)

template <typename CompletionToken, typename Signature>
struct async_result<redirect_error_t<CompletionToken>, Signature>
  : async_result<CompletionToken,
      typename detail::redirect_error_signature<Signature>::type>
{

  struct init_wrapper
  {
    explicit init_wrapper(asio::error_code& ec)
      : ec_(ec)
    {
    }

    template <typename Handler, typename Initiation, typename... Args>
    void operator()(Handler&& handler,
        Initiation&& initiation, Args&&... args) const
    {
      static_cast<Initiation&&>(initiation)(
          detail::redirect_error_handler<decay_t<Handler>>(
            ec_, static_cast<Handler&&>(handler)),
          static_cast<Args&&>(args)...);
    }

    asio::error_code& ec_;
  };

  template <typename Initiation, typename RawCompletionToken, typename... Args>
  static auto initiate(Initiation&& initiation,
      RawCompletionToken&& token, Args&&... args)
    -> decltype(
      async_initiate<CompletionToken,
        typename detail::redirect_error_signature<Signature>::type>(
          declval<init_wrapper>(), token.token_,
          static_cast<Initiation&&>(initiation),
          static_cast<Args&&>(args)...))
  {
    return async_initiate<CompletionToken,
      typename detail::redirect_error_signature<Signature>::type>(
        init_wrapper(token.ec_), token.token_,
        static_cast<Initiation&&>(initiation),
        static_cast<Args&&>(args)...);
  }
};

template <template <typename, typename> class Associator,
    typename Handler, typename DefaultCandidate>
struct associator<Associator,
    detail::redirect_error_handler<Handler>, DefaultCandidate>
  : Associator<Handler, DefaultCandidate>
{
  static typename Associator<Handler, DefaultCandidate>::type get(
      const detail::redirect_error_handler<Handler>& h) noexcept
  {
    return Associator<Handler, DefaultCandidate>::get(h.handler_);
  }

  static auto get(const detail::redirect_error_handler<Handler>& h,
      const DefaultCandidate& c) noexcept
    -> decltype(Associator<Handler, DefaultCandidate>::get(h.handler_, c))
  {
    return Associator<Handler, DefaultCandidate>::get(h.handler_, c);
  }
};

#endif // !defined(GENERATING_DOCUMENTATION)

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_IMPL_REDIRECT_ERROR_HPP
