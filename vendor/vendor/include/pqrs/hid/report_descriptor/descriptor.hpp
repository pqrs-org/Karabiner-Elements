#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "../report_id.hpp"
#include "../usage_pair.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace pqrs::hid::report_descriptor {
enum class report_type {
  input,
  output,
  feature,
};

enum class report_field_flag : uint32_t {
  constant = 1U << 0,
  variable = 1U << 1,
  relative = 1U << 2,
  wrap = 1U << 3,
  non_linear = 1U << 4,
  no_preferred_state = 1U << 5,
  null_state = 1U << 6,
  // Bit 7 is Volatile for Output and Feature, but reserved for Input.
  volatile_data = 1U << 7,
  buffered_bytes = 1U << 8,
};

// Main item flags are parsed before a report_field exists, so keep the bitmask
// operation in one helper that can be shared by the parser and public API.
[[nodiscard]] constexpr bool has_flag(uint32_t flags,
                                      report_field_flag flag) noexcept {
  return (flags & std::to_underlying(flag)) != 0;
}

class usage_set final {
public:
  usage_set(std::vector<usage_pair> usages,
            std::optional<usage_pair> usage_minimum,
            std::optional<usage_pair> usage_maximum) noexcept
      : usages_(std::move(usages)),
        usage_minimum_(usage_minimum),
        usage_maximum_(usage_maximum) {
  }

  [[nodiscard]] const std::vector<usage_pair>& get_usages() const noexcept {
    return usages_;
  }

  [[nodiscard]] const std::optional<usage_pair>& get_usage_minimum() const noexcept {
    return usage_minimum_;
  }

  [[nodiscard]] const std::optional<usage_pair>& get_usage_maximum() const noexcept {
    return usage_maximum_;
  }

private:
  std::vector<usage_pair> usages_;
  std::optional<usage_pair> usage_minimum_;
  std::optional<usage_pair> usage_maximum_;
};

class collection final {
public:
  collection(uint8_t type,
             std::optional<usage_pair> usage,
             std::vector<usage_set> usage_sets) noexcept
      : type_(type),
        usage_(usage),
        usage_sets_(std::move(usage_sets)) {
  }

  [[nodiscard]] uint8_t get_type() const noexcept {
    return type_;
  }

  [[nodiscard]] const std::optional<usage_pair>& get_usage() const noexcept {
    return usage_;
  }

  // A delimiter set assigns several alternative usages to one control. The
  // first explicit usage, or the range minimum, is returned by get_usage().
  [[nodiscard]] const std::vector<usage_set>& get_usage_sets() const noexcept {
    return usage_sets_;
  }

private:
  uint8_t type_;
  std::optional<usage_pair> usage_;
  std::vector<usage_set> usage_sets_;
};

class report_field final {
public:
  report_field(report_type report_type,
               report_id::value_t report_id,
               size_t bit_offset,
               uint32_t size_bits,
               uint32_t count,
               uint32_t flags,
               usage_page::value_t usage_page,
               std::vector<usage_pair> usages,
               std::vector<usage_set> usage_sets,
               std::optional<usage_pair> usage_minimum,
               std::optional<usage_pair> usage_maximum,
               std::vector<uint32_t> designator_indices,
               std::optional<uint32_t> designator_minimum,
               std::optional<uint32_t> designator_maximum,
               std::vector<uint32_t> string_indices,
               std::optional<uint32_t> string_minimum,
               std::optional<uint32_t> string_maximum,
               int32_t logical_minimum,
               int64_t logical_maximum,
               int32_t physical_minimum,
               int64_t physical_maximum,
               int32_t unit_exponent,
               uint32_t unit,
               std::vector<collection> collection_path) noexcept
      : report_type_(report_type),
        report_id_(report_id),
        bit_offset_(bit_offset),
        size_bits_(size_bits),
        count_(count),
        flags_(flags),
        usage_page_(usage_page),
        usages_(std::move(usages)),
        usage_sets_(std::move(usage_sets)),
        usage_minimum_(usage_minimum),
        usage_maximum_(usage_maximum),
        designator_indices_(std::move(designator_indices)),
        designator_minimum_(designator_minimum),
        designator_maximum_(designator_maximum),
        string_indices_(std::move(string_indices)),
        string_minimum_(string_minimum),
        string_maximum_(string_maximum),
        logical_minimum_(logical_minimum),
        logical_maximum_(logical_maximum),
        physical_minimum_(physical_minimum),
        physical_maximum_(physical_maximum),
        unit_exponent_(unit_exponent),
        unit_(unit),
        collection_path_(std::move(collection_path)) {
  }

  [[nodiscard]] report_type get_report_type() const noexcept {
    return report_type_;
  }

  [[nodiscard]] report_id::value_t get_report_id() const noexcept {
    return report_id_;
  }

  // The offset is measured from the beginning of the report payload and does not
  // include the report ID byte that some transports place at the start of a buffer.
  [[nodiscard]] size_t get_bit_offset() const noexcept {
    return bit_offset_;
  }

  [[nodiscard]] uint32_t get_size_bits() const noexcept {
    return size_bits_;
  }

  [[nodiscard]] uint32_t get_count() const noexcept {
    return count_;
  }

  // Keep the raw value available so reserved and future HID bits are not lost.
  [[nodiscard]] uint32_t get_raw_flags() const noexcept {
    return flags_;
  }

  [[nodiscard]] bool has_flag(report_field_flag flag) const noexcept {
    return report_descriptor::has_flag(flags_, flag);
  }

  [[nodiscard]] usage_page::value_t get_usage_page() const noexcept {
    return usage_page_;
  }

  [[nodiscard]] const std::vector<usage_pair>& get_usages() const noexcept {
    return usages_;
  }

  // Each usage_set describes one control's alternatives in preference order.
  // Non-delimited Usage items therefore appear as singleton sets.
  [[nodiscard]] const std::vector<usage_set>& get_usage_sets() const noexcept {
    return usage_sets_;
  }

  [[nodiscard]] const std::optional<usage_pair>& get_usage_minimum() const noexcept {
    return usage_minimum_;
  }

  [[nodiscard]] const std::optional<usage_pair>& get_usage_maximum() const noexcept {
    return usage_maximum_;
  }

  // These Local items associate the field with optional Physical and String
  // descriptors. Keep the individual item forms because an Index and a
  // Minimum/Maximum range have different assignment semantics in HID.
  [[nodiscard]] const std::vector<uint32_t>& get_designator_indices() const noexcept {
    return designator_indices_;
  }

  [[nodiscard]] const std::optional<uint32_t>& get_designator_minimum() const noexcept {
    return designator_minimum_;
  }

  [[nodiscard]] const std::optional<uint32_t>& get_designator_maximum() const noexcept {
    return designator_maximum_;
  }

  [[nodiscard]] const std::vector<uint32_t>& get_string_indices() const noexcept {
    return string_indices_;
  }

  [[nodiscard]] const std::optional<uint32_t>& get_string_minimum() const noexcept {
    return string_minimum_;
  }

  [[nodiscard]] const std::optional<uint32_t>& get_string_maximum() const noexcept {
    return string_maximum_;
  }

  [[nodiscard]] int32_t get_logical_minimum() const noexcept {
    return logical_minimum_;
  }

  [[nodiscard]] int64_t get_logical_maximum() const noexcept {
    return logical_maximum_;
  }

  [[nodiscard]] int32_t get_physical_minimum() const noexcept {
    return physical_minimum_;
  }

  [[nodiscard]] int64_t get_physical_maximum() const noexcept {
    return physical_maximum_;
  }

  [[nodiscard]] int32_t get_unit_exponent() const noexcept {
    return unit_exponent_;
  }

  [[nodiscard]] uint32_t get_unit() const noexcept {
    return unit_;
  }

  [[nodiscard]] const std::vector<collection>& get_collection_path() const noexcept {
    return collection_path_;
  }

private:
  report_type report_type_;
  report_id::value_t report_id_;
  size_t bit_offset_;
  uint32_t size_bits_;
  uint32_t count_;
  uint32_t flags_;
  usage_page::value_t usage_page_;
  std::vector<usage_pair> usages_;
  std::vector<usage_set> usage_sets_;
  std::optional<usage_pair> usage_minimum_;
  std::optional<usage_pair> usage_maximum_;
  std::vector<uint32_t> designator_indices_;
  std::optional<uint32_t> designator_minimum_;
  std::optional<uint32_t> designator_maximum_;
  std::vector<uint32_t> string_indices_;
  std::optional<uint32_t> string_minimum_;
  std::optional<uint32_t> string_maximum_;
  int32_t logical_minimum_;
  int64_t logical_maximum_;
  int32_t physical_minimum_;
  int64_t physical_maximum_;
  int32_t unit_exponent_;
  uint32_t unit_;
  std::vector<collection> collection_path_;
};

class descriptor final {
public:
  explicit descriptor(std::vector<report_field> report_fields) noexcept
      : report_fields_(std::move(report_fields)) {
  }

  [[nodiscard]] const std::vector<report_field>& get_report_fields() const noexcept {
    return report_fields_;
  }

  [[nodiscard]] std::vector<const report_field*> find_report_fields(
      report_type report_type,
      report_id::value_t report_id) const {
    std::vector<const report_field*> result;

    for (const auto& field : report_fields_) {
      if (field.get_report_type() == report_type &&
          field.get_report_id() == report_id) {
        result.push_back(&field);
      }
    }

    return result;
  }

private:
  std::vector<report_field> report_fields_;
};
} // namespace pqrs::hid::report_descriptor
