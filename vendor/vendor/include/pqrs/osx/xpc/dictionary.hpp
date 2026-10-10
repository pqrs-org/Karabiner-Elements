#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "error.hpp"
#include "object.hpp"
#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <vector>

namespace pqrs::osx::xpc {

// Move-only owner of a dictionary reference. Use from_xpc_object to wrap a borrowed reference.
// Shared references refer to the same mutable dictionary; callers must serialize mutations.
class dictionary final {
public:
  dictionary()
      : value_(adopt_xpc_object(xpc_dictionary_create(nullptr,
                                                      nullptr,
                                                      0))) {}

  dictionary(const dictionary&) = delete;
  dictionary& operator=(const dictionary&) = delete;
  dictionary(dictionary&&) noexcept = default;
  dictionary& operator=(dictionary&&) noexcept = default;

  // Wrap a borrowed dictionary with an owned reference. Error events are rejected.
  [[nodiscard]] static std::optional<dictionary> from_xpc_object(xpc_object_t value) noexcept {
    if (!value ||
        xpc_get_type(value) != XPC_TYPE_DICTIONARY) {
      return std::nullopt;
    }

    return dictionary(object(value));
  }

  [[nodiscard]] explicit operator bool() const noexcept {
    return static_cast<bool>(value_);
  }

  [[nodiscard]] xpc_object_t get() const noexcept {
    return value_.get();
  }

  struct get_data_options final {
    // Maximum byte count to copy. nullopt means unlimited; zero permits only empty data.
    std::optional<size_t> max_size;
  };

  // Return an owned copy independent of the dictionary lifetime.
  // Reject missing/mistyped values and oversized data before allocating a copy.
  [[nodiscard]] std::expected<std::vector<uint8_t>, errc> get_data(const char* key,
                                                                   const get_data_options& options) const {
    auto value = xpc_dictionary_get_value(get(),
                                          key);
    if (!value ||
        xpc_get_type(value) != XPC_TYPE_DATA) {
      return std::unexpected(errc::invalid_message);
    }

    auto size = xpc_data_get_length(value);
    if (options.max_size && size > *options.max_size) {
      return std::unexpected(errc::message_too_large);
    }
    if (size == 0) {
      return std::vector<uint8_t>{};
    }

    auto data = static_cast<const uint8_t*>(xpc_data_get_bytes_ptr(value));
    return std::vector<uint8_t>(data,
                                data + size);
  }

  void set_data(const char* key,
                std::span<const uint8_t> value) {
    xpc_dictionary_set_data(get(),
                            key,
                            value.data(),
                            value.size());
  }

  // Sets a zero-length data value without removing the key.
  void set_empty_data(const char* key) {
    set_data(key,
             {});
  }

  [[nodiscard]] std::optional<uint64_t> get_uint64(const char* key) const {
    auto value = xpc_dictionary_get_value(get(), key);
    if (!value || xpc_get_type(value) != XPC_TYPE_UINT64) {
      return std::nullopt;
    }
    return xpc_uint64_get_value(value);
  }

  void set_uint64(const char* key, uint64_t value) {
    xpc_dictionary_set_uint64(get(), key, value);
  }

  [[nodiscard]] std::optional<bool> get_bool(const char* key) const {
    auto value = xpc_dictionary_get_value(get(),
                                          key);
    if (!value ||
        xpc_get_type(value) != XPC_TYPE_BOOL) {
      return std::nullopt;
    }

    return xpc_bool_get_value(value);
  }

  void set_bool(const char* key,
                bool value) {
    xpc_dictionary_set_bool(get(),
                            key,
                            value);
  }

private:
  explicit dictionary(object value) noexcept
      : value_(std::move(value)) {}

  object value_;
};
} // namespace pqrs::osx::xpc
