#pragma once

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "usage.hpp"
#include "usage_page.hpp"

namespace pqrs {
namespace hid {
class usage_pair final {
public:
  constexpr usage_pair(void)
      : usage_page_(usage_page::undefined),
        usage_(usage::undefined) {
  }

  constexpr usage_pair(usage_page::value_t usage_page,
                       usage::value_t usage)
      : usage_page_(usage_page),
        usage_(usage) {
  }

  usage_page::value_t get_usage_page(void) const {
    return usage_page_;
  }

  usage_pair& set_usage_page(const usage_page::value_t& value) {
    usage_page_ = value;
    return *this;
  }

  usage::value_t get_usage(void) const {
    return usage_;
  }

  usage_pair& set_usage(const usage::value_t& value) {
    usage_ = value;
    return *this;
  }

  constexpr auto operator<=>(const usage_pair&) const = default;

private:
  usage_page::value_t usage_page_;
  usage::value_t usage_;
};
} // namespace hid
} // namespace pqrs

//
// hash
//

namespace std {
template <>
struct hash<pqrs::hid::usage_pair> {
  size_t operator()(const pqrs::hid::usage_pair& pair) const {
    size_t h = 0;
    pqrs::hash::combine(h, type_safe::get(pair.get_usage_page()));
    pqrs::hash::combine(h, type_safe::get(pair.get_usage()));
    return h;
  }
};
} // namespace std
