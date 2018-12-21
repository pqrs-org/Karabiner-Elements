#pragma once

#include "cf_utility.hpp"
#include <Carbon/Carbon.h>
#include <optional>
#include <pqrs/cf/string.hpp>

namespace krbn {
class input_source_utility final {
public:
  static std::optional<std::string> get_language(TISInputSourceRef input_source) {
    if (input_source) {
      if (auto languages = static_cast<CFArrayRef>(TISGetInputSourceProperty(input_source, kTISPropertyInputSourceLanguages))) {
        if (auto s = cf_utility::get_value<CFStringRef>(languages, 0)) {
          return pqrs::cf::make_string(s);
        }
      }
    }

    return std::nullopt;
  }

  static std::optional<std::string> get_input_source_id(TISInputSourceRef input_source) {
    if (input_source) {
      if (auto s = static_cast<CFStringRef>(TISGetInputSourceProperty(input_source, kTISPropertyInputSourceID))) {
        return pqrs::cf::make_string(s);
      }
    }

    return std::nullopt;
  }

  static std::optional<std::string> get_input_mode_id(TISInputSourceRef input_source) {
    if (input_source) {
      if (auto s = static_cast<CFStringRef>(TISGetInputSourceProperty(input_source, kTISPropertyInputModeID))) {
        return pqrs::cf::make_string(s);
      }
    }

    return std::nullopt;
  }
};
} // namespace krbn
