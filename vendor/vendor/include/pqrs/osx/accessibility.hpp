#pragma once

// pqrs::osx::accessibility v4.1.0

// (C) Copyright Takayama Fumihiko 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "accessibility/application.hpp"
#include "accessibility/focused_ui_element.hpp"
#include "accessibility/monitor.hpp"
#include <ApplicationServices/ApplicationServices.h>
#include <pqrs/cf/dictionary.hpp>

namespace pqrs::osx::accessibility {

[[nodiscard]] inline bool is_process_trusted() noexcept {
  return AXIsProcessTrusted();
}

inline bool is_process_trusted_with_prompt() {
  auto options = cf::make_cf_mutable_dictionary();
  CFDictionarySetValue(*options, kAXTrustedCheckOptionPrompt, kCFBooleanTrue);
  return AXIsProcessTrustedWithOptions(*options);
}

} // namespace pqrs::osx::accessibility
