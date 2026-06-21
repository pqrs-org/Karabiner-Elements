#pragma once

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "usage.hpp"
#include "usage_page.hpp"

namespace pqrs::hid {
class usage_pair final {
public:
  constexpr usage_pair() = default;

  constexpr usage_pair(usage_page::value_t usage_page,
                       usage::value_t usage)
      : usage_page_(usage_page),
        usage_(usage) {
  }

  [[nodiscard]] usage_page::value_t get_usage_page() const noexcept {
    return usage_page_;
  }

  usage_pair& set_usage_page(usage_page::value_t value) noexcept {
    usage_page_ = value;
    return *this;
  }

  [[nodiscard]] usage::value_t get_usage() const noexcept {
    return usage_;
  }

  usage_pair& set_usage(usage::value_t value) noexcept {
    usage_ = value;
    return *this;
  }

  [[nodiscard]] constexpr auto operator<=>(const usage_pair&) const noexcept = default;

private:
  usage_page::value_t usage_page_ = usage_page::undefined;
  usage::value_t usage_ = usage::undefined;
};
} // namespace pqrs::hid

//
// hash
//

namespace std {
template <>
struct hash<pqrs::hid::usage_pair> {
  [[nodiscard]] size_t operator()(const pqrs::hid::usage_pair& pair) const {
    size_t h = 0;
    pqrs::hash::combine(h, type_safe::get(pair.get_usage_page()));
    pqrs::hash::combine(h, type_safe::get(pair.get_usage()));
    return h;
  }
};
} // namespace std
