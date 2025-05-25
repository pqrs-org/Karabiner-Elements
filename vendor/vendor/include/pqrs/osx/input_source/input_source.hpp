#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <Carbon/Carbon.h>
#include <optional>
#include <pqrs/cf/array.hpp>
#include <pqrs/cf/dictionary.hpp>
#include <pqrs/cf/string.hpp>
#include <pqrs/gcd.hpp>
#include <vector>

namespace pqrs {
namespace osx {
namespace input_source {
inline cf::cf_ptr<TISInputSourceRef> make_current_keyboard_input_source(void) {
  cf::cf_ptr<TISInputSourceRef> result;

  __block TISInputSourceRef input_source = nullptr;

  gcd::dispatch_sync_on_main_queue(^{
    input_source = TISCopyCurrentKeyboardInputSource();
  });

  if (input_source) {
    result = input_source;
    CFRelease(input_source);
  }

  return result;
}

inline cf::cf_ptr<TISInputSourceRef> make_input_method_keyboard_layout_override(void) {
  cf::cf_ptr<TISInputSourceRef> result;

  __block TISInputSourceRef input_source = nullptr;

  gcd::dispatch_sync_on_main_queue(^{
    input_source = TISCopyInputMethodKeyboardLayoutOverride();
  });

  if (input_source) {
    result = input_source;
    CFRelease(input_source);
  }

  return result;
}

inline std::vector<cf::cf_ptr<TISInputSourceRef>> make_selectable_keyboard_input_sources(void) {
  std::vector<cf::cf_ptr<TISInputSourceRef>> result;

  if (auto properties = cf::make_cf_mutable_dictionary()) {
    CFDictionarySetValue(*properties, kTISPropertyInputSourceIsSelectCapable, kCFBooleanTrue);
    CFDictionarySetValue(*properties, kTISPropertyInputSourceCategory, kTISCategoryKeyboardInputSource);

    __block CFArrayRef input_sources = nullptr;

    gcd::dispatch_sync_on_main_queue(^{
      input_sources = TISCreateInputSourceList(*properties, false);
    });

    if (input_sources) {
      auto size = CFArrayGetCount(input_sources);
      for (CFIndex i = 0; i < size; ++i) {
        if (auto s = cf::get_cf_array_value<TISInputSourceRef>(input_sources, i)) {
          result.emplace_back(s);
        }
      }

      CFRelease(input_sources);
    }
  }

  return result;
}

inline void select(TISInputSourceRef input_source) {
  if (input_source) {
    gcd::dispatch_sync_on_main_queue(^{
      TISSelectInputSource(input_source);
    });
  }
}

inline void set_input_method_keyboard_layout_override(TISInputSourceRef input_source) {
  if (input_source) {
    gcd::dispatch_sync_on_main_queue(^{
      TISSetInputMethodKeyboardLayoutOverride(input_source);
    });
  }
}

inline std::optional<std::string> make_property_string(TISInputSourceRef input_source,
                                                       CFStringRef key) {
  if (input_source) {
    __block CFStringRef s = nullptr;

    gcd::dispatch_sync_on_main_queue(^{
      s = static_cast<CFStringRef>(TISGetInputSourceProperty(input_source, key));
    });

    if (s) {
      return cf::make_string(s);
    }
  }

  return std::nullopt;
}

inline std::optional<std::string> make_input_source_id(TISInputSourceRef input_source) {
  return make_property_string(input_source,
                              kTISPropertyInputSourceID);
}

inline std::optional<std::string> make_localized_name(TISInputSourceRef input_source) {
  return make_property_string(input_source,
                              kTISPropertyLocalizedName);
}

inline std::optional<std::string> make_input_mode_id(TISInputSourceRef input_source) {
  return make_property_string(input_source,
                              kTISPropertyInputModeID);
}

inline std::vector<std::string> make_languages(TISInputSourceRef input_source) {
  std::vector<std::string> result;

  if (input_source) {
    __block CFArrayRef languages = nullptr;

    gcd::dispatch_sync_on_main_queue(^{
      languages = static_cast<CFArrayRef>(TISGetInputSourceProperty(input_source,
                                                                    kTISPropertyInputSourceLanguages));
    });

    if (languages) {
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

inline std::optional<std::string> make_first_language(TISInputSourceRef input_source) {
  auto languages = make_languages(input_source);
  if (!languages.empty()) {
    return languages.front();
  }

  return std::nullopt;
}
} // namespace input_source
} // namespace osx
} // namespace pqrs
