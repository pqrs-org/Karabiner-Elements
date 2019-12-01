#pragma once

#include "types/absolute_time_point.hpp"
#include "types/hid_usage.hpp"
#include "types/hid_usage_page.hpp"
#include <IOKit/hid/IOHIDElement.h>
#include <IOKit/hid/IOHIDValue.h>
#include <nlohmann/json.hpp>
#include <optional>

namespace krbn {
class hid_value final {
public:
  hid_value(absolute_time_point time_stamp,
            CFIndex integer_value,
            std::optional<hid_usage_page> hid_usage_page,
            std::optional<hid_usage> hid_usage) : time_stamp_(time_stamp),
                                                  integer_value_(integer_value),
                                                  hid_usage_page_(hid_usage_page),
                                                  hid_usage_(hid_usage) {
  }

  hid_value(IOHIDValueRef value) {
    time_stamp_ = absolute_time_point(IOHIDValueGetTimeStamp(value));
    integer_value_ = IOHIDValueGetIntegerValue(value);
    if (auto element = IOHIDValueGetElement(value)) {
      hid_usage_page_ = hid_usage_page(IOHIDElementGetUsagePage(element));
      hid_usage_ = hid_usage(IOHIDElementGetUsage(element));
    }
  }

  nlohmann::json to_json(void) const {
    nlohmann::json j;
    j["time_stamp"] = type_safe::get(time_stamp_);
    j["integer_value"] = integer_value_;
    if (hid_usage_page_) {
      j["hid_usage_page"] = *hid_usage_page_;
    }
    if (hid_usage_) {
      j["hid_usage"] = *hid_usage_;
    }
    return j;
  }

  absolute_time_point get_time_stamp(void) const {
    return time_stamp_;
  }

  hid_value& set_time_stamp(absolute_time_point value) {
    time_stamp_ = value;
    return *this;
  }

  CFIndex get_integer_value(void) const {
    return integer_value_;
  }

  std::optional<hid_usage_page> get_hid_usage_page(void) const {
    return hid_usage_page_;
  }

  std::optional<hid_usage> get_hid_usage(void) const {
    return hid_usage_;
  }

  bool conforms_to(hid_usage_page hid_usage_page,
                   hid_usage hid_usage) const {
    return hid_usage_page_ == hid_usage_page &&
           hid_usage_ == hid_usage;
  }

  bool operator==(const hid_value& other) const {
    return time_stamp_ == other.time_stamp_ &&
           integer_value_ == other.integer_value_ &&
           hid_usage_page_ == other.hid_usage_page_ &&
           hid_usage_ == other.hid_usage_;
  }

private:
  absolute_time_point time_stamp_;
  CFIndex integer_value_;
  std::optional<hid_usage_page> hid_usage_page_;
  std::optional<hid_usage> hid_usage_;
};
} // namespace krbn
