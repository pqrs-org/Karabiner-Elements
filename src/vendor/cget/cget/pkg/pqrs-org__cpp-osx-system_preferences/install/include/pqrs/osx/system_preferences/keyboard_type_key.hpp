#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <cstdint>
#include <pqrs/hash.hpp>
#include <pqrs/osx/iokit_types.hpp>

namespace pqrs {
namespace osx {
namespace system_preferences {
class keyboard_type_key final {
public:
  keyboard_type_key(void) : vendor_id_(0),
                            product_id_(0),
                            country_code_(0) {
  }

  keyboard_type_key(iokit_hid_vendor_id vendor_id,
                    iokit_hid_product_id product_id,
                    iokit_hid_country_code country_code) : vendor_id_(vendor_id),
                                                           product_id_(product_id),
                                                           country_code_(country_code) {
  }

  iokit_hid_vendor_id get_vendor_id(void) const {
    return vendor_id_;
  }

  void set_vendor_id(iokit_hid_vendor_id value) {
    vendor_id_ = value;
  }

  iokit_hid_product_id get_product_id(void) const {
    return product_id_;
  }

  void set_product_id(iokit_hid_product_id value) {
    product_id_ = value;
  }

  iokit_hid_country_code get_country_code(void) const {
    return country_code_;
  }

  void set_country_code(iokit_hid_country_code value) {
    country_code_ = value;
  }

  bool operator==(const keyboard_type_key& other) const {
    return vendor_id_ == other.vendor_id_ &&
           product_id_ == other.product_id_ &&
           country_code_ == other.country_code_;
  }

  bool operator!=(const keyboard_type_key& other) const {
    return !(*this == other);
  }

  bool operator<(const keyboard_type_key& other) const {
    if (vendor_id_ != other.vendor_id_) {
      return vendor_id_ < other.vendor_id_;
    }
    if (product_id_ != other.product_id_) {
      return product_id_ < other.product_id_;
    }
    if (country_code_ != other.country_code_) {
      return country_code_ < other.country_code_;
    }
    return false;
  }

  bool operator<=(const keyboard_type_key& other) const {
    return *this < other || *this == other;
  }

  bool operator>(const keyboard_type_key& other) const {
    return !(*this <= other);
  }

  bool operator>=(const keyboard_type_key& other) const {
    return !(*this < other);
  }

private:
  iokit_hid_vendor_id vendor_id_;
  iokit_hid_product_id product_id_;
  iokit_hid_country_code country_code_;
};
} // namespace system_preferences
} // namespace osx
} // namespace pqrs

namespace std {
template <>
struct hash<pqrs::osx::system_preferences::keyboard_type_key> final {
  std::size_t operator()(const pqrs::osx::system_preferences::keyboard_type_key& value) const {
    std::size_t h = 0;

    pqrs::hash_combine(h, value.get_vendor_id());
    pqrs::hash_combine(h, value.get_product_id());
    pqrs::hash_combine(h, value.get_country_code());

    return h;
  }
};
} // namespace std
