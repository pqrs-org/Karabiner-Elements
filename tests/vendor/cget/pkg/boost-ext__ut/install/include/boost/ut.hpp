//
// Copyright (c) 2019-2021 Kris Jusiak (kris at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
#if defined(__cpp_modules) && !defined(BOOST_UT_DISABLE_MODULE)
export module boost.ut;
export import std;
#else
#pragma once
#endif

#if __has_include(<iso646.h>)
#include <iso646.h>  // and, or, not, ...
#endif

#if not defined(__cpp_rvalue_references)
#error "[Boost::ext].UT requires support for rvalue references";
#elif not defined(__cpp_decltype)
#error "[Boost::ext].UT requires support for decltype";
#elif not defined(__cpp_return_type_deduction)
#error "[Boost::ext].UT requires support for return type deduction";
#elif not defined(__cpp_deduction_guides)
#error "[Boost::ext].UT requires support for return deduction guides";
#elif not defined(__cpp_generic_lambdas)
#error "[Boost::ext].UT requires support for generic lambdas";
#elif not defined(__cpp_constexpr)
#error "[Boost::ext].UT requires support for constexpr";
#elif not defined(__cpp_alias_templates)
#error "[Boost::ext].UT requires support for alias templates";
#elif not defined(__cpp_variadic_templates)
#error "[Boost::ext].UT requires support for variadic templates";
#elif not defined(__cpp_fold_expressions)
#error "[Boost::ext].UT requires support for return fold expressions";
#elif not defined(__cpp_static_assert)
#error "[Boost::ext].UT requires support for static assert";
#else
#define BOOST_UT_VERSION 1'1'8

#if defined(__has_builtin) and defined(__GNUC__) and (__GNUC__ < 10) and \
    not defined(__clang__)
#undef __has_builtin
#endif

#if not defined(__has_builtin)
#if defined(__GNUC__) and (__GNUC__ >= 9)
#define __has___builtin_FILE 1
#define __has___builtin_LINE 1
#endif
#define __has_builtin(...) __has_##__VA_ARGS__
#endif

#include <array>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>
#if __has_include(<unistd.h>) and __has_include(<sys/wait.h>)
#include <sys/wait.h>
#include <unistd.h>
#endif
#if defined(__cpp_exceptions)
#include <exception>
#endif

#if defined(__cpp_lib_source_location)
#include <source_location>
#endif

#if defined(__cpp_modules) && !defined(BOOST_UT_DISABLE_MODULE)
export
#endif
    namespace boost::inline ext::ut::inline v1_1_8 {
namespace utility {
template <class>
class function;
template <class R, class... TArgs>
class function<R(TArgs...)> {
 public:
  constexpr function() = default;
  template <class T>
  constexpr /*explicit(false)*/ function(T data)
      : invoke_{invoke_impl<T>},
        destroy_{destroy_impl<T>},
        data_{new T{static_cast<T&&>(data)}} {}
  constexpr function(function&& other) noexcept
      : invoke_{static_cast<decltype(other.invoke_)&&>(other.invoke_)},
        destroy_{static_cast<decltype(other.destroy_)&&>(other.destroy_)},
        data_{static_cast<decltype(other.data_)&&>(other.data_)} {
    other.data_ = {};
  }
  constexpr function(const function&) = delete;
  ~function() { destroy_(data_); }

  constexpr function& operator=(const function&) = delete;
  constexpr function& operator=(function&&) = delete;
  [[nodiscard]] constexpr auto operator()(TArgs... args) -> R {
    return invoke_(data_, args...);
  }
  [[nodiscard]] constexpr auto operator()(TArgs... args) const -> R {
    return invoke_(data_, args...);
  }

 private:
  template <class T>
  [[nodiscard]] static auto invoke_impl(void* data, TArgs... args) -> R {
    return (*static_cast<T*>(data))(args...);
  }

  template <class T>
  static auto destroy_impl(void* data) -> void {
    delete static_cast<T*>(data);
  }

  R (*invoke_)(void*, TArgs...){};
  void (*destroy_)(void*){};
  void* data_{};
};

[[nodiscard]] inline auto is_match(std::string_view input,
                                   std::string_view pattern) -> bool {
  if (std::empty(pattern)) {
    return std::empty(input);
  }

  if (std::empty(input)) {
    return pattern[0] == '*' ? is_match(input, pattern.substr(1)) : false;
  }

  if (pattern[0] != '?' and pattern[0] != '*' and pattern[0] != input[0]) {
    return false;
  }

  if (pattern[0] == '*') {
    for (decltype(std::size(input)) i = 0u; i <= std::size(input); ++i) {
      if (is_match(input.substr(i), pattern.substr(1))) {
        return true;
      }
    }
    return false;
  }

  return is_match(input.substr(1), pattern.substr(1));
}

template <class TPattern, class TStr>
[[nodiscard]] constexpr auto match(const TPattern& pattern, const TStr& str)
    -> std::vector<TStr> {
  std::vector<TStr> groups{};
  auto pi = 0u;
  auto si = 0u;

  const auto matcher = [&](char b, char e, char c = 0) {
    const auto match = si;
    while (str[si] and str[si] != b and str[si] != c) {
      ++si;
    }
    groups.emplace_back(str.substr(match, si - match));
    while (pattern[pi] and pattern[pi] != e) {
      ++pi;
    }
    pi++;
  };

  while (pi < std::size(pattern) && si < std::size(str)) {
    if (pattern[pi] == '\'' and str[si] == '\'' and pattern[pi + 1] == '{') {
      ++si;
      matcher('\'', '}');
    } else if (pattern[pi] == '{') {
      matcher(' ', '}', ',');
    } else if (pattern[pi] != str[si]) {
      return {};
    }
    ++pi;
    ++si;
  }

  if (si < str.size() or pi < std::size(pattern)) {
    return {};
  }

  return groups;
}

template <class T = std::string_view, class TDelim>
[[nodiscard]] inline auto split(T input, TDelim delim) -> std::vector<T> {
  std::vector<T> output{};
  std::size_t first{};
  while (first < std::size(input)) {
    const auto second = input.find_first_of(delim, first);
    if (first != second) {
      output.emplace_back(input.substr(first, second - first));
    }
    if (second == T::npos) {
      break;
    }
    first = second + 1;
  }
  return output;
}
}  // namespace utility

namespace reflection {
#if defined(__cpp_lib_source_location)
using source_location = std::source_location;
#else
class source_location {
 public:
  [[nodiscard]] static constexpr auto current(
#if (__has_builtin(__builtin_FILE) and __has_builtin(__builtin_LINE))
      const char* file = __builtin_FILE(), int line = __builtin_LINE()
#else
      const char* file = "unknown", int line = {}
#endif
          ) noexcept {
    source_location sl{};
    sl.file_ = file;
    sl.line_ = line;
    return sl;
  }
  [[nodiscard]] constexpr auto file_name() const noexcept { return file_; }
  [[nodiscard]] constexpr auto line() const noexcept { return line_; }

 private:
  const char* file_{"unknown"};
  int line_{};
};
#endif

template <class T>
[[nodiscard]] constexpr auto type_name() -> std::string_view {
#if defined(_MSC_VER) and not defined(__clang__)
  return {&__FUNCSIG__[120], sizeof(__FUNCSIG__) - 128};
#elif defined(__clang_analyzer__)
  return {&__PRETTY_FUNCTION__[57], sizeof(__PRETTY_FUNCTION__) - 59};
#elif defined(__clang__) and (__clang_major__ >= 13) and defined(__APPLE__)
  return {&__PRETTY_FUNCTION__[57], sizeof(__PRETTY_FUNCTION__) - 59};
#elif defined(__clang__) and (__clang_major__ >= 12) and not defined(__APPLE__)
  return {&__PRETTY_FUNCTION__[57], sizeof(__PRETTY_FUNCTION__) - 59};
#elif defined(__clang__)
  return {&__PRETTY_FUNCTION__[70], sizeof(__PRETTY_FUNCTION__) - 72};
#elif defined(__GNUC__)
  return {&__PRETTY_FUNCTION__[85], sizeof(__PRETTY_FUNCTION__) - 136};
#endif
}
}  // namespace reflection

namespace math {
template <class T>
[[nodiscard]] constexpr auto abs(const T t) -> T {
  return t < T{} ? -t : t;
}

template <class T>
[[nodiscard]] constexpr auto min_value(const T& lhs, const T& rhs) -> const T& {
  return (rhs < lhs) ? rhs : lhs;
}

template <class T, class TExp>
[[nodiscard]] constexpr auto pow(const T base, const TExp exp) -> T {
  return exp ? T(base * pow(base, exp - TExp(1))) : T(1);
}

template <class T, char... Cs>
[[nodiscard]] constexpr auto num() -> T {
  static_assert(
      ((Cs == '.' or Cs == '\'' or (Cs >= '0' and Cs <= '9')) and ...));
  T result{};
  for (const char c : {Cs...}) {
    if (c == '.') {
      break;
    }
    if (c >= '0' and c <= '9') {
      result = result * T(10) + T(c - '0');
    }
  }
  return result;
}

template <class T, char... Cs>
[[nodiscard]] constexpr auto den() -> T {
  constexpr const std::array cs{Cs...};
  T result{};
  auto i = 0u;
  while (cs[i++] != '.') {
  }

  for (auto j = i; j < sizeof...(Cs); ++j) {
    result += pow(T(10), sizeof...(Cs) - j) * T(cs[j] - '0');
  }
  return result;
}

template <class T, char... Cs>
[[nodiscard]] constexpr auto den_size() -> T {
  constexpr const std::array cs{Cs...};
  T i{};
  while (cs[i++] != '.') {
  }

  return T(sizeof...(Cs)) - i + T(1);
}

template <class T, class TValue>
[[nodiscard]] constexpr auto den_size(TValue value) -> T {
  constexpr auto precision = TValue(1e-7);
  T result{};
  TValue tmp{};
  do {
    value *= 10;
    tmp = value - T(value);
    ++result;
  } while (tmp > precision);

  return result;
}

}  // namespace math

namespace type_traits {
template <class...>
struct list {};

template <class T, class...>
struct identity {
  using type = T;
};

template <class T>
struct function_traits : function_traits<decltype(&T::operator())> {};

template <class R, class... TArgs>
struct function_traits<R (*)(TArgs...)> {
  using result_type = R;
  using args = list<TArgs...>;
};

template <class R, class... TArgs>
struct function_traits<R(TArgs...)> {
  using result_type = R;
  using args = list<TArgs...>;
};

template <class R, class T, class... TArgs>
struct function_traits<R (T::*)(TArgs...)> {
  using result_type = R;
  using args = list<TArgs...>;
};

template <class R, class T, class... TArgs>
struct function_traits<R (T::*)(TArgs...) const> {
  using result_type = R;
  using args = list<TArgs...>;
};

template <class T>
T&& declval();
template <class... Ts, class TExpr>
constexpr auto is_valid(TExpr expr)
    -> decltype(expr(declval<Ts...>()), bool()) {
  return true;
}
template <class...>
constexpr auto is_valid(...) -> bool {
  return false;
}

template <class T>
static constexpr auto is_container_v =
    is_valid<T>([](auto t) -> decltype(t.begin(), t.end(), void()) {});

template <class T>
static constexpr auto has_npos_v =
    is_valid<T>([](auto t) -> decltype(void(t.npos)) {});

template <class T>
static constexpr auto has_value_v =
    is_valid<T>([](auto t) -> decltype(void(t.value)) {});

template <class T>
static constexpr auto has_epsilon_v =
    is_valid<T>([](auto t) -> decltype(void(t.epsilon)) {});

template <class T>
inline constexpr auto is_floating_point_v = false;
template <>
inline constexpr auto is_floating_point_v<float> = true;
template <>
inline constexpr auto is_floating_point_v<double> = true;
template <>
inline constexpr auto is_floating_point_v<long double> = true;

#if defined(__clang__) or defined(_MSC_VER)
template <class From, class To>
static constexpr auto is_convertible_v = __is_convertible_to(From, To);
#else
template <class From, class To>
constexpr auto is_convertible(int) -> decltype(bool(To(declval<From>()))) {
  return true;
}
template <class...>
constexpr auto is_convertible(...) {
  return false;
}
template <class From, class To>
constexpr auto is_convertible_v = is_convertible<From, To>(0);
#endif

template <bool>
struct requires_ {};
template <>
struct requires_<true> {
  using type = int;
};

template <bool Cond>
using requires_t = typename requires_<Cond>::type;
}  // namespace type_traits

struct none {};

namespace events {
struct test_begin {
  std::string_view type{};
  std::string_view name{};
  reflection::source_location location{};
};
template <class Test, class TArg = none>
struct test {
  std::string_view type{};
  std::string_view name{};
  std::vector<std::string_view> tag{};
  reflection::source_location location{};
  TArg arg{};
  Test run{};

  constexpr auto operator()() { run_impl(static_cast<Test&&>(run), arg); }
  constexpr auto operator()() const { run_impl(static_cast<Test&&>(run), arg); }

 private:
  static constexpr auto run_impl(Test test, const none&) { test(); }

  template <class T>
  static constexpr auto run_impl(T test, const TArg& arg)
      -> decltype(test(arg), void()) {
    test(arg);
  }

  template <class T>
  static constexpr auto run_impl(T test, const TArg&)
      -> decltype(test.template operator()<TArg>(), void()) {
    test.template operator()<TArg>();
  }
};
template <class Test, class TArg>
test(std::string_view, std::string_view, std::string_view,
     reflection::source_location, TArg, Test) -> test<Test, TArg>;
template <class TSuite>
struct suite {
  TSuite run{};
  constexpr auto operator()() { run(); }
  constexpr auto operator()() const { run(); }
};
template <class TSuite>
suite(TSuite) -> suite<TSuite>;
struct test_run {
  std::string_view type{};
  std::string_view name{};
};
template <class TArg = none>
struct skip {
  std::string_view type{};
  std::string_view name{};
  TArg arg{};
};
template <class TArg>
skip(std::string_view, std::string_view, TArg) -> skip<TArg>;
struct test_skip {
  std::string_view type{};
  std::string_view name{};
};
template <class TExpr>
struct assertion {
  TExpr expr{};
  reflection::source_location location{};
};
template <class TExpr>
assertion(TExpr, reflection::source_location) -> assertion<TExpr>;
template <class TExpr>
struct assertion_pass {
  TExpr expr{};
  reflection::source_location location{};
};
template <class TExpr>
assertion_pass(TExpr) -> assertion_pass<TExpr>;
template <class TExpr>
struct assertion_fail {
  TExpr expr{};
  reflection::source_location location{};
};
template <class TExpr>
assertion_fail(TExpr) -> assertion_fail<TExpr>;
struct test_end {
  std::string_view type{};
  std::string_view name{};
};
template <class TMsg>
struct log {
  TMsg msg{};
};
template <class TMsg = std::string_view>
log(TMsg) -> log<TMsg>;
struct fatal_assertion {};
struct exception {
  const char* msg{};
  [[nodiscard]] auto what() const -> const char* { return msg; }
};
struct summary {};
}  // namespace events

namespace detail {
struct op {};
struct fatal {};
struct cfg {
  static inline reflection::source_location location{};
  static inline bool wip{};
};

template <class T>
[[nodiscard]] constexpr auto get_impl(const T& t, int) -> decltype(t.get()) {
  return t.get();
}
template <class T>
[[nodiscard]] constexpr auto get_impl(const T& t, ...) -> decltype(auto) {
  return t;
}
template <class T>
[[nodiscard]] constexpr auto get(const T& t) {
  return get_impl(t, 0);
}

template <class T>
struct type_ : op {
  template <class TOther>
  [[nodiscard]] constexpr auto operator()(const TOther&) const
      -> const type_<TOther> {
    return {};
  }
  [[nodiscard]] constexpr auto operator==(type_<T>) -> bool { return true; }
  template <class TOther>
  [[nodiscard]] constexpr auto operator==(type_<TOther>) -> bool {
    return false;
  }
  template <class TOther>
  [[nodiscard]] constexpr auto operator==(const TOther&) -> bool {
    return std::is_same_v<TOther, T>;
  }
  [[nodiscard]] constexpr auto operator!=(type_<T>) -> bool { return true; }
  template <class TOther>
  [[nodiscard]] constexpr auto operator!=(type_<TOther>) -> bool {
    return true;
  }
  template <class TOther>
  [[nodiscard]] constexpr auto operator!=(const TOther&) -> bool {
    return not std::is_same_v<TOther, T>;
  }
};

template <class T, class = int>
struct value : op {
  using value_type = T;

  constexpr /*explicit(false)*/ value(const T& _value) : value_{_value} {}
  [[nodiscard]] constexpr explicit operator T() const { return value_; }
  [[nodiscard]] constexpr decltype(auto) get() const { return value_; }

  T value_{};
};

template <class T>
struct value<T, type_traits::requires_t<type_traits::is_floating_point_v<T>>>
    : op {
  using value_type = T;
  static inline auto epsilon = T{};

  constexpr value(const T& _value, const T precision) : value_{_value} {
    epsilon = precision;
  }

  constexpr /*explicit(false)*/ value(const T& val)
      : value{val, T(1) / math::pow(T(10),
                                    math::den_size<unsigned long long>(val))} {}
  [[nodiscard]] constexpr explicit operator T() const { return value_; }
  [[nodiscard]] constexpr decltype(auto) get() const { return value_; }

  T value_{};
};

template <class T>
class value_location : public detail::value<T> {
 public:
  constexpr /*explicit(false)*/ value_location(
      const T& t, const reflection::source_location& sl =
                      reflection::source_location::current())
      : detail::value<T>{t} {
    cfg::location = sl;
  }

  constexpr value_location(const T& t, const T precision,
                           const reflection::source_location& sl =
                               reflection::source_location::current())
      : detail::value<T>{t, precision} {
    cfg::location = sl;
  }
};

template <auto N>
struct integral_constant : op {
  using value_type = decltype(N);
  static constexpr auto value = N;

  [[nodiscard]] constexpr auto operator-() const {
    return integral_constant<-N>{};
  }
  [[nodiscard]] constexpr explicit operator value_type() const { return N; }
  [[nodiscard]] constexpr auto get() const { return N; }
};

template <class T, auto N, auto D, auto Size, auto P = 1>
struct floating_point_constant : op {
  using value_type = T;

  static constexpr auto epsilon = T(1) / math::pow(T(10), Size - 1);
  static constexpr auto value = T(P) * (T(N) + (T(D) / math::pow(T(10), Size)));

  [[nodiscard]] constexpr auto operator-() const {
    return floating_point_constant<T, N, D, Size, -1>{};
  }
  [[nodiscard]] constexpr explicit operator value_type() const { return value; }
  [[nodiscard]] constexpr auto get() const { return value; }
};

template <class TLhs, class TRhs>
struct eq_ : op {
  constexpr eq_(const TLhs& lhs = {}, const TRhs& rhs = {})
      : lhs_{lhs}, rhs_{rhs}, value_{[&] {
          using std::operator==;
          using std::operator<;

          if constexpr (type_traits::has_value_v<TLhs> and
                        type_traits::has_value_v<TRhs>) {
            return TLhs::value == TRhs::value;
          } else if constexpr (type_traits::has_epsilon_v<TLhs> and
                               type_traits::has_epsilon_v<TRhs>) {
            return math::abs(get(lhs) - get(rhs)) <
                   math::min_value(TLhs::epsilon, TRhs::epsilon);
          } else if constexpr (type_traits::has_epsilon_v<TLhs>) {
            return math::abs(get(lhs) - get(rhs)) < TLhs::epsilon;
          } else if constexpr (type_traits::has_epsilon_v<TRhs>) {
            return math::abs(get(lhs) - get(rhs)) < TRhs::epsilon;
          } else {
            return get(lhs) == get(rhs);
          }
        }()} {}

  [[nodiscard]] constexpr operator bool() const { return value_; }
  [[nodiscard]] constexpr auto lhs() const { return get(lhs_); }
  [[nodiscard]] constexpr auto rhs() const { return get(rhs_); }

  const TLhs lhs_{};
  const TRhs rhs_{};
  const bool value_{};
};

template <class TLhs, class TRhs>
struct neq_ : op {
  constexpr neq_(const TLhs& lhs = {}, const TRhs& rhs = {})
      : lhs_{lhs}, rhs_{rhs}, value_{[&] {
          using std::operator==;
          using std::operator!=;
          using std::operator>;

          if constexpr (type_traits::has_value_v<TLhs> and
                        type_traits::has_value_v<TRhs>) {
            return TLhs::value != TRhs::value;
          } else if constexpr (type_traits::has_epsilon_v<TLhs> and
                               type_traits::has_epsilon_v<TRhs>) {
            return math::abs(get(lhs_) - get(rhs_)) >
                   math::min_value(TLhs::epsilon, TRhs::epsilon);
          } else if constexpr (type_traits::has_epsilon_v<TLhs>) {
            return math::abs(get(lhs_) - get(rhs_)) > TLhs::epsilon;
          } else if constexpr (type_traits::has_epsilon_v<TRhs>) {
            return math::abs(get(lhs_) - get(rhs_)) > TRhs::epsilon;
          } else {
            return get(lhs_) != get(rhs_);
          }
        }()} {}

  [[nodiscard]] constexpr operator bool() const { return value_; }
  [[nodiscard]] constexpr auto lhs() const { return get(lhs_); }
  [[nodiscard]] constexpr auto rhs() const { return get(rhs_); }

  const TLhs lhs_{};
  const TRhs rhs_{};
  const bool value_{};
};

template <class TLhs, class TRhs>
struct gt_ : op {
  constexpr gt_(const TLhs& lhs = {}, const TRhs& rhs = {})
      : lhs_{lhs}, rhs_{rhs}, value_{[&] {
          using std::operator>;

          if constexpr (type_traits::has_value_v<TLhs> and
                        type_traits::has_value_v<TRhs>) {
            return TLhs::value > TRhs::value;
          } else {
            return get(lhs_) > get(rhs_);
          }
        }()} {}

  [[nodiscard]] constexpr operator bool() const { return value_; }
  [[nodiscard]] constexpr auto lhs() const { return get(lhs_); }
  [[nodiscard]] constexpr auto rhs() const { return get(rhs_); }

  const TLhs lhs_{};
  const TRhs rhs_{};
  const bool value_{};
};

template <class TLhs, class TRhs>
struct ge_ : op {
  constexpr ge_(const TLhs& lhs = {}, const TRhs& rhs = {})
      : lhs_{lhs}, rhs_{rhs}, value_{[&] {
          using std::operator>=;

          if constexpr (type_traits::has_value_v<TLhs> and
                        type_traits::has_value_v<TRhs>) {
            return TLhs::value >= TRhs::value;
          } else {
            return get(lhs_) >= get(rhs_);
          }
        }()} {}

  [[nodiscard]] constexpr operator bool() const { return value_; }
  [[nodiscard]] constexpr auto lhs() const { return get(lhs_); }
  [[nodiscard]] constexpr auto rhs() const { return get(rhs_); }

  const TLhs lhs_{};
  const TRhs rhs_{};
  const bool value_{};
};

template <class TLhs, class TRhs>
struct lt_ : op {
  constexpr lt_(const TLhs& lhs = {}, const TRhs& rhs = {})
      : lhs_{lhs}, rhs_{rhs}, value_{[&] {
          using std::operator<;

          if constexpr (type_traits::has_value_v<TLhs> and
                        type_traits::has_value_v<TRhs>) {
            return TLhs::value < TRhs::value;
          } else {
            return get(lhs_) < get(rhs_);
          }
        }()} {}

  [[nodiscard]] constexpr operator bool() const { return value_; }
  [[nodiscard]] constexpr auto lhs() const { return get(lhs_); }
  [[nodiscard]] constexpr auto rhs() const { return get(rhs_); }

 private:
  const TLhs lhs_{};
  const TRhs rhs_{};
  const bool value_{};
};

template <class TLhs, class TRhs>
struct le_ : op {
  constexpr le_(const TLhs& lhs = {}, const TRhs& rhs = {})
      : lhs_{lhs}, rhs_{rhs}, value_{[&] {
          using std::operator<=;

          if constexpr (type_traits::has_value_v<TLhs> and
                        type_traits::has_value_v<TRhs>) {
            return TLhs::value <= TRhs::value;
          } else {
            return get(lhs_) <= get(rhs_);
          }
        }()} {}

  [[nodiscard]] constexpr operator bool() const { return value_; }
  [[nodiscard]] constexpr auto lhs() const { return get(lhs_); }
  [[nodiscard]] constexpr auto rhs() const { return get(rhs_); }

  const TLhs lhs_{};
  const TRhs rhs_{};
  const bool value_{};
};

template <class TLhs, class TRhs>
struct and_ : op {
  constexpr and_(const TLhs& lhs = {}, const TRhs& rhs = {})
      : lhs_{lhs},
        rhs_{rhs},
        value_{static_cast<bool>(lhs) and static_cast<bool>(rhs)} {}

  [[nodiscard]] constexpr operator bool() const { return value_; }
  [[nodiscard]] constexpr auto lhs() const { return get(lhs_); }
  [[nodiscard]] constexpr auto rhs() const { return get(rhs_); }

  const TLhs lhs_{};
  const TRhs rhs_{};
  const bool value_{};
};

template <class TLhs, class TRhs>
struct or_ : op {
  constexpr or_(const TLhs& lhs = {}, const TRhs& rhs = {})
      : lhs_{lhs},
        rhs_{rhs},
        value_{static_cast<bool>(lhs) or static_cast<bool>(rhs)} {}

  [[nodiscard]] constexpr operator bool() const { return value_; }
  [[nodiscard]] constexpr auto lhs() const { return get(lhs_); }
  [[nodiscard]] constexpr auto rhs() const { return get(rhs_); }

  const TLhs lhs_{};
  const TRhs rhs_{};
  const bool value_{};
};

template <class T>
struct not_ : op {
  explicit constexpr not_(const T& t = {})
      : t_{t}, value_{not static_cast<bool>(t)} {}

  [[nodiscard]] constexpr operator bool() const { return value_; }
  [[nodiscard]] constexpr auto value() const { return get(t_); }

  const T t_{};
  const bool value_{};
};

template <class>
struct fatal_;

#if defined(__cpp_exceptions)
template <class TExpr, class TException = void>
struct throws_ : op {
  constexpr explicit throws_(const TExpr& expr)
      : value_{[&expr] {
          try {
            expr();
          } catch (const TException&) {
            return true;
          } catch (...) {
            return false;
          }
          return false;
        }()} {}

  [[nodiscard]] constexpr operator bool() const { return value_; }

  const bool value_{};
};

template <class TExpr>
struct throws_<TExpr, void> : op {
  constexpr explicit throws_(const TExpr& expr)
      : value_{[&expr] {
          try {
            expr();
          } catch (...) {
            return true;
          }
          return false;
        }()} {}

  [[nodiscard]] constexpr operator bool() const { return value_; }

  const bool value_{};
};

template <class TExpr>
struct nothrow_ : op {
  constexpr explicit nothrow_(const TExpr& expr)
      : value_{[&expr] {
          try {
            expr();
          } catch (...) {
            return false;
          }
          return true;
        }()} {}

  [[nodiscard]] constexpr operator bool() const { return value_; }

  const bool value_{};
};
#endif

#if __has_include(<unistd.h>) and __has_include(<sys/wait.h>)
template <class TExpr>
struct aborts_ : op {
  constexpr explicit aborts_(const TExpr& expr)
      : value_{[&expr]() -> bool {
          if (const auto pid = fork(); not pid) {
            expr();
            exit(0);
          }
          auto exit_status = 0;
          wait(&exit_status);
          return exit_status;
        }()} {}

  [[nodiscard]] constexpr operator bool() const { return value_; }

  const bool value_{};
};
#endif
}  // namespace detail

namespace type_traits {
template <class T>
inline constexpr auto is_op_v = __is_base_of(detail::op, T);
}  // namespace type_traits

struct colors {
  std::string_view none = "\033[0m";
  std::string_view pass = "\033[32m";
  std::string_view fail = "\033[31m";
};

class printer {
  [[nodiscard]] inline auto color(const bool cond) {
    return cond ? colors_.pass : colors_.fail;
  }

 public:
  printer() = default;
  /*explicit(false)*/ printer(const colors colors) : colors_{colors} {}

  template <class T>
  auto& operator<<(const T& t) {
    out_ << detail::get(t);
    return *this;
  }

  template <class T,
            type_traits::requires_t<type_traits::is_container_v<T> and
                                    not type_traits::has_npos_v<T>> = 0>
  auto& operator<<(T&& t) {
    *this << '{';
    auto first = true;
    for (const auto& arg : t) {
      *this << (first ? "" : ", ") << arg;
      first = false;
    }
    *this << '}';
    return *this;
  }

  auto& operator<<(std::string_view sv) {
    out_ << sv;
    return *this;
  }

  template <class TLhs, class TRhs>
  auto& operator<<(const detail::eq_<TLhs, TRhs>& op) {
    return (*this << color(op) << op.lhs() << " == " << op.rhs()
                  << colors_.none);
  }

  template <class TLhs, class TRhs>
  auto& operator<<(const detail::neq_<TLhs, TRhs>& op) {
    return (*this << color(op) << op.lhs() << " != " << op.rhs()
                  << colors_.none);
  }

  template <class TLhs, class TRhs>
  auto& operator<<(const detail::gt_<TLhs, TRhs>& op) {
    return (*this << color(op) << op.lhs() << " > " << op.rhs()
                  << colors_.none);
  }

  template <class TLhs, class TRhs>
  auto& operator<<(const detail::ge_<TLhs, TRhs>& op) {
    return (*this << color(op) << op.lhs() << " >= " << op.rhs()
                  << colors_.none);
  }

  template <class TLhs, class TRhs>
  auto& operator<<(const detail::lt_<TRhs, TLhs>& op) {
    return (*this << color(op) << op.lhs() << " < " << op.rhs()
                  << colors_.none);
  }

  template <class TLhs, class TRhs>
  auto& operator<<(const detail::le_<TRhs, TLhs>& op) {
    return (*this << color(op) << op.lhs() << " <= " << op.rhs()
                  << colors_.none);
  }

  template <class TLhs, class TRhs>
  auto& operator<<(const detail::and_<TLhs, TRhs>& op) {
    return (*this << '(' << op.lhs() << color(op) << " and " << colors_.none
                  << op.rhs() << ')');
  }

  template <class TLhs, class TRhs>
  auto& operator<<(const detail::or_<TLhs, TRhs>& op) {
    return (*this << '(' << op.lhs() << color(op) << " or " << colors_.none
                  << op.rhs() << ')');
  }

  template <class T>
  auto& operator<<(const detail::not_<T>& op) {
    return (*this << color(op) << "not " << op.value() << colors_.none);
  }

  template <class T>
  auto& operator<<(const detail::fatal_<T>& fatal) {
    return (*this << fatal.get());
  }

#if defined(__cpp_exceptions)
  template <class TExpr, class TException>
  auto& operator<<(const detail::throws_<TExpr, TException>& op) {
    return (*this << color(op) << "throws<"
                  << reflection::type_name<TException>() << ">"
                  << colors_.none);
  }

  template <class TExpr>
  auto& operator<<(const detail::throws_<TExpr, void>& op) {
    return (*this << color(op) << "throws" << colors_.none);
  }

  template <class TExpr>
  auto& operator<<(const detail::nothrow_<TExpr>& op) {
    return (*this << color(op) << "nothrow" << colors_.none);
  }
#endif

#if __has_include(<unistd.h>) and __has_include(<sys/wait.h>)
  template <class TExpr>
  auto& operator<<(const detail::aborts_<TExpr>& op) {
    return (*this << color(op) << "aborts" << colors_.none);
  }
#endif

  template <class T>
  auto& operator<<(const detail::type_<T>&) {
    return (*this << reflection::type_name<T>());
  }

  auto str() const { return out_.str(); }
  const auto& colors() const { return colors_; }

 private:
  ut::colors colors_{};
  std::stringstream out_{};
};

template <class TPrinter = printer>
class reporter {
 public:
  constexpr auto operator=(TPrinter printer) {
    printer_ = static_cast<TPrinter&&>(printer);
  }

  auto on(events::test_begin test_begin) -> void {
    printer_ << "Running \"" << test_begin.name << "\"...";
    fails_ = asserts_.fail;
  }

  auto on(events::test_run test_run) -> void {
    printer_ << "\n \"" << test_run.name << "\"...";
  }

  auto on(events::test_skip test_skip) -> void {
    printer_ << test_skip.name << "...SKIPPED\n";
    ++tests_.skip;
  }

  auto on(events::test_end) -> void {
    if (asserts_.fail > fails_) {
      ++tests_.fail;
      printer_ << '\n'
               << printer_.colors().fail << "FAILED" << printer_.colors().none
               << '\n';
    } else {
      ++tests_.pass;
      printer_ << printer_.colors().pass << "PASSED" << printer_.colors().none
               << '\n';
    }
  }

  template <class TMsg>
  auto on(events::log<TMsg> l) -> void {
    printer_ << l.msg;
  }

  auto on(events::exception exception) -> void {
    printer_ << "\n  " << printer_.colors().fail
             << "Unexpected exception with message:\n"
             << exception.what() << printer_.colors().none;
    ++asserts_.fail;
  }

  template <class TExpr>
  auto on(events::assertion_pass<TExpr>) -> void {
    ++asserts_.pass;
  }

  template <class TExpr>
  auto on(events::assertion_fail<TExpr> assertion) -> void {
    constexpr auto short_name = [](std::string_view name) {
      return name.rfind('/') != std::string_view::npos
                 ? name.substr(name.rfind('/') + 1)
                 : name;
    };
    printer_ << "\n  " << short_name(assertion.location.file_name()) << ':'
             << assertion.location.line() << ':' << printer_.colors().fail
             << "FAILED" << printer_.colors().none << " [" << std::boolalpha
             << assertion.expr << printer_.colors().none << ']';
    ++asserts_.fail;
  }

  auto on(events::fatal_assertion) -> void {}

  auto on(events::summary) -> void {
    if (static auto once = true; once) {
      once = false;
      if (tests_.fail or asserts_.fail) {
        printer_ << "\n========================================================"
                    "=======================\n"
                 << "tests:   " << (tests_.pass + tests_.fail) << " | "
                 << printer_.colors().fail << tests_.fail << " failed"
                 << printer_.colors().none << '\n'
                 << "asserts: " << (asserts_.pass + asserts_.fail) << " | "
                 << asserts_.pass << " passed"
                 << " | " << printer_.colors().fail << asserts_.fail
                 << " failed" << printer_.colors().none << '\n';
        std::cerr << printer_.str() << std::endl;
      } else {
        std::cout << printer_.colors().pass << "All tests passed"
                  << printer_.colors().none << " (" << asserts_.pass
                  << " asserts in " << tests_.pass << " tests)\n";

        if (tests_.skip) {
          std::cout << tests_.skip << " tests skipped\n";
        }

        std::cout.flush();
      }
    }
  }

 protected:
  struct {
    std::size_t pass{};
    std::size_t fail{};
    std::size_t skip{};
  } tests_{};

  struct {
    std::size_t pass{};
    std::size_t fail{};
  } asserts_{};

  std::size_t fails_{};

  TPrinter printer_{};
};

struct options {
  std::string_view filter{};
  std::vector<std::string_view> tag{};
  ut::colors colors{};
  bool dry_run{};
};

struct run_cfg {
  bool report_errors{false};
};

template <class TReporter = reporter<printer>, auto MaxPathSize = 16>
class runner {
  class filter {
    static constexpr auto delim = ".";

   public:
    constexpr /*explicit(false)*/ filter(std::string_view _filter = {})
        : path_{utility::split(_filter, delim)} {}

    template <class TPath>
    constexpr auto operator()(const std::size_t level, const TPath& path) const
        -> bool {
      for (auto i = 0u; i < math::min_value(level + 1, std::size(path_)); ++i) {
        if (not utility::is_match(path[i], path_[i])) {
          return false;
        }
      }
      return true;
    }

   private:
    std::vector<std::string_view> path_{};
  };

 public:
  constexpr runner() = default;
  constexpr runner(TReporter reporter, std::size_t suites_size)
      : reporter_{std::move(reporter)}, suites_(suites_size) {}

  ~runner() {
    const auto should_run = not run_;

    if (should_run) {
      static_cast<void>(run());
    }

    if (not dry_run_) {
      reporter_.on(events::summary{});
    }

    if (should_run and fails_) {
      std::exit(-1);
    }
  }

  auto operator=(const options& options) {
    filter_ = options.filter;
    tag_ = options.tag;
    dry_run_ = options.dry_run;
    reporter_ = {options.colors};
  }

  template <class TSuite>
  auto on(events::suite<TSuite> suite) {
    suites_.push_back(suite.run);
  }

  template <class... Ts>
  auto on(events::test<Ts...> test) {
    path_[level_] = test.name;

    auto execute = std::empty(test.tag);
    for (const auto& tag_element : test.tag) {
      if (utility::is_match(tag_element, "skip")) {
        on(events::skip<>{.type = test.type, .name = test.name});
        return;
      }

      for (const auto& ftag : tag_) {
        if (utility::is_match(tag_element, ftag)) {
          execute = true;
          break;
        }
      }
    }

    if (not execute) {
      on(events::skip<>{.type = test.type, .name = test.name});
      return;
    }

    if (filter_(level_, path_)) {
      if (not level_++) {
        reporter_.on(events::test_begin{
            .type = test.type, .name = test.name, .location = test.location});
      } else {
        reporter_.on(events::test_run{.type = test.type, .name = test.name});
      }

      if (dry_run_) {
        for (auto i = 0u; i < level_; ++i) {
          std::cout << (i ? "." : "") << path_[i];
        }
        std::cout << '\n';
      }

#if defined(__cpp_exceptions)
      try {
#endif
        test();
#if defined(__cpp_exceptions)
      } catch (const events::fatal_assertion&) {
      } catch (const std::exception& exception) {
        ++fails_;
        reporter_.on(events::exception{exception.what()});
      } catch (...) {
        ++fails_;
        reporter_.on(events::exception{"Unknown exception"});
      }
#endif

      if (not --level_) {
        reporter_.on(events::test_end{.type = test.type, .name = test.name});
      }
    }
  }

  template <class... Ts>
  auto on(events::skip<Ts...> test) {
    reporter_.on(events::test_skip{.type = test.type, .name = test.name});
  }

  template <class TExpr>
  [[nodiscard]] auto on(events::assertion<TExpr> assertion) -> bool {
    if (dry_run_) {
      return true;
    }

    if (static_cast<bool>(assertion.expr)) {
      reporter_.on(events::assertion_pass<TExpr>{
          .expr = assertion.expr, .location = assertion.location});
      return true;
    }

    ++fails_;
    reporter_.on(events::assertion_fail<TExpr>{.expr = assertion.expr,
                                               .location = assertion.location});
    return false;
  }

  auto on(events::fatal_assertion fatal_assertion) {
    reporter_.on(fatal_assertion);

#if defined(__cpp_exceptions)
    if (not level_) {
      reporter_.on(events::summary{});
    }
    throw fatal_assertion;
#else
    if (level_) {
      reporter_.on(events::test_end{});
    }
    reporter_.on(events::summary{});
    std::abort();
#endif
  }

  template <class TMsg>
  auto on(events::log<TMsg> l) {
    reporter_.on(l);
  }

  [[nodiscard]] auto run(run_cfg rc = {}) -> bool {
    run_ = true;
    for (const auto& suite : suites_) {
      suite();
    }
    suites_.clear();

    if (rc.report_errors) {
      reporter_.on(events::summary{});
    }

    return fails_ > 0;
  }

 protected:
  TReporter reporter_{};
  std::vector<void (*)()> suites_{};
  std::size_t level_{};
  bool run_{};
  std::size_t fails_{};
  std::array<std::string_view, MaxPathSize> path_{};
  filter filter_{};
  std::vector<std::string_view> tag_{};
  bool dry_run_{};
};

struct override {};

template <class = override, class...>
[[maybe_unused]] inline auto cfg = runner<reporter<printer>>{};

namespace detail {
struct tag {
  std::vector<std::string_view> name{};
};

template <class... Ts, class TEvent>
[[nodiscard]] constexpr decltype(auto) on(TEvent&& event) {
  return ut::cfg<typename type_traits::identity<override, Ts...>::type>.on(
      static_cast<TEvent&&>(event));
}

template <class Test>
struct test_location {
  template <class T>
  constexpr test_location(const T& t,
                          const reflection::source_location& sl =
                              reflection::source_location::current())
      : test{t}, location{sl} {}

  Test test{};
  reflection::source_location location{};
};

struct test {
  std::string_view type{};
  std::string_view name{};
  std::vector<std::string_view> tag{};

  template <class... Ts>
  constexpr auto operator=(test_location<void (*)()> _test) {
    on<Ts...>(events::test<void (*)()>{.type = type,
                                       .name = name,
                                       .tag = tag,
                                       .location = _test.location,
                                       .arg = none{},
                                       .run = _test.test});
    return _test.test;
  }

  template <class Test,
            type_traits::requires_t<
                not type_traits::is_convertible_v<Test, void (*)()>> = 0>
  constexpr auto operator=(Test _test) ->
      typename type_traits::identity<Test, decltype(_test())>::type {
    on<Test>(events::test<Test>{.type = type,
                                .name = name,
                                .tag = tag,
                                .location = {},
                                .arg = none{},
                                .run = static_cast<Test&&>(_test)});
    return _test;
  }

  constexpr auto operator=(void (*_test)(std::string_view)) const {
    return _test(name);
  }

  template <class Test,
            type_traits::requires_t<not type_traits::is_convertible_v<
                Test, void (*)(std::string_view)>> = 0>
  constexpr auto operator=(Test _test)
      -> decltype(_test(type_traits::declval<std::string_view>())) {
    return _test(name);
  }
};

struct log {
  struct next {
    template <class TMsg>
    auto& operator<<(const TMsg& msg) {
      on<TMsg>(events::log{' '});
      on<TMsg>(events::log{msg});
      return *this;
    }
  };

  template <class TMsg>
  auto operator<<(const TMsg& msg) -> next {
    on<TMsg>(events::log{'\n'});
    on<TMsg>(events::log{msg});
    return next{};
  }
};

template <class TExpr>
class terse_ {
 public:
  constexpr explicit terse_(const TExpr& expr) : expr_{expr} { cfg::wip = {}; }

  ~terse_() noexcept(false) {
    if (static auto once = true; once and not cfg::wip) {
      once = {};
    } else {
      return;
    }

    cfg::wip = true;

    void(detail::on<TExpr>(
        events::assertion<TExpr>{.expr = expr_, .location = cfg::location}));
  }

 private:
  const TExpr& expr_;
};

struct that_ {
  template <class T>
  struct expr {
    using type = expr;

    constexpr explicit expr(const T& t) : t_{t} {}

    [[nodiscard]] constexpr auto operator!() const { return not_{*this}; }

    template <class TRhs>
    [[nodiscard]] constexpr auto operator==(const TRhs& rhs) const {
      return eq_{t_, rhs};
    }

    template <class TRhs>
    [[nodiscard]] constexpr auto operator!=(const TRhs& rhs) const {
      return neq_{t_, rhs};
    }

    template <class TRhs>
    [[nodiscard]] constexpr auto operator>(const TRhs& rhs) const {
      return gt_{t_, rhs};
    }

    template <class TRhs>
    [[nodiscard]] constexpr auto operator>=(const TRhs& rhs) const {
      return ge_{t_, rhs};
    }

    template <class TRhs>
    [[nodiscard]] constexpr auto operator<(const TRhs& rhs) const {
      return lt_{t_, rhs};
    }

    template <class TRhs>
    [[nodiscard]] constexpr auto operator<=(const TRhs& rhs) const {
      return le_{t_, rhs};
    }

    [[nodiscard]] constexpr operator bool() const {
      return static_cast<bool>(t_);
    }

    const T t_{};
  };

  template <class T>
  [[nodiscard]] constexpr auto operator%(const T& t) const {
    return expr{t};
  }
};

template <class TExpr>
struct fatal_ : op {
  using type = fatal_;

  constexpr explicit fatal_(const TExpr& expr) : expr_{expr} {}

  [[nodiscard]] constexpr operator bool() const {
    if (static_cast<bool>(expr_)) {
    } else {
      cfg::wip = true;
      void(on<TExpr>(
          events::assertion<TExpr>{.expr = expr_, .location = cfg::location}));
      on<TExpr>(events::fatal_assertion{});
    }
    return static_cast<bool>(expr_);
  }

  [[nodiscard]] constexpr decltype(auto) get() const { return expr_; }

  TExpr expr_{};
};

template <class T>
struct expect_ {
  constexpr explicit expect_(bool value) : value_{value} { cfg::wip = {}; }

  template <class TMsg>
  auto& operator<<(const TMsg& msg) {
    if (not value_) {
      on<T>(events::log{' '});
      on<T>(events::log{msg});
    }
    return *this;
  }

  [[nodiscard]] constexpr operator bool() const { return value_; }

  bool value_{};
};
}  // namespace detail

namespace literals {
[[nodiscard]] inline auto operator""_test(const char* name,
                                          decltype(sizeof("")) size) {
  return detail::test{"test", std::string_view{name, size}};
}

template <char... Cs>
[[nodiscard]] constexpr auto operator""_i() {
  return detail::integral_constant<math::num<int, Cs...>()>{};
}

template <char... Cs>
[[nodiscard]] constexpr auto operator""_s() {
  return detail::integral_constant<math::num<short, Cs...>()>{};
}

template <char... Cs>
[[nodiscard]] constexpr auto operator""_c() {
  return detail::integral_constant<math::num<char, Cs...>()>{};
}

template <char... Cs>
[[nodiscard]] constexpr auto operator""_sc() {
  return detail::integral_constant<math::num<signed char, Cs...>()>{};
}

template <char... Cs>
[[nodiscard]] constexpr auto operator""_l() {
  return detail::integral_constant<math::num<long, Cs...>()>{};
}

template <char... Cs>
[[nodiscard]] constexpr auto operator""_ll() {
  return detail::integral_constant<math::num<long long, Cs...>()>{};
}

template <char... Cs>
[[nodiscard]] constexpr auto operator""_u() {
  return detail::integral_constant<math::num<unsigned, Cs...>()>{};
}

template <char... Cs>
[[nodiscard]] constexpr auto operator""_uc() {
  return detail::integral_constant<math::num<unsigned char, Cs...>()>{};
}

template <char... Cs>
[[nodiscard]] constexpr auto operator""_us() {
  return detail::integral_constant<math::num<unsigned short, Cs...>()>{};
}

template <char... Cs>
[[nodiscard]] constexpr auto operator""_ul() {
  return detail::integral_constant<math::num<unsigned long, Cs...>()>{};
}

template <char... Cs>
[[nodiscard]] constexpr auto operator""_ull() {
  return detail::integral_constant<math::num<unsigned long long, Cs...>()>{};
}

template <char... Cs>
[[nodiscard]] constexpr auto operator""_i8() {
  return detail::integral_constant<math::num<std::int8_t, Cs...>()>{};
}

template <char... Cs>
[[nodiscard]] constexpr auto operator""_i16() {
  return detail::integral_constant<math::num<std::int16_t, Cs...>()>{};
}

template <char... Cs>
[[nodiscard]] constexpr auto operator""_i32() {
  return detail::integral_constant<math::num<std::int32_t, Cs...>()>{};
}

template <char... Cs>
[[nodiscard]] constexpr auto operator""_i64() {
  return detail::integral_constant<math::num<std::int64_t, Cs...>()>{};
}

template <char... Cs>
[[nodiscard]] constexpr auto operator""_u8() {
  return detail::integral_constant<math::num<std::uint8_t, Cs...>()>{};
}

template <char... Cs>
[[nodiscard]] constexpr auto operator""_u16() {
  return detail::integral_constant<math::num<std::uint16_t, Cs...>()>{};
}

template <char... Cs>
[[nodiscard]] constexpr auto operator""_u32() {
  return detail::integral_constant<math::num<std::uint32_t, Cs...>()>{};
}

template <char... Cs>
[[nodiscard]] constexpr auto operator""_u64() {
  return detail::integral_constant<math::num<std::uint64_t, Cs...>()>{};
}

template <char... Cs>
[[nodiscard]] constexpr auto operator""_f() {
  return detail::floating_point_constant<
      float, math::num<unsigned long, Cs...>(),
      math::den<unsigned long, Cs...>(),
      math::den_size<unsigned long, Cs...>()>{};
}

template <char... Cs>
[[nodiscard]] constexpr auto operator""_d() {
  return detail::floating_point_constant<
      double, math::num<unsigned long, Cs...>(),
      math::den<unsigned long, Cs...>(),
      math::den_size<unsigned long, Cs...>()>{};
}

template <char... Cs>
[[nodiscard]] constexpr auto operator""_ld() {
  return detail::floating_point_constant<
      long double, math::num<unsigned long long, Cs...>(),
      math::den<unsigned long long, Cs...>(),
      math::den_size<unsigned long long, Cs...>()>{};
}

constexpr auto operator""_b(const char* name, decltype(sizeof("")) size) {
  struct named : std::string_view, detail::op {
    using value_type = bool;
    [[nodiscard]] constexpr operator value_type() const { return true; }
    [[nodiscard]] constexpr auto operator==(const named&) const { return true; }
    [[nodiscard]] constexpr auto operator==(const bool other) const {
      return other;
    }
  };
  return named{{name, size}, {}};
}
}  // namespace literals

namespace operators {
[[nodiscard]] constexpr auto operator==(std::string_view lhs,
                                        std::string_view rhs) {
  return detail::eq_{lhs, rhs};
}

[[nodiscard]] constexpr auto operator!=(std::string_view lhs,
                                        std::string_view rhs) {
  return detail::neq_{lhs, rhs};
}

template <class T, type_traits::requires_t<type_traits::is_container_v<T>> = 0>
[[nodiscard]] constexpr auto operator==(T&& lhs, T&& rhs) {
  return detail::eq_{static_cast<T&&>(lhs), static_cast<T&&>(rhs)};
}

template <class T, type_traits::requires_t<type_traits::is_container_v<T>> = 0>
[[nodiscard]] constexpr auto operator!=(T&& lhs, T&& rhs) {
  return detail::neq_{static_cast<T&&>(lhs), static_cast<T&&>(rhs)};
}

template <class TLhs, class TRhs,
          type_traits::requires_t<type_traits::is_op_v<TLhs> or
                                  type_traits::is_op_v<TRhs>> = 0>
[[nodiscard]] constexpr auto operator==(const TLhs& lhs, const TRhs& rhs) {
  return detail::eq_{lhs, rhs};
}

template <class TLhs, class TRhs,
          type_traits::requires_t<type_traits::is_op_v<TLhs> or
                                  type_traits::is_op_v<TRhs>> = 0>
[[nodiscard]] constexpr auto operator!=(const TLhs& lhs, const TRhs& rhs) {
  return detail::neq_{lhs, rhs};
}

template <class TLhs, class TRhs,
          type_traits::requires_t<type_traits::is_op_v<TLhs> or
                                  type_traits::is_op_v<TRhs>> = 0>
[[nodiscard]] constexpr auto operator>(const TLhs& lhs, const TRhs& rhs) {
  return detail::gt_{lhs, rhs};
}

template <class TLhs, class TRhs,
          type_traits::requires_t<type_traits::is_op_v<TLhs> or
                                  type_traits::is_op_v<TRhs>> = 0>
[[nodiscard]] constexpr auto operator>=(const TLhs& lhs, const TRhs& rhs) {
  return detail::ge_{lhs, rhs};
}

template <class TLhs, class TRhs,
          type_traits::requires_t<type_traits::is_op_v<TLhs> or
                                  type_traits::is_op_v<TRhs>> = 0>
[[nodiscard]] constexpr auto operator<(const TLhs& lhs, const TRhs& rhs) {
  return detail::lt_{lhs, rhs};
}

template <class TLhs, class TRhs,
          type_traits::requires_t<type_traits::is_op_v<TLhs> or
                                  type_traits::is_op_v<TRhs>> = 0>
[[nodiscard]] constexpr auto operator<=(const TLhs& lhs, const TRhs& rhs) {
  return detail::le_{lhs, rhs};
}

template <class TLhs, class TRhs,
          type_traits::requires_t<type_traits::is_op_v<TLhs> or
                                  type_traits::is_op_v<TRhs>> = 0>
[[nodiscard]] constexpr auto operator and(const TLhs& lhs, const TRhs& rhs) {
  return detail::and_{lhs, rhs};
}

template <class TLhs, class TRhs,
          type_traits::requires_t<type_traits::is_op_v<TLhs> or
                                  type_traits::is_op_v<TRhs>> = 0>
[[nodiscard]] constexpr auto operator or(const TLhs& lhs, const TRhs& rhs) {
  return detail::or_{lhs, rhs};
}

template <class T, type_traits::requires_t<type_traits::is_op_v<T>> = 0>
[[nodiscard]] constexpr auto operator not(const T& t) {
  return detail::not_{t};
}

template <class T>
[[nodiscard]] inline auto operator>>(
    const T& t, const detail::value_location<detail::fatal>&) {
  return detail::fatal_{t};
}

template <class Test>
[[nodiscard]] auto operator/(const detail::tag& tag, Test test) {
  for (const auto& name : tag.name) {
    test.tag.push_back(name);
  }
  return test;
}

[[nodiscard]] inline auto operator/(const detail::tag& lhs,
                                    const detail::tag& rhs) {
  std::vector<std::string_view> tag{};
  for (const auto& name : lhs.name) {
    tag.push_back(name);
  }
  for (const auto& name : rhs.name) {
    tag.push_back(name);
  }
  return detail::tag{tag};
}

template <class F, class T,
          type_traits::requires_t<type_traits::is_container_v<T>> = 0>
[[nodiscard]] constexpr auto operator|(const F& f, const T& t) {
  return [f, t](const auto name) {
    for (const auto& arg : t) {
      detail::on<F>(events::test<F, typename T::value_type>{.type = "test",
                                                            .name = name,
                                                            .tag = {},
                                                            .location = {},
                                                            .arg = arg,
                                                            .run = f});
    }
  };
}

template <
    class F, template <class...> class T, class... Ts,
    type_traits::requires_t<not type_traits::is_container_v<T<Ts...>>> = 0>
[[nodiscard]] constexpr auto operator|(const F& f, const T<Ts...>& t) {
  return [f, t](const auto name) {
    apply(
        [f, name](const auto&... args) {
          (detail::on<F>(events::test<F, Ts>{.type = "test",
                                             .name = name,
                                             .tag = {},
                                             .location = {},
                                             .arg = args,
                                             .run = f}),
           ...);
        },
        t);
  };
}

namespace terse {
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wunused-comparison"
#endif

[[maybe_unused]] constexpr struct {
} _t;

template <class T>
constexpr auto operator%(const T& t, const decltype(_t)&) {
  return detail::value<T>{t};
}

template <class T>
inline auto operator>>(const T& t,
                       const detail::value_location<detail::fatal>&) {
  using fatal_t = detail::fatal_<T>;
  struct fatal_ : fatal_t, detail::log {
    using type [[maybe_unused]] = fatal_t;
    using fatal_t::fatal_t;
    const detail::terse_<fatal_t> _{*this};
  };
  return fatal_{t};
}

template <class T, type_traits::requires_t<type_traits::is_op_v<T>> = 0>
constexpr auto operator==(
    const T& lhs, const detail::value_location<typename T::value_type>& rhs) {
  using eq_t = detail::eq_<T, detail::value_location<typename T::value_type>>;
  struct eq_ : eq_t, detail::log {
    using type [[maybe_unused]] = eq_t;
    using eq_t::eq_t;
    const detail::terse_<eq_t> _{*this};
  };
  return eq_{lhs, rhs};
}

template <class T, type_traits::requires_t<type_traits::is_op_v<T>> = 0>
constexpr auto operator==(
    const detail::value_location<typename T::value_type>& lhs, const T& rhs) {
  using eq_t = detail::eq_<detail::value_location<typename T::value_type>, T>;
  struct eq_ : eq_t, detail::log {
    using type [[maybe_unused]] = eq_t;
    using eq_t::eq_t;
    const detail::terse_<eq_t> _{*this};
  };
  return eq_{lhs, rhs};
}

template <class T, type_traits::requires_t<type_traits::is_op_v<T>> = 0>
constexpr auto operator!=(
    const T& lhs, const detail::value_location<typename T::value_type>& rhs) {
  using neq_t = detail::neq_<T, detail::value_location<typename T::value_type>>;
  struct neq_ : neq_t, detail::log {
    using type [[maybe_unused]] = neq_t;
    using neq_t::neq_t;
    const detail::terse_<neq_t> _{*this};
  };
  return neq_{lhs, rhs};
}

template <class T, type_traits::requires_t<type_traits::is_op_v<T>> = 0>
constexpr auto operator!=(
    const detail::value_location<typename T::value_type>& lhs, const T& rhs) {
  using neq_t = detail::neq_<detail::value_location<typename T::value_type>, T>;
  struct neq_ : neq_t {
    using type [[maybe_unused]] = neq_t;
    using neq_t::neq_t;
    const detail::terse_<neq_t> _{*this};
  };
  return neq_{lhs, rhs};
}

template <class T, type_traits::requires_t<type_traits::is_op_v<T>> = 0>
constexpr auto operator>(
    const T& lhs, const detail::value_location<typename T::value_type>& rhs) {
  using gt_t = detail::gt_<T, detail::value_location<typename T::value_type>>;
  struct gt_ : gt_t, detail::log {
    using type [[maybe_unused]] = gt_t;
    using gt_t::gt_t;
    const detail::terse_<gt_t> _{*this};
  };
  return gt_{lhs, rhs};
}

template <class T, type_traits::requires_t<type_traits::is_op_v<T>> = 0>
constexpr auto operator>(
    const detail::value_location<typename T::value_type>& lhs, const T& rhs) {
  using gt_t = detail::gt_<detail::value_location<typename T::value_type>, T>;
  struct gt_ : gt_t, detail::log {
    using type [[maybe_unused]] = gt_t;
    using gt_t::gt_t;
    const detail::terse_<gt_t> _{*this};
  };
  return gt_{lhs, rhs};
}

template <class T, type_traits::requires_t<type_traits::is_op_v<T>> = 0>
constexpr auto operator>=(
    const T& lhs, const detail::value_location<typename T::value_type>& rhs) {
  using ge_t = detail::ge_<T, detail::value_location<typename T::value_type>>;
  struct ge_ : ge_t, detail::log {
    using type [[maybe_unused]] = ge_t;
    using ge_t::ge_t;
    const detail::terse_<ge_t> _{*this};
  };
  return ge_{lhs, rhs};
}

template <class T, type_traits::requires_t<type_traits::is_op_v<T>> = 0>
constexpr auto operator>=(
    const detail::value_location<typename T::value_type>& lhs, const T& rhs) {
  using ge_t = detail::ge_<detail::value_location<typename T::value_type>, T>;
  struct ge_ : ge_t, detail::log {
    using type [[maybe_unused]] = ge_t;
    using ge_t::ge_t;
    const detail::terse_<ge_t> _{*this};
  };
  return ge_{lhs, rhs};
}

template <class T, type_traits::requires_t<type_traits::is_op_v<T>> = 0>
constexpr auto operator<(
    const T& lhs, const detail::value_location<typename T::value_type>& rhs) {
  using lt_t = detail::lt_<T, detail::value_location<typename T::value_type>>;
  struct lt_ : lt_t, detail::log {
    using type [[maybe_unused]] = lt_t;
    using lt_t::lt_t;
    const detail::terse_<lt_t> _{*this};
  };
  return lt_{lhs, rhs};
}

template <class T, type_traits::requires_t<type_traits::is_op_v<T>> = 0>
constexpr auto operator<(
    const detail::value_location<typename T::value_type>& lhs, const T& rhs) {
  using lt_t = detail::lt_<detail::value_location<typename T::value_type>, T>;
  struct lt_ : lt_t, detail::log {
    using type [[maybe_unused]] = lt_t;
    using lt_t::lt_t;
    const detail::terse_<lt_t> _{*this};
  };
  return lt_{lhs, rhs};
}

template <class T, type_traits::requires_t<type_traits::is_op_v<T>> = 0>
constexpr auto operator<=(
    const T& lhs, const detail::value_location<typename T::value_type>& rhs) {
  using le_t = detail::le_<T, detail::value_location<typename T::value_type>>;
  struct le_ : le_t, detail::log {
    using type [[maybe_unused]] = le_t;
    using le_t::le_t;
    const detail::terse_<le_t> _{*this};
  };
  return le_{lhs, rhs};
}

template <class T, type_traits::requires_t<type_traits::is_op_v<T>> = 0>
constexpr auto operator<=(
    const detail::value_location<typename T::value_type>& lhs, const T& rhs) {
  using le_t = detail::le_<detail::value_location<typename T::value_type>, T>;
  struct le_ : le_t {
    using type [[maybe_unused]] = le_t;
    using le_t::le_t;
    const detail::terse_<le_t> _{*this};
  };
  return le_{lhs, rhs};
}

template <class TLhs, class TRhs,
          type_traits::requires_t<type_traits::is_op_v<TLhs> or
                                  type_traits::is_op_v<TRhs>> = 0>
constexpr auto operator and(const TLhs& lhs, const TRhs& rhs) {
  using and_t = detail::and_<typename TLhs::type, typename TRhs::type>;
  struct and_ : and_t, detail::log {
    using type [[maybe_unused]] = and_t;
    using and_t::and_t;
    const detail::terse_<and_t> _{*this};
  };
  return and_{lhs, rhs};
}

template <class TLhs, class TRhs,
          type_traits::requires_t<type_traits::is_op_v<TLhs> or
                                  type_traits::is_op_v<TRhs>> = 0>
constexpr auto operator or(const TLhs& lhs, const TRhs& rhs) {
  using or_t = detail::or_<typename TLhs::type, typename TRhs::type>;
  struct or_ : or_t, detail::log {
    using type [[maybe_unused]] = or_t;
    using or_t::or_t;
    const detail::terse_<or_t> _{*this};
  };
  return or_{lhs, rhs};
}

template <class T, type_traits::requires_t<type_traits::is_op_v<T>> = 0>
constexpr auto operator not(const T& t) {
  using not_t = detail::not_<typename T::type>;
  struct not_ : not_t, detail::log {
    using type [[maybe_unused]] = not_t;
    using not_t::not_t;
    const detail::terse_<not_t> _{*this};
  };
  return not_{t};
}

}  // namespace terse
}  // namespace operators

template <class TExpr, type_traits::requires_t<
                           type_traits::is_op_v<TExpr> or
                           type_traits::is_convertible_v<TExpr, bool>> = 0>
constexpr auto expect(const TExpr& expr,
                      const reflection::source_location& sl =
                          reflection::source_location::current()) {
  return detail::expect_<TExpr>{detail::on<TExpr>(
      events::assertion<TExpr>{.expr = expr, .location = sl})};
}

[[maybe_unused]] constexpr auto fatal = detail::fatal{};

#if defined(__cpp_nontype_template_parameter_class)
template <auto Constant>
#else
template <bool Constant>
#endif
constexpr auto constant = Constant;

#if defined(__cpp_exceptions)
template <class TException, class TExpr>
[[nodiscard]] constexpr auto throws(const TExpr& expr) {
  return detail::throws_<TExpr, TException>{expr};
}

template <class TExpr>
[[nodiscard]] constexpr auto throws(const TExpr& expr) {
  return detail::throws_<TExpr>{expr};
}

template <class TExpr>
[[nodiscard]] constexpr auto nothrow(const TExpr& expr) {
  return detail::nothrow_{expr};
}
#endif

#if __has_include(<unistd.h>) and __has_include(<sys/wait.h>)
template <class TExpr>
[[nodiscard]] constexpr auto aborts(const TExpr& expr) {
  return detail::aborts_{expr};
}
#endif

using _b = detail::value<bool>;
using _c = detail::value<char>;
using _sc = detail::value<signed char>;
using _s = detail::value<short>;
using _i = detail::value<int>;
using _l = detail::value<long>;
using _ll = detail::value<long long>;
using _u = detail::value<unsigned>;
using _uc = detail::value<unsigned char>;
using _us = detail::value<unsigned short>;
using _ul = detail::value<unsigned long>;
using _ull = detail::value<unsigned long long>;
using _i8 = detail::value<std::int8_t>;
using _i16 = detail::value<std::int16_t>;
using _i32 = detail::value<std::int32_t>;
using _i64 = detail::value<std::int64_t>;
using _u8 = detail::value<std::uint8_t>;
using _u16 = detail::value<std::uint16_t>;
using _u32 = detail::value<std::uint32_t>;
using _u64 = detail::value<std::uint64_t>;
using _f = detail::value<float>;
using _d = detail::value<double>;
using _ld = detail::value<long double>;

template <class T>
struct _t : detail::value<T> {
  constexpr explicit _t(const T& t) : detail::value<T>{t} {}
};

struct suite {
  template <class TSuite>
  constexpr /*explicit(false)*/ suite(TSuite _suite) {
    static_assert(1 == sizeof(_suite));
    detail::on<decltype(+_suite)>(
        events::suite<decltype(+_suite)>{.run = +_suite});
  }
};

[[maybe_unused]] inline auto log = detail::log{};
[[maybe_unused]] inline auto that = detail::that_{};
[[maybe_unused]] constexpr auto test = [](const auto name) {
  return detail::test{"test", name};
};
[[maybe_unused]] constexpr auto should = test;
[[maybe_unused]] inline auto tag = [](const auto name) {
  return detail::tag{{name}};
};
[[maybe_unused]] inline auto skip = tag("skip");
template <class T = void>
[[maybe_unused]] constexpr auto type = detail::type_<T>();

template <class TLhs, class TRhs>
[[nodiscard]] constexpr auto eq(const TLhs& lhs, const TRhs& rhs) {
  return detail::eq_{lhs, rhs};
}
template <class TLhs, class TRhs>
[[nodiscard]] constexpr auto neq(const TLhs& lhs, const TRhs& rhs) {
  return detail::neq_{lhs, rhs};
}
template <class TLhs, class TRhs>
[[nodiscard]] constexpr auto gt(const TLhs& lhs, const TRhs& rhs) {
  return detail::gt_{lhs, rhs};
}
template <class TLhs, class TRhs>
[[nodiscard]] constexpr auto ge(const TLhs& lhs, const TRhs& rhs) {
  return detail::ge_{lhs, rhs};
}
template <class TLhs, class TRhs>
[[nodiscard]] constexpr auto lt(const TLhs& lhs, const TRhs& rhs) {
  return detail::lt_{lhs, rhs};
}
template <class TLhs, class TRhs>
[[nodiscard]] constexpr auto le(const TLhs& lhs, const TRhs& rhs) {
  return detail::le_{lhs, rhs};
}

template <class T>
[[nodiscard]] constexpr auto mut(const T& t) noexcept -> T& {
  return const_cast<T&>(t);
}

namespace bdd {
[[maybe_unused]] constexpr auto feature = [](const auto name) {
  return detail::test{"feature", name};
};
[[maybe_unused]] constexpr auto scenario = [](const auto name) {
  return detail::test{"scenario", name};
};
[[maybe_unused]] constexpr auto given = [](const auto name) {
  return detail::test{"given", name};
};
[[maybe_unused]] constexpr auto when = [](const auto name) {
  return detail::test{"when", name};
};
[[maybe_unused]] constexpr auto then = [](const auto name) {
  return detail::test{"then", name};
};

namespace gherkin {
class steps {
  using step_t = std::string;
  using steps_t = void (*)(steps&);
  using gherkin_t = std::vector<step_t>;
  using call_step_t = utility::function<void(const std::string&)>;
  using call_steps_t = std::vector<std::pair<step_t, call_step_t>>;

  class step {
   public:
    template <class TPattern>
    step(steps& steps, const TPattern& pattern)
        : steps_{steps}, pattern_{pattern} {}

    ~step() { steps_.next(pattern_); }

    template <class TExpr>
    auto operator=(const TExpr& expr) -> void {
      for (const auto& [pattern, _] : steps_.call_steps()) {
        if (pattern_ == pattern) {
          return;
        }
      }

      steps_.call_steps().emplace_back(
          pattern_, [expr, pattern = pattern_](const auto& _step) {
            [=]<class... TArgs>(type_traits::list<TArgs...>) {
              log << _step;
              auto i = 0u;
              const auto& ms = utility::match(pattern, _step);
              expr(lexical_cast<TArgs>(ms[i++])...);
            }
            (typename type_traits::function_traits<TExpr>::args{});
          });
    }

   private:
    template <class T>
    static auto lexical_cast(const std::string& str) {
      T t{};
      std::istringstream iss{};
      iss.str(str);
      if constexpr (std::is_same_v<T, std::string>) {
        t = iss.str();
      } else {
        iss >> t;
      }
      return t;
    }

    steps& steps_;
    std::string pattern_{};
  };

 public:
  template <class TSteps>
  constexpr /*explicit(false)*/ steps(const TSteps& _steps) : steps_{_steps} {}

  template <class TGherkin>
  auto operator|(const TGherkin& gherkin) {
    gherkin_ = utility::split<std::string>(gherkin, '\n');
    for (auto& _step : gherkin_) {
      _step.erase(0, _step.find_first_not_of(" \t"));
    }

    return [this] {
      step_ = {};
      steps_(*this);
    };
  }
  auto feature(const std::string& pattern) {
    return step{*this, "Feature: " + pattern};
  }
  auto scenario(const std::string& pattern) {
    return step{*this, "Scenario: " + pattern};
  }
  auto given(const std::string& pattern) {
    return step{*this, "Given " + pattern};
  }
  auto when(const std::string& pattern) {
    return step{*this, "When " + pattern};
  }
  auto then(const std::string& pattern) {
    return step{*this, "Then " + pattern};
  }

 private:
  template <class TPattern>
  auto next(const TPattern& pattern) -> void {
    const auto is_scenario = [&pattern](const auto& _step) {
      constexpr auto scenario = "Scenario";
      return pattern.find(scenario) == std::string::npos and
             _step.find(scenario) != std::string::npos;
    };

    const auto call_steps = [this, is_scenario](const auto& _step,
                                                const auto i) {
      for (const auto& [name, call] : call_steps_) {
        if (is_scenario(_step)) {
          break;
        }

        if (utility::is_match(_step, name) or
            not std::empty(utility::match(name, _step))) {
          step_ = i;
          call(_step);
        }
      }
    };

    decltype(step_) i{};
    for (const auto& _step : gherkin_) {
      if (i++ == step_) {
        call_steps(_step, i);
      }
    }
  }

  auto call_steps() -> call_steps_t& { return call_steps_; }

  steps_t steps_{};
  gherkin_t gherkin_{};
  call_steps_t call_steps_{};
  decltype(sizeof("")) step_{};
};
}  // namespace gherkin
}  // namespace bdd

namespace spec {
[[maybe_unused]] constexpr auto describe = [](const auto name) {
  return detail::test{"describe", name};
};
[[maybe_unused]] constexpr auto it = [](const auto name) {
  return detail::test{"it", name};
};
}  // namespace spec

using literals::operator""_test;

using literals::operator""_b;
using literals::operator""_i;
using literals::operator""_s;
using literals::operator""_c;
using literals::operator""_sc;
using literals::operator""_l;
using literals::operator""_ll;
using literals::operator""_u;
using literals::operator""_uc;
using literals::operator""_us;
using literals::operator""_ul;
using literals::operator""_i8;
using literals::operator""_i16;
using literals::operator""_i32;
using literals::operator""_i64;
using literals::operator""_u8;
using literals::operator""_u16;
using literals::operator""_u32;
using literals::operator""_u64;
using literals::operator""_f;
using literals::operator""_d;
using literals::operator""_ld;
using literals::operator""_ull;

using operators::operator==;
using operators::operator!=;
using operators::operator>;
using operators::operator>=;
using operators::operator<;
using operators::operator<=;
using operators::operator and;
using operators::operator or;
using operators::operator not;
using operators::operator|;
using operators::operator/;
using operators::operator>>;
}  // namespace boost::inline ext::ut::inline v1_1_8
#endif
