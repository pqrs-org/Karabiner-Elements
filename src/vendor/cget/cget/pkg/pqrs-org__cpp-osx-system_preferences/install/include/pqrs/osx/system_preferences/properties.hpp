#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include "keyboard_type_key.hpp"
#include "system_preferences.hpp"
#include <pqrs/cf/string.hpp>
#include <pqrs/hash.hpp>
#include <unordered_map>

namespace pqrs {
namespace osx {
namespace system_preferences {
class properties final {
public:
  properties(void) : use_fkeys_as_standard_function_keys_(false),
                     scroll_direction_is_natural_(true) {
  }

  bool get_use_fkeys_as_standard_function_keys(void) const {
    return use_fkeys_as_standard_function_keys_;
  }

  void set_use_fkeys_as_standard_function_keys(bool value) {
    use_fkeys_as_standard_function_keys_ = value;
  }

  bool get_scroll_direction_is_natural(void) const {
    return scroll_direction_is_natural_;
  }

  void set_scroll_direction_is_natural(bool value) {
    scroll_direction_is_natural_ = value;
  }

  const std::unordered_map<keyboard_type_key, iokit_keyboard_type> get_keyboard_types(void) const {
    return keyboard_types_;
  }

  void set_keyboard_types(const std::unordered_map<keyboard_type_key, iokit_keyboard_type>& value) {
    keyboard_types_ = value;
  }

  void update(void) {
    // use_fkeys_as_standard_function_keys_

    if (auto value = find_bool_property(CFSTR("com.apple.keyboard.fnState"),
                                        CFSTR("Apple Global Domain"))) {
      use_fkeys_as_standard_function_keys_ = *value;
    }

    // scroll_direction_is_natural_

    if (auto value = find_bool_property(CFSTR("com.apple.swipescrolldirection"),
                                        CFSTR("Apple Global Domain"))) {
      scroll_direction_is_natural_ = *value;
    }

    // keyboard_types_

    keyboard_types_.clear();

    if (auto value = find_dictionary_property(CFSTR("keyboardtype"),
                                              CFSTR("com.apple.keyboardtype"))) {
      CFDictionaryApplyFunction(
          *value,
          [](const void* key, const void* value, void* context) {
            auto self = reinterpret_cast<properties*>(context);

            if (auto key_string = pqrs::cf::make_string(key)) {
              auto separator1 = key_string->find("-", 0);
              if (separator1 == std::string::npos) {
                return;
              }
              auto separator2 = key_string->find("-", separator1 + 1);
              if (separator2 == std::string::npos) {
                return;
              }

              try {
                auto product_id_string = key_string->substr(0, separator1);
                auto vendor_id_string = key_string->substr(separator1 + 1, separator2 - separator1 - 1);
                auto country_code_string = key_string->substr(separator2 + 1);

                auto product_id = iokit_hid_product_id(std::stoll(product_id_string));
                auto vendor_id = iokit_hid_vendor_id(std::stoll(vendor_id_string));
                auto country_code = iokit_hid_country_code(std::stoll(country_code_string));

                if (auto value_number = pqrs::cf::make_number<int8_t>(value)) {
                  auto k = keyboard_type_key(vendor_id,
                                             product_id,
                                             country_code);
                  self->keyboard_types_[k] = iokit_keyboard_type(*value_number);
                }
              } catch (...) {
              }
            }
          },
          this);
    }
  }

  bool operator==(const properties& other) const {
    return use_fkeys_as_standard_function_keys_ == other.use_fkeys_as_standard_function_keys_ &&
           scroll_direction_is_natural_ == other.scroll_direction_is_natural_ &&
           keyboard_types_ == other.keyboard_types_;
  }

  bool operator!=(const properties& other) const {
    return !(*this == other);
  }

private:
  bool use_fkeys_as_standard_function_keys_;
  bool scroll_direction_is_natural_;
  std::unordered_map<keyboard_type_key, iokit_keyboard_type> keyboard_types_;
};
} // namespace system_preferences
} // namespace osx
} // namespace pqrs
