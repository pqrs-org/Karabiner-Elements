#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include "iokit_hid_location_id.hpp"
#include <pqrs/hid.hpp>
#include <tuple>
#include <utility>

namespace std {
template <>
struct hash<std::tuple<pqrs::hid::vendor_id::value_t, pqrs::hid::product_id::value_t, pqrs::osx::iokit_hid_location_id::value_t>> {
  size_t operator()(const std::tuple<pqrs::hid::vendor_id::value_t, pqrs::hid::product_id::value_t, pqrs::osx::iokit_hid_location_id::value_t>& tuple) const {
    auto h1 = std::hash<int32_t>{}(type_safe::get(std::get<0>(tuple)));
    auto h2 = std::hash<int32_t>{}(type_safe::get(std::get<1>(tuple)));
    auto h3 = std::hash<int32_t>{}(type_safe::get(std::get<2>(tuple)));
    return h1 ^ (h2 << 16) ^ h3;
  }
};
} // namespace std
