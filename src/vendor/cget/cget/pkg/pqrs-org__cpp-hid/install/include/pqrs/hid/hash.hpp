#pragma once

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include "product_id.hpp"
#include "usage.hpp"
#include "usage_page.hpp"
#include "vendor_id.hpp"
#include <tuple>
#include <utility>

namespace std {
template <>
struct hash<std::pair<pqrs::hid::usage_page::value_t, pqrs::hid::usage::value_t>> {
  size_t operator()(const std::pair<pqrs::hid::usage_page::value_t, pqrs::hid::usage::value_t>& pair) const {
    auto h1 = std::hash<int32_t>{}(type_safe::get(pair.first));
    auto h2 = std::hash<int32_t>{}(type_safe::get(pair.second));
    return h1 ^ (h2 << 16);
  }
};

template <>
struct hash<std::pair<pqrs::hid::vendor_id::value_t, pqrs::hid::product_id::value_t>> {
  size_t operator()(const std::pair<pqrs::hid::vendor_id::value_t, pqrs::hid::product_id::value_t>& pair) const {
    auto h1 = std::hash<int32_t>{}(type_safe::get(pair.first));
    auto h2 = std::hash<int32_t>{}(type_safe::get(pair.second));
    return h1 ^ (h2 << 16);
  }
};
} // namespace std
