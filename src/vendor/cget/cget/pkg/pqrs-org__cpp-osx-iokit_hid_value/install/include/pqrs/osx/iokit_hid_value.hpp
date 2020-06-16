#pragma once

// pqrs::osx::iokit_hid_value v3.1

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <IOKit/hid/IOHIDElement.h>
#include <IOKit/hid/IOHIDValue.h>
#include <optional>
#include <pqrs/hid.hpp>
#include <pqrs/osx/chrono.hpp>

namespace pqrs {
namespace osx {
class iokit_hid_value final {
public:
  iokit_hid_value(void) : time_stamp_(chrono::absolute_time_point(0)),
                          integer_value_(0) {
  }

  iokit_hid_value(chrono::absolute_time_point time_stamp,
                  CFIndex integer_value,
                  std::optional<hid::usage_page::value_t> usage_page,
                  std::optional<hid::usage::value_t> usage) : time_stamp_(time_stamp),
                                                                   integer_value_(integer_value),
                                                                   usage_page_(usage_page),
                                                                   usage_(usage) {
  }

  iokit_hid_value(IOHIDValueRef value) : iokit_hid_value() {
    if (value) {
      time_stamp_ = chrono::absolute_time_point(IOHIDValueGetTimeStamp(value));
      integer_value_ = IOHIDValueGetIntegerValue(value);
      if (auto element = IOHIDValueGetElement(value)) {
        usage_page_ = hid::usage_page::value_t(IOHIDElementGetUsagePage(element));
        usage_ = hid::usage::value_t(IOHIDElementGetUsage(element));
      }
    }
  }

  chrono::absolute_time_point get_time_stamp(void) const {
    return time_stamp_;
  }

  iokit_hid_value& set_time_stamp(chrono::absolute_time_point value) {
    time_stamp_ = value;
    return *this;
  }

  CFIndex get_integer_value(void) const {
    return integer_value_;
  }

  iokit_hid_value& set_integer_value(CFIndex value) {
    integer_value_ = value;
    return *this;
  }

  std::optional<hid::usage_page::value_t> get_usage_page(void) const {
    return usage_page_;
  }

  iokit_hid_value& set_usage_page(const std::optional<hid::usage_page::value_t>& value) {
    usage_page_ = value;
    return *this;
  }

  std::optional<hid::usage::value_t> get_usage(void) const {
    return usage_;
  }

  iokit_hid_value& set_usage(const std::optional<hid::usage::value_t>& value) {
    usage_ = value;
    return *this;
  }

  bool conforms_to(hid::usage_page::value_t usage_page,
                   hid::usage::value_t usage) const {
    return usage_page_ == usage_page &&
           usage_ == usage;
  }

  bool operator==(const iokit_hid_value& other) const {
    return time_stamp_ == other.time_stamp_ &&
           integer_value_ == other.integer_value_ &&
           usage_page_ == other.usage_page_ &&
           usage_ == other.usage_;
  }

  bool operator!=(const iokit_hid_value& other) const {
    return !(*this == other);
  }

private:
  chrono::absolute_time_point time_stamp_;
  CFIndex integer_value_;
  std::optional<hid::usage_page::value_t> usage_page_;
  std::optional<hid::usage::value_t> usage_;
};
} // namespace osx
} // namespace pqrs
