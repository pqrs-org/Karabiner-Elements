#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <CoreGraphics/CoreGraphics.h>
#include <IOKit/hidsystem/IOLLEvent.h>
#include <optional>

#include "aux_control_button.hpp"
#include "event_type.hpp"
#include "impl/impl.h"
#include "key_code.hpp"

namespace pqrs {
namespace osx {
namespace cg_event {
inline std::optional<event_type> make_event_type(CGEventRef event) {
  if (!event) {
    return std::nullopt;
  }

  switch (CGEventGetType(event)) {
    case kCGEventKeyDown:
      return event_type::key_down;

    case kCGEventKeyUp:
      return event_type::key_up;

    default:
      break;
  }

  uint8_t value = 0;
  if (pqrs_osx_cg_event_get_aux_control_button_event_type(event, &value)) {
    switch (value) {
      case NX_KEYDOWN:
        return event_type::key_down;

      case NX_KEYUP:
        return event_type::key_up;

      default:
        break;
    }
  }

  return std::nullopt;
}

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
