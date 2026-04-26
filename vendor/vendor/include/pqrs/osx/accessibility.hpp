#pragma once

// pqrs::osx::accessibility v3.5

// (C) Copyright Takayama Fumihiko 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "accessibility/application.hpp"
#include "accessibility/focused_ui_element.hpp"
#include "accessibility/monitor.hpp"
#include <ApplicationServices/ApplicationServices.h>
#include <pqrs/cf/dictionary.hpp>

namespace pqrs {
namespace osx {
namespace accessibility {

inline bool is_process_trusted() {
  return AXIsProcessTrusted();
}

inline bool is_process_trusted_with_prompt() {
  auto options = cf::make_cf_mutable_dictionary();
  CFDictionarySetValue(*options, kAXTrustedCheckOptionPrompt, kCFBooleanTrue);
  return AXIsProcessTrustedWithOptions(*options);
}

} // namespace accessibility
} // namespace osx
} // namespace pqrs
