#pragma once

// pqrs::osx::iokit_hid_element v1.3

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include "iokit_hid_element/iokit_hid_element_type.hpp"
#include <IOKit/hid/IOHIDElement.h>
#include <optional>
#include <pqrs/cf/cf_ptr.hpp>
#include <pqrs/cf/string.hpp>
#include <pqrs/hid.hpp>

namespace pqrs {
namespace osx {
class iokit_hid_element final {
public:
  iokit_hid_element(void) : iokit_hid_element(nullptr) {
  }

  iokit_hid_element(IOHIDElementRef element) : element_(element) {
  }

  virtual ~iokit_hid_element(void) {
  }

  cf::cf_ptr<IOHIDElementRef> get_cf_ptr(void) const {
    return element_;
  }

  IOHIDElementRef get_raw_ptr(void) const {
    if (element_) {
      return *element_;
    }
    return nullptr;
  }

  std::optional<CFIndex> get_logical_max(void) const {
    if (element_) {
      return IOHIDElementGetLogicalMax(*element_);
    }
    return std::nullopt;
  }

  std::optional<CFIndex> get_logical_min(void) const {
    if (element_) {
      return IOHIDElementGetLogicalMin(*element_);
    }
    return std::nullopt;
  }

  std::optional<std::string> get_name(void) const {
    if (element_) {
      return cf::make_string(IOHIDElementGetName(*element_));
    }
    return std::nullopt;
  }

  std::optional<CFIndex> get_physical_max(void) const {
    if (element_) {
      return IOHIDElementGetPhysicalMax(*element_);
    }
    return std::nullopt;
  }

  std::optional<CFIndex> get_physical_min(void) const {
    if (element_) {
      return IOHIDElementGetPhysicalMin(*element_);
    }
    return std::nullopt;
  }

  std::optional<uint32_t> get_report_count(void) const {
    if (element_) {
      return IOHIDElementGetReportCount(*element_);
    }
    return std::nullopt;
  }

  std::optional<uint32_t> get_report_id(void) const {
    if (element_) {
      return IOHIDElementGetReportID(*element_);
    }
    return std::nullopt;
  }

  std::optional<uint32_t> get_report_size(void) const {
    if (element_) {
      return IOHIDElementGetReportSize(*element_);
    }
    return std::nullopt;
  }

  std::optional<iokit_hid_element_type> get_type(void) const {
    if (element_) {
      return iokit_hid_element_type(IOHIDElementGetType(*element_));
    }
    return std::nullopt;
  }

  std::optional<uint32_t> get_unit(void) const {
    if (element_) {
      return IOHIDElementGetUnit(*element_);
    }
    return std::nullopt;
  }

  std::optional<uint32_t> get_unit_exponent(void) const {
    if (element_) {
      return IOHIDElementGetUnitExponent(*element_);
    }
    return std::nullopt;
  }

  std::optional<hid::usage_page::value_t> get_usage_page(void) const {
    if (element_) {
      return hid::usage_page::value_t(IOHIDElementGetUsagePage(*element_));
    }
    return std::nullopt;
  }

  std::optional<hid::usage::value_t> get_usage(void) const {
    if (element_) {
      return hid::usage::value_t(IOHIDElementGetUsage(*element_));
    }
    return std::nullopt;
  }

  std::optional<bool> has_null_state(void) const {
    if (element_) {
      return IOHIDElementHasNullState(*element_);
    }
    return std::nullopt;
  }

  std::optional<bool> has_preferred_state(void) const {
    if (element_) {
      return IOHIDElementHasPreferredState(*element_);
    }
    return std::nullopt;
  }

  std::optional<bool> is_array(void) const {
    if (element_) {
      return IOHIDElementIsArray(*element_);
    }
    return std::nullopt;
  }

  std::optional<bool> is_non_linear(void) const {
    if (element_) {
      return IOHIDElementIsNonLinear(*element_);
    }
    return std::nullopt;
  }

  std::optional<bool> is_relative(void) const {
    if (element_) {
      return IOHIDElementIsRelative(*element_);
    }
    return std::nullopt;
  }

  std::optional<bool> is_virtual(void) const {
    if (element_) {
      return IOHIDElementIsVirtual(*element_);
    }
    return std::nullopt;
  }

  std::optional<bool> is_wrapping(void) const {
    if (element_) {
      return IOHIDElementIsWrapping(*element_);
    }
    return std::nullopt;
  }

  operator bool(void) const {
    return element_;
  }

private:
  cf::cf_ptr<IOHIDElementRef> element_;
};
} // namespace osx
} // namespace pqrs
