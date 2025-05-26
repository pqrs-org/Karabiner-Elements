#pragma once

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "product_id.hpp"
#include "usage.hpp"
#include "usage_page.hpp"
#include "vendor_id.hpp"
#include <pqrs/hash.hpp>
#include <tuple>
#include <utility>

namespace std {
template <>
struct hash<std::pair<pqrs::hid::usage_page::value_t, pqrs::hid::usage::value_t>> {
  size_t operator()(const std::pair<pqrs::hid::usage_page::value_t, pqrs::hid::usage::value_t>& pair) const {
    size_t h = 0;
    pqrs::hash::combine(h, type_safe::get(pair.first));
    pqrs::hash::combine(h, type_safe::get(pair.second));
    return h;
  }
};

template <>
struct hash<std::pair<pqrs::hid::vendor_id::value_t, pqrs::hid::product_id::value_t>> {
  size_t operator()(const std::pair<pqrs::hid::vendor_id::value_t, pqrs::hid::product_id::value_t>& pair) const {
    size_t h = 0;
    pqrs::hash::combine(h, type_safe::get(pair.first));
    pqrs::hash::combine(h, type_safe::get(pair.second));
    return h;
  }
};
} // namespace std
