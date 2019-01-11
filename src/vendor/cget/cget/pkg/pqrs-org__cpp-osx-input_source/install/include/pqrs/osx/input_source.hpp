#pragma once

// pqrs::osx::input_source v1.1

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <Carbon/Carbon.h>
#include <optional>
#include <pqrs/cf/array.hpp>
#include <pqrs/cf/dictionary.hpp>
#include <pqrs/cf/string.hpp>
#include <vector>

namespace pqrs {
namespace osx {
namespace input_source {
// You have to call this method in main thread since TIS/TSM requires to be called in main thread.
inline cf::cf_ptr<TISInputSourceRef> make_current_keyboard_input_source(void) {
  cf::cf_ptr<TISInputSourceRef> result;

  if (auto input_source = TISCopyCurrentKeyboardInputSource()) {
    result = input_source;
    CFRelease(input_source);
  }

  return result;
}

// You have to call this method in main thread since TIS/TSM requires to be called in main thread.
inline std::vector<cf::cf_ptr<TISInputSourceRef>> make_selectable_keyboard_input_sources(void) {
  std::vector<cf::cf_ptr<TISInputSourceRef>> result;

  if (auto properties = pqrs::cf::make_cf_mutable_dictionary()) {
    CFDictionarySetValue(*properties, kTISPropertyInputSourceIsSelectCapable, kCFBooleanTrue);
    CFDictionarySetValue(*properties, kTISPropertyInputSourceCategory, kTISCategoryKeyboardInputSource);

    if (auto input_sources = TISCreateInputSourceList(*properties, false)) {
      auto size = CFArrayGetCount(input_sources);
      for (CFIndex i = 0; i < size; ++i) {
        if (auto s = pqrs::cf::get_cf_array_value<TISInputSourceRef>(input_sources, i)) {
          result.emplace_back(s);
        }
      }

      CFRelease(input_sources);
    }
  }

  return result;
}

// You have to call this method in main thread since TIS/TSM requires to be called in main thread.
inline std::optional<std::string> make_input_source_id(TISInputSourceRef input_source) {
  if (input_source) {
    if (auto s = static_cast<CFStringRef>(TISGetInputSourceProperty(input_source, kTISPropertyInputSourceID))) {
      return cf::make_string(s);
    }
  }

  return std::nullopt;
}

// You have to call this method in main thread since TIS/TSM requires to be called in main thread.
inline std::optional<std::string> make_localized_name(TISInputSourceRef input_source) {
  if (input_source) {
    if (auto s = static_cast<CFStringRef>(TISGetInputSourceProperty(input_source, kTISPropertyLocalizedName))) {
      return cf::make_string(s);
    }
  }

  return std::nullopt;
}

// You have to call this method in main thread since TIS/TSM requires to be called in main thread.
inline std::optional<std::string> make_input_mode_id(TISInputSourceRef input_source) {
  if (input_source) {
    if (auto s = static_cast<CFStringRef>(TISGetInputSourceProperty(input_source, kTISPropertyInputModeID))) {
      return cf::make_string(s);
    }
  }

  return std::nullopt;
}

// You have to call this method in main thread since TIS/TSM requires to be called in main thread.
inline std::vector<std::string> make_languages(TISInputSourceRef input_source) {
  std::vector<std::string> result;

  if (input_source) {
    if (auto languages = static_cast<CFArrayRef>(TISGetInputSourceProperty(input_source, kTISPropertyInputSourceLanguages))) {
      auto size = CFArrayGetCount(languages);
      for (CFIndex i = 0; i < size; ++i) {
        if (auto s = cf::get_cf_array_value<CFStringRef>(languages, i)) {
          if (auto language = cf::make_string(s)) {
            result.push_back(*language);
          }
        }
      }
    }
  }

  return result;
}

// You have to call this method in main thread since TIS/TSM requires to be called in main thread.
inline std::optional<std::string> make_first_language(TISInputSourceRef input_source) {
  if (input_source) {
    if (auto languages = static_cast<CFArrayRef>(TISGetInputSourceProperty(input_source, kTISPropertyInputSourceLanguages))) {
      if (auto s = cf::get_cf_array_value<CFStringRef>(languages, 0)) {
        return cf::make_string(s);
      }
    }
  }

  return std::nullopt;
}
} // namespace input_source
} // namespace osx
} // namespace pqrs
