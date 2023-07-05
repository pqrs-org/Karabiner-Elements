//
// async_result.hpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2023 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_ASYNC_RESULT_HPP
#define ASIO_ASYNC_RESULT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/detail/variadic_templates.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {

#if defined(ASIO_HAS_CONCEPTS) \
  && defined(ASIO_HAS_VARIADIC_TEMPLATES) \
  && defined(ASIO_HAS_DECLTYPE)

namespace detail {

template <typename T>
struct is_completion_signature : false_type
{
};

template <typename R, typename... Args>
struct is_completion_signature<R(Args...)> : true_type
{
};

#if defined(ASIO_HAS_REF_QUALIFIED_FUNCTIONS)

template <typename R, typename... Args>
struct is_completion_signature<R(Args...) &> : true_type
{
};

template <typename R, typename... Args>
struct is_completion_signature<R(Args...) &&> : true_type
{
};

# if defined(ASIO_HAS_NOEXCEPT_FUNCTION_TYPE)

template <typename R, typename... Args>
struct is_completion_signature<R(Args...) noexcept> : true_type
{
};

template <typename R, typename... Args>
struct is_completion_signature<R(Args...) & noexcept> : true_type
{
};

template <typename R, typename... Args>
struct is_completion_signature<R(Args...) && noexcept> : true_type
{
};

# endif // defined(ASIO_HAS_NOEXCEPT_FUNCTION_TYPE)
#endif // defined(ASIO_HAS_REF_QUALIFIED_FUNCTIONS)

template <typename... T>
struct are_completion_signatures : false_type
{
};

template <typename T0>
struct are_completion_signatures<T0>
  : is_completion_signature<T0>
{
};

template <typename T0, typename... TN>
struct are_completion_signatures<T0, TN...>
  : integral_constant<bool, (
      is_completion_signature<T0>::value
        && are_completion_signatures<TN...>::value)>
{
};

template <typename T, typename... Args>
ASIO_CONCEPT callable_with = requires(T&& t, Args&&... args)
{
  static_cast<T&&>(t)(static_cast<Args&&>(args)...);
};

template <typename T, typename... Signatures>
struct is_completion_handler_for : false_type
{
};

template <typename T, typename R, typename... Args>
struct is_completion_handler_for<T, R(Args...)>
  : integral_constant<bool, (callable_with<T, Args...>)>
{
};

#if defined(ASIO_HAS_REF_QUALIFIED_FUNCTIONS)

template <typename T, typename R, typename... Args>
struct is_completion_handler_for<T, R(Args...) &>
  : integral_constant<bool, (callable_with<T&, Args...>)>
{
};

template <typename T, typename R, typename... Args>
struct is_completion_handler_for<T, R(Args...) &&>
  : integral_constant<bool, (callable_with<T&&, Args...>)>
{
};

# if defined(ASIO_HAS_NOEXCEPT_FUNCTION_TYPE)

template <typename T, typename R, typename... Args>
struct is_completion_handler_for<T, R(Args...) noexcept>
  : integral_constant<bool, (callable_with<T, Args...>)>
{
};

template <typename T, typename R, typename... Args>
struct is_completion_handler_for<T, R(Args...) & noexcept>
  : integral_constant<bool, (callable_with<T&, Args...>)>
{
};

template <typename T, typename R, typename... Args>
struct is_completion_handler_for<T, R(Args...) && noexcept>
  : integral_constant<bool, (callable_with<T&&, Args...>)>
{
};

# endif // defined(ASIO_HAS_NOEXCEPT_FUNCTION_TYPE)
#endif // defined(ASIO_HAS_REF_QUALIFIED_FUNCTIONS)

template <typename T, typename Signature0, typename... SignatureN>
struct is_completion_handler_for<T, Signature0, SignatureN...>
  : integral_constant<bool, (
      is_completion_handler_for<T, Signature0>::value
        && is_completion_handler_for<T, SignatureN...>::value)>
{
};

} // namespace detail

template <typename T>
ASIO_CONCEPT completion_signature =
  detail::is_completion_signature<T>::value;

#define ASIO_COMPLETION_SIGNATURE \
  ::asio::completion_signature

template <typename T, typename... Signatures>
ASIO_CONCEPT completion_handler_for =
  detail::are_completion_signatures<Signatures...>::value
    && detail::is_completion_handler_for<T, Signatures...>::value;

#define ASIO_COMPLETION_HANDLER_FOR(sig) \
  ::asio::completion_handler_for<sig>
#define ASIO_COMPLETION_HANDLER_FOR2(sig0, sig1) \
  ::asio::completion_handler_for<sig0, sig1>
#define ASIO_COMPLETION_HANDLER_FOR3(sig0, sig1, sig2) \
  ::asio::completion_handler_for<sig0, sig1, sig2>

#else // defined(ASIO_HAS_CONCEPTS)
      //   && defined(ASIO_HAS_VARIADIC_TEMPLATES)
      //   && defined(ASIO_HAS_DECLTYPE)

#define ASIO_COMPLETION_SIGNATURE typename
#define ASIO_COMPLETION_HANDLER_FOR(sig) typename
#define ASIO_COMPLETION_HANDLER_FOR2(sig0, sig1) typename
#define ASIO_COMPLETION_HANDLER_FOR3(sig0, sig1, sig2) typename

#endif // defined(ASIO_HAS_CONCEPTS)
       //   && defined(ASIO_HAS_VARIADIC_TEMPLATES)
       //   && defined(ASIO_HAS_DECLTYPE)

namespace detail {

template <typename T>
struct is_simple_completion_signature : false_type
{
};

template <typename T>
struct simple_completion_signature;

#if defined(ASIO_HAS_VARIADIC_TEMPLATES)

template <typename R, typename... Args>
struct is_simple_completion_signature<R(Args...)> : true_type
{
};

template <typename... Signatures>
struct are_simple_completion_signatures : false_type
{
};

template <typename Sig0>
struct are_simple_completion_signatures<Sig0>
  : is_simple_completion_signature<Sig0>
{
};

template <typename Sig0, typename... SigN>
struct are_simple_completion_signatures<Sig0, SigN...>
  : integral_constant<bool, (
      is_simple_completion_signature<Sig0>::value
        && are_simple_completion_signatures<SigN...>::value)>
{
};

template <typename R, typename... Args>
struct simple_completion_signature<R(Args...)>
{
  typedef R type(Args...);
};

#if defined(ASIO_HAS_REF_QUALIFIED_FUNCTIONS)

template <typename R, typename... Args>
struct simple_completion_signature<R(Args...) &>
{
  typedef R type(Args...);
};

template <typename R, typename... Args>
struct simple_completion_signature<R(Args...) &&>
{
  typedef R type(Args...);
};

# if defined(ASIO_HAS_NOEXCEPT_FUNCTION_TYPE)

template <typename R, typename... Args>
struct simple_completion_signature<R(Args...) noexcept>
{
  typedef R type(Args...);
};

template <typename R, typename... Args>
struct simple_completion_signature<R(Args...) & noexcept>
{
  typedef R type(Args...);
};

template <typename R, typename... Args>
struct simple_completion_signature<R(Args...) && noexcept>
{
  typedef R type(Args...);
};

# endif // defined(ASIO_HAS_NOEXCEPT_FUNCTION_TYPE)
#endif // defined(ASIO_HAS_REF_QUALIFIED_FUNCTIONS)

#else // defined(ASIO_HAS_VARIADIC_TEMPLATES)

template <typename R>
struct is_simple_completion_signature<R()> : true_type
{
};

#define ASIO_PRIVATE_SIMPLE_SIG_DEF(n) \
  template <typename R, ASIO_VARIADIC_TPARAMS(n)> \
  struct is_simple_completion_signature<R(ASIO_VARIADIC_TARGS(n))> \
    : true_type \
  { \
  }; \
  /**/
  ASIO_VARIADIC_GENERATE(ASIO_PRIVATE_SIMPLE_SIG_DEF)
#undef ASIO_PRIVATE_SIMPLE_SIG_DEF

template <typename Sig0 = void, typename Sig1 = void,
    typename Sig2 = void, typename = void>
struct are_simple_completion_signatures : false_type
{
};

template <typename Sig0>
struct are_simple_completion_signatures<Sig0>
  : is_simple_completion_signature<Sig0>
{
};

template <typename Sig0, typename Sig1>
struct are_simple_completion_signatures<Sig0, Sig1>
  : integral_constant<bool,
      (is_simple_completion_signature<Sig0>::value
        && is_simple_completion_signature<Sig1>::value)>
{
};

template <typename Sig0, typename Sig1, typename Sig2>
struct are_simple_completion_signatures<Sig0, Sig1, Sig2>
  : integral_constant<bool,
      (is_simple_completion_signature<Sig0>::value
        && is_simple_completion_signature<Sig1>::value
        && is_simple_completion_signature<Sig2>::value)>
{
};

template <>
struct simple_completion_signature<void>
{
  typedef void type;
};

template <typename R>
struct simple_completion_signature<R()>
{
  typedef R type();
};

#define ASIO_PRIVATE_SIMPLE_SIG_DEF(n) \
  template <typename R, ASIO_VARIADIC_TPARAMS(n)> \
  struct simple_completion_signature<R(ASIO_VARIADIC_TARGS(n))> \
  { \
    typedef R type(ASIO_VARIADIC_TARGS(n)); \
  }; \
  /**/
  ASIO_VARIADIC_GENERATE(ASIO_PRIVATE_SIMPLE_SIG_DEF)
#undef ASIO_PRIVATE_SIMPLE_SIG_DEF

#if defined(ASIO_HAS_REF_QUALIFIED_FUNCTIONS)

template <typename R>
struct simple_completion_signature<R() &>
{
  typedef R type();
};

template <typename R>
struct simple_completion_signature<R() &&>
{
  typedef R type();
};

#define ASIO_PRIVATE_SIMPLE_SIG_DEF(n) \
  template <typename R, ASIO_VARIADIC_TPARAMS(n)> \
  struct simple_completion_signature< \
    R(ASIO_VARIADIC_TARGS(n)) &> \
  { \
    typedef R type(ASIO_VARIADIC_TARGS(n)); \
  }; \
  \
  template <typename R, ASIO_VARIADIC_TPARAMS(n)> \
  struct simple_completion_signature< \
    R(ASIO_VARIADIC_TARGS(n)) &&> \
  { \
    typedef R type(ASIO_VARIADIC_TARGS(n)); \
  }; \
  /**/
  ASIO_VARIADIC_GENERATE(ASIO_PRIVATE_SIMPLE_SIG_DEF)
#undef ASIO_PRIVATE_SIMPLE_SIG_DEF

# if defined(ASIO_HAS_NOEXCEPT_FUNCTION_TYPE)

template <typename R>
struct simple_completion_signature<R() noexcept>
{
  typedef R type();
};

template <typename R>
struct simple_completion_signature<R() & noexcept>
{
  typedef R type();
};

template <typename R>
struct simple_completion_signature<R() && noexcept>
{
  typedef R type();
};

#define ASIO_PRIVATE_SIMPLE_SIG_DEF(n) \
  template <typename R, ASIO_VARIADIC_TPARAMS(n)> \
  struct simple_completion_signature< \
    R(ASIO_VARIADIC_TARGS(n)) noexcept> \
  { \
    typedef R type(ASIO_VARIADIC_TARGS(n)); \
  }; \
  \
  template <typename R, ASIO_VARIADIC_TPARAMS(n)> \
  struct simple_completion_signature< \
    R(ASIO_VARIADIC_TARGS(n)) & noexcept> \
  { \
    typedef R type(ASIO_VARIADIC_TARGS(n)); \
  }; \
  \
  template <typename R, ASIO_VARIADIC_TPARAMS(n)> \
  struct simple_completion_signature< \
    R(ASIO_VARIADIC_TARGS(n)) && noexcept> \
  { \
    typedef R type(ASIO_VARIADIC_TARGS(n)); \
  }; \
  /**/
  ASIO_VARIADIC_GENERATE(ASIO_PRIVATE_SIMPLE_SIG_DEF)
#undef ASIO_PRIVATE_SIMPLE_SIG_DEF

# endif // defined(ASIO_HAS_NOEXCEPT_FUNCTION_TYPE)
#endif // defined(ASIO_HAS_REF_QUALIFIED_FUNCTIONS)

#endif // defined(ASIO_HAS_VARIADIC_TEMPLATES)

#if defined(ASIO_HAS_VARIADIC_TEMPLATES) \
  || defined(GENERATING_DOCUMENTATION)

# define ASIO_COMPLETION_SIGNATURES_TPARAMS \
    ASIO_COMPLETION_SIGNATURE... Signatures

# define ASIO_COMPLETION_SIGNATURES_TSPECPARAMS \
    ASIO_COMPLETION_SIGNATURE... Signatures

# define ASIO_COMPLETION_SIGNATURES_TARGS Signatures...

# define ASIO_COMPLETION_SIGNATURES_TSIMPLEARGS \
    typename asio::detail::simple_completion_signature< \
      Signatures>::type...

#else // defined(ASIO_HAS_VARIADIC_TEMPLATES)
      //   || defined(GENERATING_DOCUMENTATION)

# define ASIO_COMPLETION_SIGNATURES_TPARAMS \
    typename Sig0 = void, \
    typename Sig1 = void, \
    typename Sig2 = void

# define ASIO_COMPLETION_SIGNATURES_TSPECPARAMS \
    typename Sig0, \
    typename Sig1, \
    typename Sig2

# define ASIO_COMPLETION_SIGNATURES_TARGS Sig0, Sig1, Sig2

# define ASIO_COMPLETION_SIGNATURES_TSIMPLEARGS \
    typename ::asio::detail::simple_completion_signature<Sig0>::type, \
    typename ::asio::detail::simple_completion_signature<Sig1>::type, \
    typename ::asio::detail::simple_completion_signature<Sig2>::type

#endif // defined(ASIO_HAS_VARIADIC_TEMPLATES)
       //   || defined(GENERATING_DOCUMENTATION)

template <typename CompletionToken, ASIO_COMPLETION_SIGNATURES_TPARAMS>
class completion_handler_async_result
{
public:
  typedef CompletionToken completion_handler_type;
  typedef void return_type;

  explicit completion_handler_async_result(completion_handler_type&)
  {
  }

  return_type get()
  {
  }

#if defined(ASIO_HAS_VARIADIC_TEMPLATES)

  template <typename Initiation,
      ASIO_COMPLETION_HANDLER_FOR(Signatures...) RawCompletionToken,
      typename... Args>
  static return_type initiate(
      ASIO_MOVE_ARG(Initiation) initiation,
      ASIO_MOVE_ARG(RawCompletionToken) token,
      ASIO_MOVE_ARG(Args)... args)
  {
    ASIO_MOVE_CAST(Initiation)(initiation)(
        ASIO_MOVE_CAST(RawCompletionToken)(token),
        ASIO_MOVE_CAST(Args)(args)...);
  }

#else // defined(ASIO_HAS_VARIADIC_TEMPLATES)

  template <typename Initiation, typename RawCompletionToken>
  static return_type initiate(
      ASIO_MOVE_ARG(Initiation) initiation,
      ASIO_MOVE_ARG(RawCompletionToken) token)
  {
    ASIO_MOVE_CAST(Initiation)(initiation)(
        ASIO_MOVE_CAST(RawCompletionToken)(token));
  }

#define ASIO_PRIVATE_INITIATE_DEF(n) \
  template <typename Initiation, \
      typename RawCompletionToken, \
      ASIO_VARIADIC_TPARAMS(n)> \
  static return_type initiate( \
      ASIO_MOVE_ARG(Initiation) initiation, \
      ASIO_MOVE_ARG(RawCompletionToken) token, \
      ASIO_VARIADIC_MOVE_PARAMS(n)) \
  { \
    ASIO_MOVE_CAST(Initiation)(initiation)( \
        ASIO_MOVE_CAST(RawCompletionToken)(token), \
        ASIO_VARIADIC_MOVE_ARGS(n)); \
  } \
  /**/
  ASIO_VARIADIC_GENERATE(ASIO_PRIVATE_INITIATE_DEF)
#undef ASIO_PRIVATE_INITIATE_DEF

#endif // defined(ASIO_HAS_VARIADIC_TEMPLATES)

private:
  completion_handler_async_result(
      const completion_handler_async_result&) ASIO_DELETED;
  completion_handler_async_result& operator=(
      const completion_handler_async_result&) ASIO_DELETED;
};

} // namespace detail

#if defined(GENERATING_DOCUMENTATION)

/// An interface for customising the behaviour of an initiating function.
/**
 * The async_result traits class is used for determining:
 *
 * @li the concrete completion handler type to be called at the end of the
 * asynchronous operation;
 *
 * @li the initiating function return type; and
 *
 * @li how the return value of the initiating function is obtained.
 *
 * The trait allows the handler and return types to be determined at the point
 * where the specific completion handler signature is known.
 *
 * This template may be specialised for user-defined completion token types.
 * The primary template assumes that the CompletionToken is the completion
 * handler.
 */
template <typename CompletionToken, ASIO_COMPLETION_SIGNATURES_TPARAMS>
class async_result
{
public:
  /// The concrete completion handler type for the specific signature.
  typedef CompletionToken completion_handler_type;

  /// The return type of the initiating function.
  typedef void return_type;

  /// Construct an async result from a given handler.
  /**
   * When using a specalised async_result, the constructor has an opportunity
   * to initialise some state associated with the completion handler, which is
   * then returned from the initiating function.
   */
  explicit async_result(completion_handler_type& h);

  /// Obtain the value to be returned from the initiating function.
  return_type get();

  /// Initiate the asynchronous operation that will produce the result, and
  /// obtain the value to be returned from the initiating function.
  template <typename Initiation, typename RawCompletionToken, typename... Args>
  static return_type initiate(
      ASIO_MOVE_ARG(Initiation) initiation,
      ASIO_MOVE_ARG(RawCompletionToken) token,
      ASIO_MOVE_ARG(Args)... args);

private:
  async_result(const async_result&) ASIO_DELETED;
  async_result& operator=(const async_result&) ASIO_DELETED;
};

#else // defined(GENERATING_DOCUMENTATION)

#if defined(ASIO_HAS_REF_QUALIFIED_FUNCTIONS)

template <typename CompletionToken, ASIO_COMPLETION_SIGNATURES_TPARAMS>
class async_result :
  public conditional<
      detail::are_simple_completion_signatures<
        ASIO_COMPLETION_SIGNATURES_TARGS>::value,
      detail::completion_handler_async_result<
        CompletionToken, ASIO_COMPLETION_SIGNATURES_TARGS>,
      async_result<CompletionToken,
        ASIO_COMPLETION_SIGNATURES_TSIMPLEARGS>
    >::type
{
public:
  typedef typename conditional<
      detail::are_simple_completion_signatures<
        ASIO_COMPLETION_SIGNATURES_TARGS>::value,
      detail::completion_handler_async_result<
        CompletionToken, ASIO_COMPLETION_SIGNATURES_TARGS>,
      async_result<CompletionToken,
        ASIO_COMPLETION_SIGNATURES_TSIMPLEARGS>
    >::type base_type;

  using base_type::base_type;

private:
  async_result(const async_result&) ASIO_DELETED;
  async_result& operator=(const async_result&) ASIO_DELETED;
};

#else // defined(ASIO_HAS_REF_QUALIFIED_FUNCTIONS)

template <typename CompletionToken, ASIO_COMPLETION_SIGNATURES_TPARAMS>
class async_result :
  public detail::completion_handler_async_result<
    CompletionToken, ASIO_COMPLETION_SIGNATURES_TARGS>
{
public:
  explicit async_result(CompletionToken& h)
    : detail::completion_handler_async_result<
        CompletionToken, ASIO_COMPLETION_SIGNATURES_TARGS>(h)
  {
  }

private:
  async_result(const async_result&) ASIO_DELETED;
  async_result& operator=(const async_result&) ASIO_DELETED;
};

#endif // defined(ASIO_HAS_REF_QUALIFIED_FUNCTIONS)

template <ASIO_COMPLETION_SIGNATURES_TSPECPARAMS>
class async_result<void, ASIO_COMPLETION_SIGNATURES_TARGS>
{
  // Empty.
};

#endif // defined(GENERATING_DOCUMENTATION)

/// Helper template to deduce the handler type from a CompletionToken, capture
/// a local copy of the handler, and then create an async_result for the
/// handler.
template <typename CompletionToken, ASIO_COMPLETION_SIGNATURES_TPARAMS>
struct async_completion
{
  /// The real handler type to be used for the asynchronous operation.
  typedef typename asio::async_result<
    typename decay<CompletionToken>::type,
      ASIO_COMPLETION_SIGNATURES_TARGS>::completion_handler_type
        completion_handler_type;

#if defined(ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)
  /// Constructor.
  /**
   * The constructor creates the concrete completion handler and makes the link
   * between the handler and the asynchronous result.
   */
  explicit async_completion(CompletionToken& token)
    : completion_handler(static_cast<typename conditional<
        is_same<CompletionToken, completion_handler_type>::value,
        completion_handler_type&, CompletionToken&&>::type>(token)),
      result(completion_handler)
  {
  }
#else // defined(ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)
  explicit async_completion(typename decay<CompletionToken>::type& token)
    : completion_handler(token),
      result(completion_handler)
  {
  }

  explicit async_completion(const typename decay<CompletionToken>::type& token)
    : completion_handler(token),
      result(completion_handler)
  {
  }
#endif // defined(ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)

  /// A copy of, or reference to, a real handler object.
#if defined(ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)
  typename conditional<
    is_same<CompletionToken, completion_handler_type>::value,
    completion_handler_type&, completion_handler_type>::type completion_handler;
#else // defined(ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)
  completion_handler_type completion_handler;
#endif // defined(ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)

  /// The result of the asynchronous operation's initiating function.
  async_result<typename decay<CompletionToken>::type,
    ASIO_COMPLETION_SIGNATURES_TARGS> result;
};

namespace detail {

template <typename CompletionToken, ASIO_COMPLETION_SIGNATURES_TPARAMS>
struct async_result_helper
  : async_result<typename decay<CompletionToken>::type,
      ASIO_COMPLETION_SIGNATURES_TARGS>
{
};

struct async_result_memfns_base
{
  void initiate();
};

template <typename T>
struct async_result_memfns_derived
  : T, async_result_memfns_base
{
};

template <typename T, T>
struct async_result_memfns_check
{
};

template <typename>
char (&async_result_initiate_memfn_helper(...))[2];

template <typename T>
char async_result_initiate_memfn_helper(
    async_result_memfns_check<
      void (async_result_memfns_base::*)(),
      &async_result_memfns_derived<T>::initiate>*);

template <typename CompletionToken, ASIO_COMPLETION_SIGNATURES_TPARAMS>
struct async_result_has_initiate_memfn
  : integral_constant<bool, sizeof(async_result_initiate_memfn_helper<
      async_result<typename decay<CompletionToken>::type,
        ASIO_COMPLETION_SIGNATURES_TARGS>
    >(0)) != 1>
{
};

} // namespace detail

#if defined(GENERATING_DOCUMENTATION)
# define ASIO_INITFN_RESULT_TYPE(ct, sig) \
  void_or_deduced
# define ASIO_INITFN_RESULT_TYPE2(ct, sig0, sig1) \
  void_or_deduced
# define ASIO_INITFN_RESULT_TYPE3(ct, sig0, sig1, sig2) \
  void_or_deduced
#elif defined(_MSC_VER) && (_MSC_VER < 1500)
# define ASIO_INITFN_RESULT_TYPE(ct, sig) \
  typename ::asio::detail::async_result_helper< \
    ct, sig>::return_type
# define ASIO_INITFN_RESULT_TYPE2(ct, sig0, sig1) \
  typename ::asio::detail::async_result_helper< \
    ct, sig0, sig1>::return_type
# define ASIO_INITFN_RESULT_TYPE3(ct, sig0, sig1, sig2) \
  typename ::asio::detail::async_result_helper< \
    ct, sig0, sig1, sig2>::return_type
#define ASIO_HANDLER_TYPE(ct, sig) \
  typename ::asio::detail::async_result_helper< \
    ct, sig>::completion_handler_type
#define ASIO_HANDLER_TYPE2(ct, sig0, sig1) \
  typename ::asio::detail::async_result_helper< \
    ct, sig0, sig1>::completion_handler_type
#define ASIO_HANDLER_TYPE3(ct, sig0, sig1, sig2) \
  typename ::asio::detail::async_result_helper< \
    ct, sig0, sig1, sig2>::completion_handler_type
#else
# define ASIO_INITFN_RESULT_TYPE(ct, sig) \
  typename ::asio::async_result< \
    typename ::asio::decay<ct>::type, sig>::return_type
# define ASIO_INITFN_RESULT_TYPE2(ct, sig0, sig1) \
  typename ::asio::async_result< \
    typename ::asio::decay<ct>::type, sig0, sig1>::return_type
# define ASIO_INITFN_RESULT_TYPE3(ct, sig0, sig1, sig2) \
  typename ::asio::async_result< \
    typename ::asio::decay<ct>::type, sig0, sig1, sig2>::return_type
#define ASIO_HANDLER_TYPE(ct, sig) \
  typename ::asio::async_result< \
    typename ::asio::decay<ct>::type, sig>::completion_handler_type
#define ASIO_HANDLER_TYPE2(ct, sig0, sig1) \
  typename ::asio::async_result< \
    typename ::asio::decay<ct>::type, \
      sig0, sig1>::completion_handler_type
#define ASIO_HANDLER_TYPE3(ct, sig0, sig1, sig2) \
  typename ::asio::async_result< \
    typename ::asio::decay<ct>::type, \
      sig0, sig1, sig2>::completion_handler_type
#endif

#if defined(GENERATING_DOCUMENTATION)
# define ASIO_INITFN_AUTO_RESULT_TYPE(ct, sig) \
  auto
# define ASIO_INITFN_AUTO_RESULT_TYPE2(ct, sig0, sig1) \
  auto
# define ASIO_INITFN_AUTO_RESULT_TYPE3(ct, sig0, sig1, sig2) \
  auto
#elif defined(ASIO_HAS_RETURN_TYPE_DEDUCTION)
# define ASIO_INITFN_AUTO_RESULT_TYPE(ct, sig) \
  auto
# define ASIO_INITFN_AUTO_RESULT_TYPE2(ct, sig0, sig1) \
  auto
# define ASIO_INITFN_AUTO_RESULT_TYPE3(ct, sig0, sig1, sig2) \
  auto
#else
# define ASIO_INITFN_AUTO_RESULT_TYPE(ct, sig) \
  ASIO_INITFN_RESULT_TYPE(ct, sig)
# define ASIO_INITFN_AUTO_RESULT_TYPE2(ct, sig0, sig1) \
  ASIO_INITFN_RESULT_TYPE2(ct, sig0, sig1)
# define ASIO_INITFN_AUTO_RESULT_TYPE3(ct, sig0, sig1, sig2) \
  ASIO_INITFN_RESULT_TYPE3(ct, sig0, sig1, sig2)
#endif

#if defined(GENERATING_DOCUMENTATION)
# define ASIO_INITFN_AUTO_RESULT_TYPE_PREFIX(ct, sig) \
  auto
# define ASIO_INITFN_AUTO_RESULT_TYPE_PREFIX2(ct, sig0, sig1) \
  auto
# define ASIO_INITFN_AUTO_RESULT_TYPE_PREFIX3(ct, sig0, sig1, sig2) \
  auto
# define ASIO_INITFN_AUTO_RESULT_TYPE_SUFFIX(expr)
#elif defined(ASIO_HAS_RETURN_TYPE_DEDUCTION)
# define ASIO_INITFN_AUTO_RESULT_TYPE_PREFIX(ct, sig) \
  auto
# define ASIO_INITFN_AUTO_RESULT_TYPE_PREFIX2(ct, sig0, sig1) \
  auto
# define ASIO_INITFN_AUTO_RESULT_TYPE_PREFIX3(ct, sig0, sig1, sig2) \
  auto
# define ASIO_INITFN_AUTO_RESULT_TYPE_SUFFIX(expr)
#elif defined(ASIO_HAS_DECLTYPE)
# define ASIO_INITFN_AUTO_RESULT_TYPE_PREFIX(ct, sig) \
  auto
# define ASIO_INITFN_AUTO_RESULT_TYPE_PREFIX2(ct, sig0, sig1) \
  auto
# define ASIO_INITFN_AUTO_RESULT_TYPE_PREFIX3(ct, sig0, sig1, sig2) \
  auto
# define ASIO_INITFN_AUTO_RESULT_TYPE_SUFFIX(expr) -> decltype expr
#else
# define ASIO_INITFN_AUTO_RESULT_TYPE_PREFIX(ct, sig) \
  ASIO_INITFN_RESULT_TYPE(ct, sig)
# define ASIO_INITFN_AUTO_RESULT_TYPE_PREFIX2(ct, sig0, sig1) \
  ASIO_INITFN_RESULT_TYPE2(ct, sig0, sig1)
# define ASIO_INITFN_AUTO_RESULT_TYPE_PREFIX3(ct, sig0, sig1, sig2) \
  ASIO_INITFN_RESULT_TYPE3(ct, sig0, sig1, sig2)
# define ASIO_INITFN_AUTO_RESULT_TYPE_SUFFIX(expr)
#endif

#if defined(GENERATING_DOCUMENTATION)
# define ASIO_INITFN_DEDUCED_RESULT_TYPE(ct, sig, expr) \
  void_or_deduced
# define ASIO_INITFN_DEDUCED_RESULT_TYPE2(ct, sig0, sig1, expr) \
  void_or_deduced
# define ASIO_INITFN_DEDUCED_RESULT_TYPE3(ct, sig0, sig1, sig2, expr) \
  void_or_deduced
#elif defined(ASIO_HAS_DECLTYPE)
# define ASIO_INITFN_DEDUCED_RESULT_TYPE(ct, sig, expr) \
  decltype expr
# define ASIO_INITFN_DEDUCED_RESULT_TYPE2(ct, sig0, sig1, expr) \
  decltype expr
# define ASIO_INITFN_DEDUCED_RESULT_TYPE3(ct, sig0, sig1, sig2, expr) \
  decltype expr
#else
# define ASIO_INITFN_DEDUCED_RESULT_TYPE(ct, sig, expr) \
  ASIO_INITFN_RESULT_TYPE(ct, sig)
# define ASIO_INITFN_DEDUCED_RESULT_TYPE2(ct, sig0, sig1, expr) \
  ASIO_INITFN_RESULT_TYPE2(ct, sig0, sig1)
# define ASIO_INITFN_DEDUCED_RESULT_TYPE3(ct, sig0, sig1, sig2, expr) \
  ASIO_INITFN_RESULT_TYPE3(ct, sig0, sig1, sig2)
#endif

#if defined(GENERATING_DOCUMENTATION)

template <typename CompletionToken,
    completion_signature... Signatures,
    typename Initiation, typename... Args>
void_or_deduced async_initiate(
    ASIO_MOVE_ARG(Initiation) initiation,
    ASIO_NONDEDUCED_MOVE_ARG(CompletionToken),
    ASIO_MOVE_ARG(Args)... args);

#elif defined(ASIO_HAS_VARIADIC_TEMPLATES)

template <typename CompletionToken,
    ASIO_COMPLETION_SIGNATURE... Signatures,
    typename Initiation, typename... Args>
inline typename constraint<
    detail::async_result_has_initiate_memfn<
      CompletionToken, Signatures...>::value,
    ASIO_INITFN_DEDUCED_RESULT_TYPE(CompletionToken, Signatures...,
      (async_result<typename decay<CompletionToken>::type,
        Signatures...>::initiate(declval<ASIO_MOVE_ARG(Initiation)>(),
          declval<ASIO_MOVE_ARG(CompletionToken)>(),
          declval<ASIO_MOVE_ARG(Args)>()...)))>::type
async_initiate(ASIO_MOVE_ARG(Initiation) initiation,
    ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token,
    ASIO_MOVE_ARG(Args)... args)
{
  return async_result<typename decay<CompletionToken>::type,
    Signatures...>::initiate(ASIO_MOVE_CAST(Initiation)(initiation),
      ASIO_MOVE_CAST(CompletionToken)(token),
      ASIO_MOVE_CAST(Args)(args)...);
}

template <typename CompletionToken,
    ASIO_COMPLETION_SIGNATURE... Signatures,
    typename Initiation, typename... Args>
inline typename constraint<
    !detail::async_result_has_initiate_memfn<
      CompletionToken, Signatures...>::value,
    ASIO_INITFN_RESULT_TYPE(CompletionToken, Signatures...)>::type
async_initiate(ASIO_MOVE_ARG(Initiation) initiation,
    ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token,
    ASIO_MOVE_ARG(Args)... args)
{
  async_completion<CompletionToken, Signatures...> completion(token);

  ASIO_MOVE_CAST(Initiation)(initiation)(
      ASIO_MOVE_CAST(ASIO_HANDLER_TYPE(CompletionToken,
        Signatures...))(completion.completion_handler),
      ASIO_MOVE_CAST(Args)(args)...);

  return completion.result.get();
}

#else // defined(ASIO_HAS_VARIADIC_TEMPLATES)

template <typename CompletionToken,
    ASIO_COMPLETION_SIGNATURE Sig0,
    typename Initiation>
inline typename constraint<
    detail::async_result_has_initiate_memfn<
      CompletionToken, Sig0>::value,
    ASIO_INITFN_DEDUCED_RESULT_TYPE(CompletionToken, Sig0,
      (async_result<typename decay<CompletionToken>::type,
        Sig0>::initiate(declval<ASIO_MOVE_ARG(Initiation)>(),
          declval<ASIO_MOVE_ARG(CompletionToken)>())))>::type
async_initiate(ASIO_MOVE_ARG(Initiation) initiation,
    ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token)
{
  return async_result<typename decay<CompletionToken>::type,
    Sig0>::initiate(ASIO_MOVE_CAST(Initiation)(initiation),
      ASIO_MOVE_CAST(CompletionToken)(token));
}

template <typename CompletionToken,
    ASIO_COMPLETION_SIGNATURE Sig0,
    ASIO_COMPLETION_SIGNATURE Sig1,
    typename Initiation>
inline typename constraint<
    detail::async_result_has_initiate_memfn<
      CompletionToken, Sig0, Sig1>::value,
    ASIO_INITFN_DEDUCED_RESULT_TYPE2(CompletionToken, Sig0, Sig1,
      (async_result<typename decay<CompletionToken>::type,
        Sig0, Sig1>::initiate(declval<ASIO_MOVE_ARG(Initiation)>(),
          declval<ASIO_MOVE_ARG(CompletionToken)>())))>::type
async_initiate(ASIO_MOVE_ARG(Initiation) initiation,
    ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token)
{
  return async_result<typename decay<CompletionToken>::type,
    Sig0, Sig1>::initiate(ASIO_MOVE_CAST(Initiation)(initiation),
      ASIO_MOVE_CAST(CompletionToken)(token));
}

template <typename CompletionToken,
    ASIO_COMPLETION_SIGNATURE Sig0,
    ASIO_COMPLETION_SIGNATURE Sig1,
    ASIO_COMPLETION_SIGNATURE Sig2,
    typename Initiation>
inline typename constraint<
    detail::async_result_has_initiate_memfn<
      CompletionToken, Sig0, Sig1, Sig2>::value,
    ASIO_INITFN_DEDUCED_RESULT_TYPE3(CompletionToken, Sig0, Sig1, Sig2,
      (async_result<typename decay<CompletionToken>::type,
        Sig0, Sig1, Sig2>::initiate(declval<ASIO_MOVE_ARG(Initiation)>(),
          declval<ASIO_MOVE_ARG(CompletionToken)>())))>::type
async_initiate(ASIO_MOVE_ARG(Initiation) initiation,
    ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token)
{
  return async_result<typename decay<CompletionToken>::type,
    Sig0, Sig1, Sig2>::initiate(ASIO_MOVE_CAST(Initiation)(initiation),
      ASIO_MOVE_CAST(CompletionToken)(token));
}

template <typename CompletionToken,
    ASIO_COMPLETION_SIGNATURE Sig0,
    typename Initiation>
inline typename constraint<
    !detail::async_result_has_initiate_memfn<
      CompletionToken, Sig0>::value,
    ASIO_INITFN_RESULT_TYPE(CompletionToken, Sig0)>::type
async_initiate(ASIO_MOVE_ARG(Initiation) initiation,
    ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token)
{
  async_completion<CompletionToken, Sig0> completion(token);

  ASIO_MOVE_CAST(Initiation)(initiation)(
      ASIO_MOVE_CAST(ASIO_HANDLER_TYPE(CompletionToken,
        Sig0))(completion.completion_handler));

  return completion.result.get();
}

template <typename CompletionToken,
    ASIO_COMPLETION_SIGNATURE Sig0,
    ASIO_COMPLETION_SIGNATURE Sig1,
    typename Initiation>
inline typename constraint<
    !detail::async_result_has_initiate_memfn<
      CompletionToken, Sig0, Sig1>::value,
    ASIO_INITFN_RESULT_TYPE2(CompletionToken, Sig0, Sig1)>::type
async_initiate(ASIO_MOVE_ARG(Initiation) initiation,
    ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token)
{
  async_completion<CompletionToken, Sig0, Sig1> completion(token);

  ASIO_MOVE_CAST(Initiation)(initiation)(
      ASIO_MOVE_CAST(ASIO_HANDLER_TYPE2(CompletionToken,
        Sig0, Sig1))(completion.completion_handler));

  return completion.result.get();
}

template <typename CompletionToken,
    ASIO_COMPLETION_SIGNATURE Sig0,
    ASIO_COMPLETION_SIGNATURE Sig1,
    ASIO_COMPLETION_SIGNATURE Sig2,
    typename Initiation>
inline typename constraint<
    !detail::async_result_has_initiate_memfn<
      CompletionToken, Sig0, Sig1, Sig2>::value,
    ASIO_INITFN_RESULT_TYPE3(CompletionToken, Sig0, Sig1, Sig2)>::type
async_initiate(ASIO_MOVE_ARG(Initiation) initiation,
    ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token)
{
  async_completion<CompletionToken, Sig0, Sig1, Sig2> completion(token);

  ASIO_MOVE_CAST(Initiation)(initiation)(
      ASIO_MOVE_CAST(ASIO_HANDLER_TYPE3(CompletionToken,
        Sig0, Sig1, Sig2))(completion.completion_handler));

  return completion.result.get();
}

#define ASIO_PRIVATE_INITIATE_DEF(n) \
  template <typename CompletionToken, \
      ASIO_COMPLETION_SIGNATURE Sig0, \
      typename Initiation, ASIO_VARIADIC_TPARAMS(n)> \
  inline typename constraint< \
      detail::async_result_has_initiate_memfn< \
        CompletionToken, Sig0>::value, \
      ASIO_INITFN_DEDUCED_RESULT_TYPE( \
        CompletionToken, Sig0, \
        (async_result<typename decay<CompletionToken>::type, \
          Sig0>::initiate( \
            declval<ASIO_MOVE_ARG(Initiation)>(), \
            declval<ASIO_MOVE_ARG(CompletionToken)>(), \
            ASIO_VARIADIC_MOVE_DECLVAL(n))))>::type \
  async_initiate(ASIO_MOVE_ARG(Initiation) initiation, \
      ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token, \
      ASIO_VARIADIC_MOVE_PARAMS(n)) \
  { \
    return async_result<typename decay<CompletionToken>::type, \
      Sig0>::initiate( \
        ASIO_MOVE_CAST(Initiation)(initiation), \
        ASIO_MOVE_CAST(CompletionToken)(token), \
        ASIO_VARIADIC_MOVE_ARGS(n)); \
  } \
  \
  template <typename CompletionToken, \
      ASIO_COMPLETION_SIGNATURE Sig0, \
      ASIO_COMPLETION_SIGNATURE Sig1, \
      typename Initiation, ASIO_VARIADIC_TPARAMS(n)> \
  inline typename constraint< \
      detail::async_result_has_initiate_memfn< \
        CompletionToken, Sig0, Sig1>::value, \
      ASIO_INITFN_DEDUCED_RESULT_TYPE2( \
        CompletionToken, Sig0, Sig1, \
        (async_result<typename decay<CompletionToken>::type, \
          Sig0, Sig1>::initiate( \
            declval<ASIO_MOVE_ARG(Initiation)>(), \
            declval<ASIO_MOVE_ARG(CompletionToken)>(), \
            ASIO_VARIADIC_MOVE_DECLVAL(n))))>::type \
  async_initiate(ASIO_MOVE_ARG(Initiation) initiation, \
      ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token, \
      ASIO_VARIADIC_MOVE_PARAMS(n)) \
  { \
    return async_result<typename decay<CompletionToken>::type, \
      Sig0, Sig1>::initiate( \
        ASIO_MOVE_CAST(Initiation)(initiation), \
        ASIO_MOVE_CAST(CompletionToken)(token), \
        ASIO_VARIADIC_MOVE_ARGS(n)); \
  } \
  \
  template <typename CompletionToken, \
      ASIO_COMPLETION_SIGNATURE Sig0, \
      ASIO_COMPLETION_SIGNATURE Sig1, \
      ASIO_COMPLETION_SIGNATURE Sig2, \
      typename Initiation, ASIO_VARIADIC_TPARAMS(n)> \
  inline typename constraint< \
      detail::async_result_has_initiate_memfn< \
        CompletionToken, Sig0, Sig1, Sig2>::value, \
      ASIO_INITFN_DEDUCED_RESULT_TYPE3( \
        CompletionToken, Sig0, Sig1, Sig2, \
        (async_result<typename decay<CompletionToken>::type, \
          Sig0, Sig1, Sig2>::initiate( \
            declval<ASIO_MOVE_ARG(Initiation)>(), \
            declval<ASIO_MOVE_ARG(CompletionToken)>(), \
            ASIO_VARIADIC_MOVE_DECLVAL(n))))>::type \
  async_initiate(ASIO_MOVE_ARG(Initiation) initiation, \
      ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token, \
      ASIO_VARIADIC_MOVE_PARAMS(n)) \
  { \
    return async_result<typename decay<CompletionToken>::type, \
      Sig0, Sig1, Sig2>::initiate( \
        ASIO_MOVE_CAST(Initiation)(initiation), \
        ASIO_MOVE_CAST(CompletionToken)(token), \
        ASIO_VARIADIC_MOVE_ARGS(n)); \
  } \
  \
  template <typename CompletionToken, \
      ASIO_COMPLETION_SIGNATURE Sig0, \
      typename Initiation, ASIO_VARIADIC_TPARAMS(n)> \
  inline typename constraint< \
      !detail::async_result_has_initiate_memfn< \
        CompletionToken, Sig0>::value, \
      ASIO_INITFN_RESULT_TYPE(CompletionToken, Sig0)>::type \
  async_initiate(ASIO_MOVE_ARG(Initiation) initiation, \
      ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token, \
      ASIO_VARIADIC_MOVE_PARAMS(n)) \
  { \
    async_completion<CompletionToken, \
      Sig0> completion(token); \
  \
    ASIO_MOVE_CAST(Initiation)(initiation)( \
        ASIO_MOVE_CAST(ASIO_HANDLER_TYPE(CompletionToken, \
          Sig0))(completion.completion_handler), \
        ASIO_VARIADIC_MOVE_ARGS(n)); \
  \
    return completion.result.get(); \
  } \
  \
  template <typename CompletionToken, \
      ASIO_COMPLETION_SIGNATURE Sig0, \
      ASIO_COMPLETION_SIGNATURE Sig1, \
      typename Initiation, ASIO_VARIADIC_TPARAMS(n)> \
  inline typename constraint< \
      !detail::async_result_has_initiate_memfn< \
        CompletionToken, Sig0, Sig1>::value, \
      ASIO_INITFN_RESULT_TYPE2(CompletionToken, Sig0, Sig1)>::type \
  async_initiate(ASIO_MOVE_ARG(Initiation) initiation, \
      ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token, \
      ASIO_VARIADIC_MOVE_PARAMS(n)) \
  { \
    async_completion<CompletionToken, \
      Sig0, Sig1> completion(token); \
  \
    ASIO_MOVE_CAST(Initiation)(initiation)( \
        ASIO_MOVE_CAST(ASIO_HANDLER_TYPE2(CompletionToken, \
          Sig0, Sig1))(completion.completion_handler), \
        ASIO_VARIADIC_MOVE_ARGS(n)); \
  \
    return completion.result.get(); \
  } \
  \
  template <typename CompletionToken, \
      ASIO_COMPLETION_SIGNATURE Sig0, \
      ASIO_COMPLETION_SIGNATURE Sig1, \
      ASIO_COMPLETION_SIGNATURE Sig2, \
      typename Initiation, ASIO_VARIADIC_TPARAMS(n)> \
  inline typename constraint< \
      !detail::async_result_has_initiate_memfn< \
        CompletionToken, Sig0, Sig1, Sig2>::value, \
      ASIO_INITFN_RESULT_TYPE3(CompletionToken, Sig0, Sig1, Sig2)>::type \
  async_initiate(ASIO_MOVE_ARG(Initiation) initiation, \
      ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token, \
      ASIO_VARIADIC_MOVE_PARAMS(n)) \
  { \
    async_completion<CompletionToken, \
      Sig0, Sig1, Sig2> completion(token); \
  \
    ASIO_MOVE_CAST(Initiation)(initiation)( \
        ASIO_MOVE_CAST(ASIO_HANDLER_TYPE3(CompletionToken, \
          Sig0, Sig1, Sig2))(completion.completion_handler), \
        ASIO_VARIADIC_MOVE_ARGS(n)); \
  \
    return completion.result.get(); \
  } \
  /**/
  ASIO_VARIADIC_GENERATE(ASIO_PRIVATE_INITIATE_DEF)
#undef ASIO_PRIVATE_INITIATE_DEF

#endif // defined(ASIO_HAS_VARIADIC_TEMPLATES)

#if defined(ASIO_HAS_CONCEPTS) \
  && defined(ASIO_HAS_VARIADIC_TEMPLATES) \
  && defined(ASIO_HAS_DECLTYPE)

namespace detail {

template <typename... Signatures>
struct initiation_archetype
{
  template <completion_handler_for<Signatures...> CompletionHandler>
  void operator()(CompletionHandler&&) const
  {
  }
};

} // namespace detail

template <typename T, typename... Signatures>
ASIO_CONCEPT completion_token_for =
  detail::are_completion_signatures<Signatures...>::value
  &&
  requires(T&& t)
  {
    async_initiate<T, Signatures...>(
        detail::initiation_archetype<Signatures...>{}, t);
  };

#define ASIO_COMPLETION_TOKEN_FOR(sig) \
  ::asio::completion_token_for<sig>
#define ASIO_COMPLETION_TOKEN_FOR2(sig0, sig1) \
  ::asio::completion_token_for<sig0, sig1>
#define ASIO_COMPLETION_TOKEN_FOR3(sig0, sig1, sig2) \
  ::asio::completion_token_for<sig0, sig1, sig2>

#else // defined(ASIO_HAS_CONCEPTS)
      //   && defined(ASIO_HAS_VARIADIC_TEMPLATES)
      //   && defined(ASIO_HAS_DECLTYPE)

#define ASIO_COMPLETION_TOKEN_FOR(sig) typename
#define ASIO_COMPLETION_TOKEN_FOR2(sig0, sig1) typename
#define ASIO_COMPLETION_TOKEN_FOR3(sig0, sig1, sig2) typename

#endif // defined(ASIO_HAS_CONCEPTS)
       //   && defined(ASIO_HAS_VARIADIC_TEMPLATES)
       //   && defined(ASIO_HAS_DECLTYPE)

namespace detail {

struct async_operation_probe {};
struct async_operation_probe_result {};

template <typename Call, typename = void>
struct is_async_operation_call : false_type
{
};

template <typename Call>
struct is_async_operation_call<Call,
    typename void_type<
      typename enable_if<
        is_same<
          typename result_of<Call>::type,
          async_operation_probe_result
        >::value
      >::type
    >::type> : true_type
{
};

} // namespace detail

#if !defined(GENERATING_DOCUMENTATION)
#if defined(ASIO_HAS_VARIADIC_TEMPLATES)

template <typename... Signatures>
class async_result<detail::async_operation_probe, Signatures...>
{
public:
  typedef detail::async_operation_probe_result return_type;

  template <typename Initiation, typename... InitArgs>
  static return_type initiate(ASIO_MOVE_ARG(Initiation),
      detail::async_operation_probe, ASIO_MOVE_ARG(InitArgs)...)
  {
    return return_type();
  }
};

#else // defined(ASIO_HAS_VARIADIC_TEMPLATES)

namespace detail {

struct async_result_probe_base
{
  typedef detail::async_operation_probe_result return_type;

  template <typename Initiation>
  static return_type initiate(ASIO_MOVE_ARG(Initiation),
      detail::async_operation_probe)
  {
    return return_type();
  }

#define ASIO_PRIVATE_INITIATE_DEF(n) \
  template <typename Initiation, ASIO_VARIADIC_TPARAMS(n)> \
  static return_type initiate(ASIO_MOVE_ARG(Initiation), \
      detail::async_operation_probe, \
      ASIO_VARIADIC_UNNAMED_MOVE_PARAMS(n)) \
  { \
    return return_type(); \
  } \
  /**/
  ASIO_VARIADIC_GENERATE(ASIO_PRIVATE_INITIATE_DEF)
#undef ASIO_PRIVATE_INITIATE_DEF
};

} // namespace detail

template <typename Sig0>
class async_result<detail::async_operation_probe, Sig0>
  : public detail::async_result_probe_base {};

template <typename Sig0, typename Sig1>
class async_result<detail::async_operation_probe, Sig0, Sig1>
  : public detail::async_result_probe_base {};

template <typename Sig0, typename Sig1, typename Sig2>
class async_result<detail::async_operation_probe, Sig0, Sig1, Sig2>
  : public detail::async_result_probe_base {};

#endif // defined(ASIO_HAS_VARIADIC_TEMPLATES)
#endif // !defined(GENERATING_DOCUMENTATION)

#if defined(GENERATING_DOCUMENTATION)

/// The is_async_operation trait detects whether a type @c T and arguments
/// @c Args... may be used to initiate an asynchronous operation.
/**
 * Class template @c is_async_operation is a trait is derived from @c true_type
 * if the expression <tt>T(Args..., token)</tt> initiates an asynchronous
 * operation, where @c token is an unspecified completion token type. Otherwise,
 * @c is_async_operation is derived from @c false_type.
 */
template <typename T, typename... Args>
struct is_async_operation : integral_constant<bool, automatically_determined>
{
};

#elif defined(ASIO_HAS_VARIADIC_TEMPLATES)

template <typename T, typename... Args>
struct is_async_operation :
  detail::is_async_operation_call<
    T(Args..., detail::async_operation_probe)>
{
};

#else // defined(ASIO_HAS_VARIADIC_TEMPLATES)

template <typename T, typename = void, typename = void, typename = void,
    typename = void, typename = void, typename = void, typename = void,
    typename = void, typename = void>
struct is_async_operation;

template <typename T>
struct is_async_operation<T> :
  detail::is_async_operation_call<
    T(detail::async_operation_probe)>
{
};

#define ASIO_PRIVATE_IS_ASYNC_OP_DEF(n) \
  template <typename T, ASIO_VARIADIC_TPARAMS(n)> \
  struct is_async_operation<T, ASIO_VARIADIC_TARGS(n)> : \
    detail::is_async_operation_call< \
      T(ASIO_VARIADIC_TARGS(n), detail::async_operation_probe)> \
  { \
  }; \
  /**/
  ASIO_VARIADIC_GENERATE(ASIO_PRIVATE_IS_ASYNC_OP_DEF)
#undef ASIO_PRIVATE_IS_ASYNC_OP_DEF

#endif // defined(ASIO_HAS_VARIADIC_TEMPLATES)

#if defined(ASIO_HAS_CONCEPTS) \
  && defined(ASIO_HAS_VARIADIC_TEMPLATES)

template <typename T, typename... Args>
ASIO_CONCEPT async_operation = is_async_operation<T, Args...>::value;

#define ASIO_ASYNC_OPERATION(t) \
  ::asio::async_operation<t>
#define ASIO_ASYNC_OPERATION1(t, a0) \
  ::asio::async_operation<t, a0>
#define ASIO_ASYNC_OPERATION2(t, a0, a1) \
  ::asio::async_operation<t, a0, a1>
#define ASIO_ASYNC_OPERATION3(t, a0, a1, a2) \
  ::asio::async_operation<t, a0, a1, a2>

#else // defined(ASIO_HAS_CONCEPTS)
      //   && defined(ASIO_HAS_VARIADIC_TEMPLATES)

#define ASIO_ASYNC_OPERATION(t) typename
#define ASIO_ASYNC_OPERATION1(t, a0) typename
#define ASIO_ASYNC_OPERATION2(t, a0, a1) typename
#define ASIO_ASYNC_OPERATION3(t, a0, a1, a2) typename

#endif // defined(ASIO_HAS_CONCEPTS)
       //   && defined(ASIO_HAS_VARIADIC_TEMPLATES)

namespace detail {

struct completion_signature_probe {};

#if defined(ASIO_HAS_VARIADIC_TEMPLATES)

template <typename... T>
struct completion_signature_probe_result
{
  template <template <typename...> class Op>
  struct apply
  {
    typedef Op<T...> type;
  };
};

template <typename T>
struct completion_signature_probe_result<T>
{
  typedef T type;

  template <template <typename...> class Op>
  struct apply
  {
    typedef Op<T> type;
  };
};

template <>
struct completion_signature_probe_result<void>
{
  template <template <typename...> class Op>
  struct apply
  {
    typedef Op<> type;
  };
};

#else // defined(ASIO_HAS_VARIADIC_TEMPLATES)

template <typename T>
struct completion_signature_probe_result
{
  typedef T type;
};

template <>
struct completion_signature_probe_result<void>
{
};

#endif // defined(ASIO_HAS_VARIADIC_TEMPLATES)

} // namespace detail

#if !defined(GENERATING_DOCUMENTATION)
#if defined(ASIO_HAS_VARIADIC_TEMPLATES)

template <typename... Signatures>
class async_result<detail::completion_signature_probe, Signatures...>
{
public:
  typedef detail::completion_signature_probe_result<Signatures...> return_type;

  template <typename Initiation, typename... InitArgs>
  static return_type initiate(ASIO_MOVE_ARG(Initiation),
      detail::completion_signature_probe, ASIO_MOVE_ARG(InitArgs)...)
  {
    return return_type();
  }
};

template <typename Signature>
class async_result<detail::completion_signature_probe, Signature>
{
public:
  typedef detail::completion_signature_probe_result<Signature> return_type;

  template <typename Initiation, typename... InitArgs>
  static return_type initiate(ASIO_MOVE_ARG(Initiation),
      detail::completion_signature_probe, ASIO_MOVE_ARG(InitArgs)...)
  {
    return return_type();
  }
};

#else // defined(ASIO_HAS_VARIADIC_TEMPLATES)

namespace detail {

template <typename Signature>
class async_result_sig_probe_base
{
public:
  typedef detail::completion_signature_probe_result<Signature> return_type;

  template <typename Initiation>
  static return_type initiate(ASIO_MOVE_ARG(Initiation),
      detail::async_operation_probe)
  {
    return return_type();
  }

#define ASIO_PRIVATE_INITIATE_DEF(n) \
  template <typename Initiation, ASIO_VARIADIC_TPARAMS(n)> \
  static return_type initiate(ASIO_MOVE_ARG(Initiation), \
      detail::completion_signature_probe, \
      ASIO_VARIADIC_UNNAMED_MOVE_PARAMS(n)) \
  { \
    return return_type(); \
  } \
  /**/
  ASIO_VARIADIC_GENERATE(ASIO_PRIVATE_INITIATE_DEF)
#undef ASIO_PRIVATE_INITIATE_DEF
};

} // namespace detail

template <>
class async_result<detail::completion_signature_probe>
  : public detail::async_result_sig_probe_base<void> {};

template <typename Sig0>
class async_result<detail::completion_signature_probe, Sig0>
  : public detail::async_result_sig_probe_base<Sig0> {};

template <typename Sig0, typename Sig1>
class async_result<detail::completion_signature_probe, Sig0, Sig1>
  : public detail::async_result_sig_probe_base<void> {};

template <typename Sig0, typename Sig1, typename Sig2>
class async_result<detail::completion_signature_probe, Sig0, Sig1, Sig2>
  : public detail::async_result_sig_probe_base<void> {};

#endif // defined(ASIO_HAS_VARIADIC_TEMPLATES)
#endif // !defined(GENERATING_DOCUMENTATION)

#if defined(GENERATING_DOCUMENTATION)

/// The completion_signature_of trait determines the completion signature
/// of an asynchronous operation.
/**
 * Class template @c completion_signature_of is a trait with a member type
 * alias @c type that denotes the completion signature of the asynchronous
 * operation initiated by the expression <tt>T(Args..., token)</tt> operation,
 * where @c token is an unspecified completion token type. If the asynchronous
 * operation does not have exactly one completion signature, the instantion of
 * the trait is well-formed but the member type alias @c type is omitted. If
 * the expression <tt>T(Args..., token)</tt> is not an asynchronous operation
 * then use of the trait is ill-formed.
 */
template <typename T, typename... Args>
struct completion_signature_of
{
  typedef automatically_determined type;
};

template <typename T, typename... Args>
using completion_signature_of_t =
  typename completion_signature_of<T, Args...>::type;

#elif defined(ASIO_HAS_VARIADIC_TEMPLATES)

template <typename T, typename... Args>
struct completion_signature_of :
  result_of<T(Args..., detail::completion_signature_probe)>::type
{
};

#if defined(ASIO_HAS_ALIAS_TEMPLATES)
template <typename T, typename... Args>
using completion_signature_of_t =
  typename completion_signature_of<T, Args...>::type;
#endif // defined(ASIO_HAS_ALIAS_TEMPLATES)

#else // defined(ASIO_HAS_VARIADIC_TEMPLATES)

template <typename T, typename = void, typename = void, typename = void,
    typename = void, typename = void, typename = void, typename = void,
    typename = void, typename = void>
struct completion_signature_of;

template <typename T>
struct completion_signature_of<T> :
  result_of<T(detail::completion_signature_probe)>::type
{
};

#define ASIO_PRIVATE_COMPLETION_SIG_OF_DEF(n) \
  template <typename T, ASIO_VARIADIC_TPARAMS(n)> \
  struct completion_signature_of<T, ASIO_VARIADIC_TARGS(n)> : \
    result_of< \
      T(ASIO_VARIADIC_TARGS(n), \
        detail::completion_signature_probe)>::type \
  { \
  }; \
  /**/
  ASIO_VARIADIC_GENERATE(ASIO_PRIVATE_COMPLETION_SIG_OF_DEF)
#undef ASIO_PRIVATE_COMPLETION_SIG_OF_DEF

#endif // defined(ASIO_HAS_VARIADIC_TEMPLATES)

namespace detail {

template <typename T, typename = void>
struct default_completion_token_impl
{
  typedef void type;
};

template <typename T>
struct default_completion_token_impl<T,
  typename void_type<typename T::default_completion_token_type>::type>
{
  typedef typename T::default_completion_token_type type;
};

} // namespace detail

#if defined(GENERATING_DOCUMENTATION)

/// Traits type used to determine the default completion token type associated
/// with a type (such as an executor).
/**
 * A program may specialise this traits type if the @c T template parameter in
 * the specialisation is a user-defined type.
 *
 * Specialisations of this trait may provide a nested typedef @c type, which is
 * a default-constructible completion token type.
 */
template <typename T>
struct default_completion_token
{
  /// If @c T has a nested type @c default_completion_token_type,
  /// <tt>T::default_completion_token_type</tt>. Otherwise the typedef @c type
  /// is not defined.
  typedef see_below type;
};
#else
template <typename T>
struct default_completion_token
  : detail::default_completion_token_impl<T>
{
};
#endif

#if defined(ASIO_HAS_ALIAS_TEMPLATES)

template <typename T>
using default_completion_token_t = typename default_completion_token<T>::type;

#endif // defined(ASIO_HAS_ALIAS_TEMPLATES)

#if defined(ASIO_HAS_DEFAULT_FUNCTION_TEMPLATE_ARGUMENTS)

#define ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(e) \
  = typename ::asio::default_completion_token<e>::type
#define ASIO_DEFAULT_COMPLETION_TOKEN(e) \
  = typename ::asio::default_completion_token<e>::type()

#else // defined(ASIO_HAS_DEFAULT_FUNCTION_TEMPLATE_ARGUMENTS)

#define ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(e)
#define ASIO_DEFAULT_COMPLETION_TOKEN(e)

#endif // defined(ASIO_HAS_DEFAULT_FUNCTION_TEMPLATE_ARGUMENTS)

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_ASYNC_RESULT_HPP
