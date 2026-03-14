#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <CoreGraphics/CoreGraphics.h>
#include <optional>

#include "aux_control_button.hpp"
#include "impl/impl.h"
#include "key_code.hpp"

namespace pqrs {
namespace osx {
namespace cg_event {
inline std::optional<hid::usage_pair> make_usage_pair(CGEventRef event) {
  if (!event) {
    return std::nullopt;
  }

  switch (CGEventGetType(event)) {
    case kCGEventKeyDown:
    case kCGEventKeyUp:
    case kCGEventFlagsChanged:
      return make_usage_pair(key_code::value_t(static_cast<uint16_t>(
          CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode))));

    default:
      break;
  }

  uint16_t value = 0;
  if (pqrs_osx_cg_event_get_aux_control_button(event, &value)) {
    return make_usage_pair(aux_control_button::value_t(value));
  }

  return std::nullopt;
}
} // namespace cg_event
} // namespace osx
} // namespace pqrs
