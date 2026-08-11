#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "descriptor.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <limits>
#include <map>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace pqrs::hid::report_descriptor {
enum class parse_error_code {
  unexpected_end_of_descriptor,
  unknown_or_reserved_item,
  invalid_item_data_size,
  invalid_usage_page,
  missing_required_global_item,
  missing_usage,
  invalid_report_id,
  report_id_declared_too_late,
  report_id_spans_top_level_application_collection,
  invalid_report_size_or_count,
  report_size_overflow,
  global_state_stack_underflow,
  collection_stack_underflow,
  application_collection_not_top_level,
  top_level_collection_not_application,
  invalid_delimiter,
  invalid_usage_range,
  invalid_designator_range,
  invalid_string_range,
  incomplete_physical_extents,
  invalid_extents,
  logical_range_exceeds_report_size,
  report_field_spans_more_than_four_bytes,
  buffered_bytes_not_byte_aligned,
  unclosed_delimiter,
  unclosed_global_state,
  unclosed_collection,
};

class parse_error final {
public:
  parse_error(parse_error_code code,
              size_t byte_offset) noexcept
      : code_(code),
        byte_offset_(byte_offset) {
  }

  [[nodiscard]] parse_error_code get_code() const noexcept {
    return code_;
  }

  [[nodiscard]] size_t get_byte_offset() const noexcept {
    return byte_offset_;
  }

private:
  parse_error_code code_;
  size_t byte_offset_;
};

enum class validation_mode {
  // Real-world HID parsers commonly accept descriptors with recoverable
  // specification violations. Keep their original metadata and report those
  // violations to the caller instead of silently repairing it.
  permissive,
  // Strict mode is intended for descriptor validation and generation tests.
  // The first recoverable specification violation becomes a parse error.
  strict,
};

class parse_diagnostic final {
public:
  parse_diagnostic(parse_error_code code,
                   size_t byte_offset) noexcept
      : code_(code),
        byte_offset_(byte_offset) {
  }

  [[nodiscard]] parse_error_code get_code() const noexcept {
    return code_;
  }

  [[nodiscard]] size_t get_byte_offset() const noexcept {
    return byte_offset_;
  }

private:
  parse_error_code code_;
  size_t byte_offset_;
};

class parse_result final {
public:
  parse_result(descriptor descriptor,
               std::vector<parse_diagnostic> diagnostics) noexcept
      : descriptor_(std::move(descriptor)),
        diagnostics_(std::move(diagnostics)) {
  }

  [[nodiscard]] const descriptor& get_descriptor() const noexcept {
    return descriptor_;
  }

  [[nodiscard]] const std::vector<parse_diagnostic>& get_diagnostics() const noexcept {
    return diagnostics_;
  }

private:
  descriptor descriptor_;
  std::vector<parse_diagnostic> diagnostics_;
};

namespace impl {
// These values are encoded directly in a HID short-item prefix. Keeping them as
// enums makes the parser follow the specification's item names instead of tags.
enum class item_type : uint8_t {
  main = 0,
  global = 1,
  local = 2,
  reserved = 3,
};

enum class main_item_tag : uint8_t {
  input = 8,
  output = 9,
  collection = 10,
  feature = 11,
  end_collection = 12,
};

enum class global_item_tag : uint8_t {
  usage_page = 0,
  logical_minimum = 1,
  logical_maximum = 2,
  physical_minimum = 3,
  physical_maximum = 4,
  unit_exponent = 5,
  unit = 6,
  report_size = 7,
  report_id = 8,
  report_count = 9,
  push = 10,
  pop = 11,
};

enum class local_item_tag : uint8_t {
  usage = 0,
  usage_minimum = 1,
  usage_maximum = 2,
  designator_index = 3,
  designator_minimum = 4,
  designator_maximum = 5,
  string_index = 7,
  string_minimum = 8,
  string_maximum = 9,
  delimiter = 10,
};

enum class delimiter_value : uint32_t {
  close = 0,
  open = 1,
};

enum class collection_type : uint8_t {
  application = 1,
};

class maximum_value final {
public:
  maximum_value() noexcept
      : maximum_value(0, 0) {
  }

  maximum_value(uint32_t unsigned_value,
                int32_t signed_value) noexcept
      : unsigned_value_(unsigned_value),
        signed_value_(signed_value) {
  }

  [[nodiscard]] int64_t resolve(int32_t minimum) const noexcept {
    // The sign of a HID maximum is determined by its corresponding minimum,
    // which may legally be declared later in the descriptor.
    return minimum < 0
               ? signed_value_
               : static_cast<int64_t>(unsigned_value_);
  }

private:
  // These two interpretations are fixed at construction. Keeping them private
  // prevents either value from being changed independently while preserving
  // assignment of the complete value object for HID Global Push/Pop.
  uint32_t unsigned_value_;
  int32_t signed_value_;
};

struct global_state final {
  uint32_t usage_page = 0;
  int32_t logical_minimum = 0;
  bool logical_minimum_defined = false;
  maximum_value logical_maximum;
  bool logical_maximum_defined = false;
  // Undefined physical extents have different semantics from explicit values:
  // the HID specification makes them inherit the logical extents.
  std::optional<int32_t> physical_minimum;
  std::optional<maximum_value> physical_maximum;
  int32_t unit_exponent = 0;
  uint32_t unit = 0;
  uint32_t report_size = 0;
  bool report_size_defined = false;
  uint8_t report_id = 0;
  uint32_t report_count = 0;
};

struct local_usage final {
  uint32_t value;
  size_t value_size;
};

struct local_usage_set final {
  std::vector<local_usage> usages;
  std::optional<local_usage> usage_minimum;
  std::optional<local_usage> usage_maximum;
};

struct local_state final {
  std::vector<local_usage> usages;
  std::optional<local_usage> usage_minimum;
  std::optional<local_usage> usage_maximum;
  std::vector<uint32_t> designator_indices;
  std::optional<uint32_t> designator_minimum;
  std::optional<uint32_t> designator_maximum;
  std::vector<uint32_t> string_indices;
  std::optional<uint32_t> string_minimum;
  std::optional<uint32_t> string_maximum;
  std::vector<local_usage_set> delimiter_usage_sets;
  bool delimiter_open = false;

  void clear() {
    usages.clear();
    usage_minimum = std::nullopt;
    usage_maximum = std::nullopt;
    designator_indices.clear();
    designator_minimum = std::nullopt;
    designator_maximum = std::nullopt;
    string_indices.clear();
    string_minimum = std::nullopt;
    string_maximum = std::nullopt;
    delimiter_usage_sets.clear();
    delimiter_open = false;
  }
};

struct resolved_local_state final {
  // usages contains one preferred usage per control, while usage_sets retains
  // every alternative declared by Delimiter items.
  std::vector<usage_pair> usages;
  std::vector<usage_set> usage_sets;
  std::optional<usage_pair> usage_minimum;
  std::optional<usage_pair> usage_maximum;
};

struct extents final {
  int32_t minimum;
  int64_t maximum;
};

[[nodiscard]] inline bool valid_extents(const extents& value) noexcept {
  return value.minimum <= value.maximum;
}

[[nodiscard]] inline bool valid_index_range(
    const std::optional<uint32_t>& minimum,
    const std::optional<uint32_t>& maximum) noexcept {
  return minimum.has_value() == maximum.has_value() &&
         (!minimum || *minimum <= *maximum);
}

[[nodiscard]] inline bool logical_range_fits_report_size(
    const extents& logical_extents,
    uint32_t report_size,
    uint32_t flags) noexcept {
  if (!valid_extents(logical_extents)) {
    return false;
  }

  // Report Size is required, but the parser error-checking specification does
  // not prohibit zero. A zero-bit field has only the value zero and therefore
  // cannot also reserve a distinct Null State value.
  if (report_size == 0) {
    return logical_extents.minimum == 0 &&
           logical_extents.maximum == 0 &&
           !has_flag(flags, report_field_flag::null_state);
  }

  // HID fields are signed when either logical bound is negative. The parsed
  // extents themselves are at most 32 bits, so wider fields always fit.
  if (logical_extents.minimum < 0) {
    if (report_size < 32) {
      auto magnitude = int64_t{1} << (report_size - 1);
      if (logical_extents.minimum < -magnitude ||
          logical_extents.maximum >= magnitude) {
        return false;
      }
    }
  } else if (report_size < 32) {
    auto maximum = (uint64_t{1} << report_size) - 1;
    if (static_cast<uint64_t>(logical_extents.maximum) > maximum) {
      return false;
    }
  }

  if (has_flag(flags, report_field_flag::null_state)) {
    // Null State needs at least one representable value outside the declared
    // logical range. Extents are 32-bit, so fields wider than 32 bits have room.
    if (report_size <= 32) {
      auto capacity = uint64_t{1} << report_size;
      auto range_size = static_cast<uint64_t>(logical_extents.maximum -
                                              logical_extents.minimum) +
                        1;
      if (range_size >= capacity) {
        return false;
      }
    }
  }

  return true;
}

[[nodiscard]] inline bool report_elements_fit_four_bytes(
    size_t bit_offset,
    uint32_t report_size,
    uint32_t report_count) noexcept {
  if (report_size > 32) {
    return false;
  }

  // Only the starting bit within a byte affects how many bytes an element
  // spans. Its sequence repeats within eight elements for every Report Size.
  auto start_bit = static_cast<uint32_t>(bit_offset % 8);
  auto iterations = std::min(report_count, uint32_t{8});
  for (uint32_t i = 0; i < iterations; ++i) {
    if (start_bit + report_size > 32) {
      return false;
    }
    start_bit = (start_bit + report_size) % 8;
  }

  return true;
}

struct report_offset_key final {
  report_type type;
  uint8_t report_id;

  auto operator<=>(const report_offset_key&) const = default;
};

[[nodiscard]] inline uint32_t make_unsigned_value(std::span<const uint8_t> bytes) noexcept {
  uint32_t result = 0;

  // HID short-item data is little-endian. Assemble it explicitly so parsing is
  // independent of the host byte order and does not require unaligned loads.
  for (size_t i = 0; i < bytes.size(); ++i) {
    result |= static_cast<uint32_t>(bytes[i]) << (i * 8);
  }

  return result;
}

[[nodiscard]] inline int32_t make_signed_value(std::span<const uint8_t> bytes) noexcept {
  auto value = make_unsigned_value(bytes);

  // Item payloads may be 1, 2, or 4 bytes. Extend the payload's own sign bit to
  // 32 bits before conversion; converting the zero-extended value directly
  // would turn, for example, a one-byte 0xff into positive 255 instead of -1.
  if (!bytes.empty() && bytes.size() < sizeof(value)) {
    auto sign_bit = static_cast<uint32_t>(1U << (bytes.size() * 8 - 1));
    if (value & sign_bit) {
      value |= std::numeric_limits<uint32_t>::max() << (bytes.size() * 8);
    }
  }

  return static_cast<int32_t>(value);
}

// Unit Exponent uses a signed four-bit code even though the containing short
// item commonly has a one-byte data field (HID 1.11 section 6.2.2.7).
[[nodiscard]] inline int32_t make_unit_exponent(uint32_t value) noexcept {
  auto result = static_cast<int32_t>(value & 0xf);
  if ((result & 0x8) != 0) {
    result -= 0x10;
  }
  return result;
}

[[nodiscard]] inline usage_pair make_usage_pair(uint32_t value,
                                                size_t value_size,
                                                uint32_t global_usage_page) noexcept {
  auto page = value_size == 4 ? value >> 16 : global_usage_page;
  auto usage = value_size == 4 ? value & 0xffff : value;

  return usage_pair(usage_page::value_t(static_cast<int32_t>(page)),
                    usage::value_t(static_cast<int32_t>(usage)));
}

[[nodiscard]] inline usage_pair make_usage_pair(const local_usage& value,
                                                uint32_t global_usage_page) noexcept {
  return make_usage_pair(value.value,
                         value.value_size,
                         global_usage_page);
}

[[nodiscard]] inline bool valid_usage_range(
    const std::optional<local_usage>& minimum,
    const std::optional<local_usage>& maximum,
    uint32_t global_usage_page) noexcept {
  // A range is either absent or a complete pair. HID also requires both ends
  // to use the extended form together so their embedded Usage Pages agree.
  if (minimum.has_value() != maximum.has_value()) {
    return false;
  }
  if (!minimum) {
    return true;
  }
  if ((minimum->value_size == 4) != (maximum->value_size == 4)) {
    return false;
  }

  auto resolved_minimum = make_usage_pair(*minimum, global_usage_page);
  auto resolved_maximum = make_usage_pair(*maximum, global_usage_page);
  return resolved_minimum.get_usage_page() == resolved_maximum.get_usage_page() &&
         resolved_minimum.get_usage() <= resolved_maximum.get_usage();
}

[[nodiscard]] inline bool valid_usage_ranges(
    const local_state& local,
    uint32_t global_usage_page) noexcept {
  if (!valid_usage_range(local.usage_minimum,
                         local.usage_maximum,
                         global_usage_page)) {
    return false;
  }

  for (const auto& set : local.delimiter_usage_sets) {
    if (!valid_usage_range(set.usage_minimum,
                           set.usage_maximum,
                           global_usage_page)) {
      return false;
    }
  }

  return true;
}

[[nodiscard]] inline bool has_usage(const local_state& local) noexcept {
  if (!local.usages.empty() || local.usage_minimum) {
    return true;
  }

  for (const auto& set : local.delimiter_usage_sets) {
    if (!set.usages.empty() || set.usage_minimum) {
      return true;
    }
  }

  return false;
}

[[nodiscard]] inline resolved_local_state resolve_local_state(
    const local_state& local,
    uint32_t global_usage_page) {
  resolved_local_state result;

  // Short Usage items are intentionally resolved here rather than when read.
  // HID binds them to the last Usage Page in effect at the following Main item.
  for (const auto& usage : local.usages) {
    auto resolved = make_usage_pair(usage, global_usage_page);
    result.usages.push_back(resolved);
    result.usage_sets.emplace_back(std::vector<usage_pair>{resolved},
                                   std::nullopt,
                                   std::nullopt);
  }

  for (const auto& set : local.delimiter_usage_sets) {
    std::vector<usage_pair> resolved_usages;
    for (const auto& usage : set.usages) {
      resolved_usages.push_back(make_usage_pair(usage, global_usage_page));
    }

    std::optional<usage_pair> resolved_minimum;
    std::optional<usage_pair> resolved_maximum;
    if (set.usage_minimum) {
      resolved_minimum = make_usage_pair(*set.usage_minimum,
                                         global_usage_page);
    }
    if (set.usage_maximum) {
      resolved_maximum = make_usage_pair(*set.usage_maximum,
                                         global_usage_page);
    }

    // Empty delimiter sets carry no usage information, but keeping them would
    // make the control-to-set correspondence ambiguous to callers.
    if (!resolved_usages.empty() || resolved_minimum || resolved_maximum) {
      if (!resolved_usages.empty()) {
        result.usages.push_back(resolved_usages.front());
      } else if (resolved_minimum) {
        result.usages.push_back(*resolved_minimum);
      }

      result.usage_sets.emplace_back(std::move(resolved_usages),
                                     resolved_minimum,
                                     resolved_maximum);
    }
  }

  if (local.usage_minimum) {
    result.usage_minimum = make_usage_pair(*local.usage_minimum,
                                           global_usage_page);
  }
  if (local.usage_maximum) {
    result.usage_maximum = make_usage_pair(*local.usage_maximum,
                                           global_usage_page);
  }

  return result;
}

[[nodiscard]] inline extents resolve_logical_extents(
    const global_state& global) noexcept {
  return {
      .minimum = global.logical_minimum,
      .maximum = global.logical_maximum.resolve(global.logical_minimum),
  };
}

[[nodiscard]] inline extents resolve_physical_extents(
    const global_state& global,
    const extents& logical_extents) noexcept {
  // HID treats either missing bound, as well as an explicit zero pair, as a
  // request to use the logical bounds for physical-unit calculations.
  if (!global.physical_minimum ||
      !global.physical_maximum ||
      (*global.physical_minimum == 0 &&
       global.physical_maximum->resolve(*global.physical_minimum) == 0)) {
    return logical_extents;
  }

  return {
      .minimum = *global.physical_minimum,
      .maximum = global.physical_maximum->resolve(*global.physical_minimum),
  };
}
} // namespace impl

[[nodiscard]] inline std::expected<parse_result, parse_error> parse(
    std::span<const uint8_t> bytes,
    validation_mode mode = validation_mode::permissive) {
  impl::global_state global;
  impl::local_state local;
  std::vector<impl::global_state> global_stack;
  std::vector<collection> collection_path;
  std::map<impl::report_offset_key, size_t> report_offsets;
  std::vector<report_field> report_fields;
  bool report_main_item_seen = false;
  bool report_id_declared = false;
  size_t next_application_generation = 0;
  std::optional<size_t> current_application_generation;
  std::map<impl::report_offset_key, size_t> report_application_owners;
  std::vector<parse_diagnostic> diagnostics;

  // Only validation findings come through this helper. Errors that prevent us
  // from determining item boundaries, collection state, or report layout stay
  // fatal in both modes and return directly from the parser.
  auto report_validation_failure = [&](parse_error_code code,
                                       size_t byte_offset) -> std::optional<parse_error> {
    if (mode == validation_mode::strict) {
      return parse_error(code, byte_offset);
    }

    diagnostics.emplace_back(code, byte_offset);
    return std::nullopt;
  };

  size_t offset = 0;

  while (offset < bytes.size()) {
    auto item_offset = offset;
    auto prefix = bytes[offset++];

    // HID 1.11 defines the long-item envelope but reserves every long-item tag.
    // Consume the envelope first so truncation is still reported precisely.
    if (prefix == 0xfe) {
      // A delimiter may contain only Usage, Usage Minimum, and Usage Maximum
      // local items, so a long item cannot appear before the matching Close.
      if (local.delimiter_open) {
        return std::unexpected(parse_error(parse_error_code::invalid_delimiter,
                                           item_offset));
      }

      if (bytes.size() - offset < 2) {
        return std::unexpected(parse_error(parse_error_code::unexpected_end_of_descriptor,
                                           item_offset));
      }

      auto data_size = static_cast<size_t>(bytes[offset]);
      offset += 2; // Data size and long item tag.

      if (bytes.size() - offset < data_size) {
        return std::unexpected(parse_error(parse_error_code::unexpected_end_of_descriptor,
                                           item_offset));
      }

      offset += data_size;
      if (auto error = report_validation_failure(
              parse_error_code::unknown_or_reserved_item,
              item_offset)) {
        return std::unexpected(*error);
      }
      continue;
    }

    auto size_code = static_cast<uint8_t>(prefix & 0x3);
    auto data_size = static_cast<size_t>(size_code == 3 ? 4 : size_code);

    if (bytes.size() - offset < data_size) {
      return std::unexpected(parse_error(parse_error_code::unexpected_end_of_descriptor,
                                         item_offset));
    }

    auto data = bytes.subspan(offset, data_size);
    offset += data_size;

    auto unsigned_value = impl::make_unsigned_value(data);
    auto signed_value = impl::make_signed_value(data);
    auto item_type = static_cast<impl::item_type>((prefix >> 2) & 0x3);
    auto item_tag = static_cast<uint8_t>((prefix >> 4) & 0xf);

    switch (item_type) {
      case impl::item_type::main: {
        // A Main item inside a delimiter would consume an incomplete set and
        // make the following local items impossible to associate reliably.
        if (local.delimiter_open) {
          return std::unexpected(parse_error(parse_error_code::unclosed_delimiter,
                                             item_offset));
        }

        if (!impl::valid_usage_ranges(local, global.usage_page)) {
          if (auto error = report_validation_failure(
                  parse_error_code::invalid_usage_range,
                  item_offset)) {
            return std::unexpected(*error);
          }
        }
        if (!impl::valid_index_range(local.designator_minimum,
                                     local.designator_maximum)) {
          if (auto error = report_validation_failure(
                  parse_error_code::invalid_designator_range,
                  item_offset)) {
            return std::unexpected(*error);
          }
        }
        if (!impl::valid_index_range(local.string_minimum,
                                     local.string_maximum)) {
          if (auto error = report_validation_failure(
                  parse_error_code::invalid_string_range,
                  item_offset)) {
            return std::unexpected(*error);
          }
        }
        // Physical bounds form one global pair. The HID parser error table
        // requires checking correspondence at the next Main item, not only at
        // Input, Output, or Feature items that consume the numeric extents.
        if (global.physical_minimum.has_value() !=
            global.physical_maximum.has_value()) {
          if (auto error = report_validation_failure(
                  parse_error_code::incomplete_physical_extents,
                  item_offset)) {
            return std::unexpected(*error);
          }
        }

        auto resolved_local = impl::resolve_local_state(local,
                                                        global.usage_page);
        std::optional<report_type> field_report_type;

        switch (static_cast<impl::main_item_tag>(item_tag)) {
          case impl::main_item_tag::input:
            field_report_type = report_type::input;
            break;
          case impl::main_item_tag::output:
            field_report_type = report_type::output;
            break;
          case impl::main_item_tag::feature:
            field_report_type = report_type::feature;
            break;
          case impl::main_item_tag::collection: {
            auto type = static_cast<uint8_t>(unsigned_value);

            // HID 1.11 requires every top-level Collection to be an
            // Application Collection; other types may only refine one.
            if (collection_path.empty() &&
                type != std::to_underlying(impl::collection_type::application)) {
              if (auto error = report_validation_failure(
                      parse_error_code::top_level_collection_not_application,
                      item_offset)) {
                return std::unexpected(*error);
              }
            }

            if (type == std::to_underlying(impl::collection_type::application)) {
              // Application Collections identify independently routed devices,
              // so HID requires them to be top-level rather than nested.
              if (!collection_path.empty()) {
                if (auto error = report_validation_failure(
                        parse_error_code::application_collection_not_top_level,
                        item_offset)) {
                  return std::unexpected(*error);
                }
              }

              // Aliases are valid for other Collection types, but an
              // Application Collection must expose one unambiguous identity.
              if (!local.delimiter_usage_sets.empty()) {
                if (auto error = report_validation_failure(
                        parse_error_code::invalid_delimiter,
                        item_offset)) {
                  return std::unexpected(*error);
                }
              }

              // A nested Application Collection is retained in the collection
              // path in permissive mode, but it must not replace ownership of
              // the enclosing top-level report.
              if (collection_path.empty()) {
                current_application_generation = next_application_generation++;
              }
            }

            auto usage = resolved_local.usages.empty()
                             ? resolved_local.usage_minimum
                             : std::make_optional(resolved_local.usages.front());
            collection_path.emplace_back(type,
                                         usage,
                                         std::move(resolved_local.usage_sets));
            break;
          }
          case impl::main_item_tag::end_collection:
            if (collection_path.empty()) {
              return std::unexpected(parse_error(parse_error_code::collection_stack_underflow,
                                                 item_offset));
            }
            if (collection_path.size() == 1 &&
                collection_path.back().get_type() ==
                    std::to_underlying(impl::collection_type::application)) {
              current_application_generation = std::nullopt;
            }
            collection_path.pop_back();
            break;
          default:
            if (auto error = report_validation_failure(
                    parse_error_code::unknown_or_reserved_item,
                    item_offset)) {
              return std::unexpected(*error);
            }
            break;
        }

        if (field_report_type) {
          // The payload of Input, Output, and Feature Main items is the HID
          // field flag bitmask. Other Main item tags interpret it differently.
          auto flags = unsigned_value;

          // Once any Report ID is present, every data report needs its ID byte;
          // returning a report with the reserved ID 0 would misdescribe buffers.
          if (report_id_declared && global.report_id == 0) {
            return std::unexpected(parse_error(parse_error_code::invalid_report_id,
                                               item_offset));
          }

          auto key = impl::report_offset_key{
              .type = *field_report_type,
              .report_id = global.report_id,
          };

          if (current_application_generation) {
            // A data report cannot span Application Collections, including the
            // implicit ID 0 report. HID identifies a report by both its type
            // and ID, so equal numeric IDs of different types remain distinct.
            if (auto it = report_application_owners.find(key);
                it != report_application_owners.end() &&
                it->second != *current_application_generation) {
              if (auto error = report_validation_failure(
                      parse_error_code::report_id_spans_top_level_application_collection,
                      item_offset)) {
                return std::unexpected(*error);
              }
            } else {
              report_application_owners[key] = *current_application_generation;
            }
          }

          if (!global.report_size_defined || global.report_count == 0) {
            return std::unexpected(parse_error(parse_error_code::invalid_report_size_or_count,
                                               item_offset));
          }

          // Arrays use values as indices into one usage list, so alternative
          // usages grouped by Delimiter are only meaningful for Variable items.
          if (!has_flag(flags, report_field_flag::variable) &&
              !local.delimiter_usage_sets.empty()) {
            if (auto error = report_validation_failure(
                    parse_error_code::invalid_delimiter,
                    item_offset)) {
              return std::unexpected(*error);
            }
          }

          auto logical_extents = impl::resolve_logical_extents(global);

          auto physical_extents = impl::resolve_physical_extents(global,
                                                                 logical_extents);

          // Reversed logical or explicit physical bounds cannot describe a
          // valid HID field range. Strict mode rejects them, while permissive
          // mode preserves the original bounds and reports the violation.
          if (!impl::valid_extents(logical_extents) ||
              !impl::valid_extents(physical_extents)) {
            if (auto error = report_validation_failure(
                    parse_error_code::invalid_extents,
                    item_offset)) {
              return std::unexpected(*error);
            }
          }

          // These Global items are required to give every field a complete
          // numeric interpretation. A valid Usage Page is nonzero, so it also
          // records whether the item has been declared.
          if (global.usage_page == 0 ||
              !global.logical_minimum_defined ||
              !global.logical_maximum_defined) {
            if (auto error = report_validation_failure(
                    parse_error_code::missing_required_global_item,
                    item_offset)) {
              return std::unexpected(*error);
            }
          }

          // Constant fields without a Usage are padding. Data fields require a
          // Usage (an explicit item, range, or delimited set) to define meaning.
          if (!has_flag(flags, report_field_flag::constant) &&
              !impl::has_usage(local)) {
            if (auto error = report_validation_failure(
                    parse_error_code::missing_usage,
                    item_offset)) {
              return std::unexpected(*error);
            }
          }

          if (global.report_size != 0 &&
              global.report_count > std::numeric_limits<size_t>::max() / global.report_size) {
            return std::unexpected(parse_error(parse_error_code::report_size_overflow,
                                               item_offset));
          }

          auto field_size = static_cast<size_t>(global.report_size) * global.report_count;
          auto& bit_offset = report_offsets[key];
          // Report-size capacity is meaningful only for an ordered range. A
          // reversed range already has its own diagnostic and should not cause
          // a second, derivative finding in permissive mode.
          if (impl::valid_extents(logical_extents) &&
              !impl::logical_range_fits_report_size(logical_extents,
                                                    global.report_size,
                                                    flags)) {
            if (auto error = report_validation_failure(
                    parse_error_code::logical_range_exceeds_report_size,
                    item_offset)) {
              return std::unexpected(*error);
            }
          }

          if (!impl::report_elements_fit_four_bytes(bit_offset,
                                                    global.report_size,
                                                    global.report_count)) {
            // Section 8.4 limits each individual element, not the combined
            // Report Size * Report Count group, to at most four bytes.
            if (auto error = report_validation_failure(
                    parse_error_code::report_field_spans_more_than_four_bytes,
                    item_offset)) {
              return std::unexpected(*error);
            }
          }

          if (has_flag(flags, report_field_flag::buffered_bytes) &&
              (bit_offset % 8 != 0 || field_size % 8 != 0)) {
            // Buffered Bytes represents a byte stream, so both ends of its
            // report field must fall on byte boundaries.
            if (auto error = report_validation_failure(
                    parse_error_code::buffered_bytes_not_byte_aligned,
                    item_offset)) {
              return std::unexpected(*error);
            }
          }

          if (field_size > std::numeric_limits<size_t>::max() - bit_offset) {
            return std::unexpected(parse_error(parse_error_code::report_size_overflow,
                                               item_offset));
          }

          report_fields.emplace_back(*field_report_type,
                                     report_id::value_t(global.report_id),
                                     bit_offset,
                                     global.report_size,
                                     global.report_count,
                                     flags,
                                     usage_page::value_t(static_cast<int32_t>(global.usage_page)),
                                     std::move(resolved_local.usages),
                                     std::move(resolved_local.usage_sets),
                                     resolved_local.usage_minimum,
                                     resolved_local.usage_maximum,
                                     std::move(local.designator_indices),
                                     local.designator_minimum,
                                     local.designator_maximum,
                                     std::move(local.string_indices),
                                     local.string_minimum,
                                     local.string_maximum,
                                     logical_extents.minimum,
                                     logical_extents.maximum,
                                     physical_extents.minimum,
                                     physical_extents.maximum,
                                     global.unit_exponent,
                                     global.unit,
                                     collection_path);

          bit_offset += field_size;
          report_main_item_seen = true;
        }

        // Local items apply only to the next Main item, including Collection.
        local.clear();
        break;
      }

      case impl::item_type::global:
        if (local.delimiter_open) {
          return std::unexpected(parse_error(parse_error_code::invalid_delimiter,
                                             item_offset));
        }
        switch (static_cast<impl::global_item_tag>(item_tag)) {
          case impl::global_item_tag::usage_page:
            if (unsigned_value == 0 ||
                unsigned_value > std::numeric_limits<uint16_t>::max()) {
              if (auto error = report_validation_failure(
                      parse_error_code::invalid_usage_page,
                      item_offset)) {
                return std::unexpected(*error);
              }
              // An invalid page cannot be represented by usage_pair. Preserve
              // the last valid Global value and expose the skipped item through
              // the diagnostic instead of narrowing or clamping it.
              break;
            }
            global.usage_page = unsigned_value;
            break;
          case impl::global_item_tag::logical_minimum:
            global.logical_minimum = signed_value;
            global.logical_minimum_defined = true;
            break;
          case impl::global_item_tag::logical_maximum:
            global.logical_maximum = impl::maximum_value(unsigned_value,
                                                         signed_value);
            global.logical_maximum_defined = true;
            break;
          case impl::global_item_tag::physical_minimum:
            global.physical_minimum = signed_value;
            break;
          case impl::global_item_tag::physical_maximum:
            global.physical_maximum = impl::maximum_value(unsigned_value,
                                                          signed_value);
            break;
          case impl::global_item_tag::unit_exponent:
            global.unit_exponent = impl::make_unit_exponent(unsigned_value);
            break;
          case impl::global_item_tag::unit:
            global.unit = unsigned_value;
            break;
          case impl::global_item_tag::report_size:
            global.report_size = unsigned_value;
            global.report_size_defined = true;
            break;
          case impl::global_item_tag::report_id:
            if (unsigned_value == 0 ||
                unsigned_value > std::numeric_limits<uint8_t>::max()) {
              if (auto error = report_validation_failure(
                      parse_error_code::invalid_report_id,
                      item_offset)) {
                return std::unexpected(*error);
              }
              // Report ID 0 means the unnumbered report, while values above
              // 255 cannot be represented on the wire. Ignoring the invalid
              // declaration is the only option that does not truncate it.
              break;
            }
            // HID cannot add an ID byte after an unnumbered report has already
            // been described because all reports must then carry that byte.
            if (!report_id_declared && report_main_item_seen) {
              return std::unexpected(parse_error(parse_error_code::report_id_declared_too_late,
                                                 item_offset));
            }
            report_id_declared = true;
            global.report_id = static_cast<uint8_t>(unsigned_value);
            break;
          case impl::global_item_tag::report_count:
            if (unsigned_value == 0) {
              if (auto error = report_validation_failure(
                      parse_error_code::invalid_report_size_or_count,
                      item_offset)) {
                return std::unexpected(*error);
              }
            }
            global.report_count = unsigned_value;
            break;
          case impl::global_item_tag::push:
            if (data_size != 0) {
              if (auto error = report_validation_failure(
                      parse_error_code::invalid_item_data_size,
                      item_offset)) {
                return std::unexpected(*error);
              }
            }
            global_stack.push_back(global);
            break;
          case impl::global_item_tag::pop:
            if (data_size != 0) {
              if (auto error = report_validation_failure(
                      parse_error_code::invalid_item_data_size,
                      item_offset)) {
                return std::unexpected(*error);
              }
            }
            if (global_stack.empty()) {
              return std::unexpected(parse_error(parse_error_code::global_state_stack_underflow,
                                                 item_offset));
            }
            global = global_stack.back();
            global_stack.pop_back();
            break;
          default:
            if (auto error = report_validation_failure(
                    parse_error_code::unknown_or_reserved_item,
                    item_offset)) {
              return std::unexpected(*error);
            }
            break;
        }
        break;

      case impl::item_type::local:
        if (local.delimiter_open &&
            item_tag != std::to_underlying(impl::local_item_tag::usage) &&
            item_tag != std::to_underlying(impl::local_item_tag::usage_minimum) &&
            item_tag != std::to_underlying(impl::local_item_tag::usage_maximum) &&
            item_tag != std::to_underlying(impl::local_item_tag::delimiter)) {
          return std::unexpected(parse_error(parse_error_code::invalid_delimiter,
                                             item_offset));
        }

        switch (static_cast<impl::local_item_tag>(item_tag)) {
          case impl::local_item_tag::usage:
            if (local.delimiter_open) {
              local.delimiter_usage_sets.back().usages.push_back({unsigned_value, data_size});
            } else {
              local.usages.push_back({unsigned_value, data_size});
            }
            break;
          case impl::local_item_tag::usage_minimum:
            if (local.delimiter_open) {
              local.delimiter_usage_sets.back().usage_minimum =
                  impl::local_usage{unsigned_value, data_size};
            } else {
              local.usage_minimum = impl::local_usage{unsigned_value, data_size};
            }
            break;
          case impl::local_item_tag::usage_maximum:
            if (local.delimiter_open) {
              local.delimiter_usage_sets.back().usage_maximum =
                  impl::local_usage{unsigned_value, data_size};
            } else {
              local.usage_maximum = impl::local_usage{unsigned_value, data_size};
            }
            break;
          case impl::local_item_tag::designator_index:
            local.designator_indices.push_back(unsigned_value);
            break;
          case impl::local_item_tag::designator_minimum:
            local.designator_minimum = unsigned_value;
            break;
          case impl::local_item_tag::designator_maximum:
            local.designator_maximum = unsigned_value;
            break;
          case impl::local_item_tag::string_index:
            local.string_indices.push_back(unsigned_value);
            break;
          case impl::local_item_tag::string_minimum:
            local.string_minimum = unsigned_value;
            break;
          case impl::local_item_tag::string_maximum:
            local.string_maximum = unsigned_value;
            break;
          case impl::local_item_tag::delimiter:
            if (unsigned_value == std::to_underlying(impl::delimiter_value::open) &&
                !local.delimiter_open) {
              local.delimiter_usage_sets.emplace_back();
              local.delimiter_open = true;
            } else if (unsigned_value == std::to_underlying(impl::delimiter_value::close) &&
                       local.delimiter_open) {
              local.delimiter_open = false;
            } else {
              // Only a non-nested Open(1)/Close(0) pair is defined by HID.
              return std::unexpected(parse_error(parse_error_code::invalid_delimiter,
                                                 item_offset));
            }
            break;
          default:
            if (auto error = report_validation_failure(
                    parse_error_code::unknown_or_reserved_item,
                    item_offset)) {
              return std::unexpected(*error);
            }
            break;
        }
        break;

      case impl::item_type::reserved:
        if (local.delimiter_open) {
          return std::unexpected(parse_error(parse_error_code::invalid_delimiter,
                                             item_offset));
        }
        if (auto error = report_validation_failure(
                parse_error_code::unknown_or_reserved_item,
                item_offset)) {
          return std::unexpected(*error);
        }
        break;
    }
  }

  if (local.delimiter_open) {
    return std::unexpected(parse_error(parse_error_code::unclosed_delimiter,
                                       bytes.size()));
  }

  if (!global_stack.empty()) {
    return std::unexpected(parse_error(parse_error_code::unclosed_global_state,
                                       bytes.size()));
  }

  if (!collection_path.empty()) {
    return std::unexpected(parse_error(parse_error_code::unclosed_collection,
                                       bytes.size()));
  }

  return parse_result(descriptor(std::move(report_fields)),
                      std::move(diagnostics));
}
} // namespace pqrs::hid::report_descriptor
