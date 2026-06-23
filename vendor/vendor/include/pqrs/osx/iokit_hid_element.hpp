#pragma once

// pqrs::osx::iokit_hid_element v1.4.0

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "iokit_hid_element/iokit_hid_element_type.hpp"
#include <IOKit/hid/IOHIDElement.h>
#include <optional>
#include <pqrs/cf/cf_ptr.hpp>
#include <pqrs/cf/string.hpp>
#include <pqrs/hid.hpp>
#include <utility>

namespace pqrs::osx {
class iokit_hid_element final {
public:
  iokit_hid_element() noexcept
      : iokit_hid_element(nullptr) {
  }

  iokit_hid_element(IOHIDElementRef element) noexcept
      : element_(element) {
  }

  iokit_hid_element(cf::cf_ptr<IOHIDElementRef> element) noexcept
      : element_(std::move(element)) {
  }

  ~iokit_hid_element() = default;

  [[nodiscard]] cf::cf_ptr<IOHIDElementRef> get_cf_ptr() const noexcept {
    return element_;
  }

  [[nodiscard]] IOHIDElementRef get_raw_ptr() const noexcept {
    if (element_) {
      return *element_;
    }
    return nullptr;
  }

  [[nodiscard]] std::optional<CFIndex> get_logical_max() const noexcept {
    if (element_) {
      return IOHIDElementGetLogicalMax(*element_);
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<CFIndex> get_logical_min() const noexcept {
    if (element_) {
      return IOHIDElementGetLogicalMin(*element_);
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<std::string> get_name() const {
    if (element_) {
      return cf::make_string(IOHIDElementGetName(*element_));
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<CFIndex> get_physical_max() const noexcept {
    if (element_) {
      return IOHIDElementGetPhysicalMax(*element_);
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<CFIndex> get_physical_min() const noexcept {
    if (element_) {
      return IOHIDElementGetPhysicalMin(*element_);
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<uint32_t> get_report_count() const noexcept {
    if (element_) {
      return IOHIDElementGetReportCount(*element_);
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<uint32_t> get_report_id() const noexcept {
    if (element_) {
      return IOHIDElementGetReportID(*element_);
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<uint32_t> get_report_size() const noexcept {
    if (element_) {
      return IOHIDElementGetReportSize(*element_);
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<iokit_hid_element_type> get_type() const noexcept {
    if (element_) {
      return iokit_hid_element_type(IOHIDElementGetType(*element_));
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<uint32_t> get_unit() const noexcept {
    if (element_) {
      return IOHIDElementGetUnit(*element_);
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<uint32_t> get_unit_exponent() const noexcept {
    if (element_) {
      return IOHIDElementGetUnitExponent(*element_);
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<hid::usage_page::value_t> get_usage_page() const noexcept {
    if (element_) {
      return hid::usage_page::value_t(IOHIDElementGetUsagePage(*element_));
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<hid::usage::value_t> get_usage() const noexcept {
    if (element_) {
      return hid::usage::value_t(IOHIDElementGetUsage(*element_));
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<bool> has_null_state() const noexcept {
    if (element_) {
      return IOHIDElementHasNullState(*element_);
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<bool> has_preferred_state() const noexcept {
    if (element_) {
      return IOHIDElementHasPreferredState(*element_);
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<bool> is_array() const noexcept {
    if (element_) {
      return IOHIDElementIsArray(*element_);
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<bool> is_non_linear() const noexcept {
    if (element_) {
      return IOHIDElementIsNonLinear(*element_);
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<bool> is_relative() const noexcept {
    if (element_) {
      return IOHIDElementIsRelative(*element_);
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<bool> is_virtual() const noexcept {
    if (element_) {
      return IOHIDElementIsVirtual(*element_);
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<bool> is_wrapping() const noexcept {
    if (element_) {
      return IOHIDElementIsWrapping(*element_);
    }
    return std::nullopt;
  }

  [[nodiscard]] operator bool() const noexcept {
    return element_;
  }

private:
  cf::cf_ptr<IOHIDElementRef> element_;
};
} // namespace pqrs::osx
