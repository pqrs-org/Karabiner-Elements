#pragma once

// (C) Copyright Takayama Fumihiko 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <CoreGraphics/CoreGraphics.h>
#include <pqrs/cf/cf_ptr.hpp>

namespace pqrs::osx::cg_event::mouse {
[[nodiscard]] inline CGPoint cursor_position() noexcept {
  auto point = CGPointZero;

  if (auto e = cf::adopt_cf_ptr(CGEventCreate(nullptr))) {
    point = CGEventGetLocation(*e);
  }

  return point;
}

[[nodiscard]] inline CGEventType mouse_down_event_type(CGMouseButton mouse_button) noexcept {
  switch (mouse_button) {
    case kCGMouseButtonLeft:
      return kCGEventLeftMouseDown;
    case kCGMouseButtonRight:
      return kCGEventRightMouseDown;
    default:
      return kCGEventOtherMouseDown;
  }
}

[[nodiscard]] inline CGEventType mouse_up_event_type(CGMouseButton mouse_button) noexcept {
  switch (mouse_button) {
    case kCGMouseButtonLeft:
      return kCGEventLeftMouseUp;
    case kCGMouseButtonRight:
      return kCGEventRightMouseUp;
    default:
      return kCGEventOtherMouseUp;
  }
}

inline void post_event(CGEventType mouse_type,
                       const CGPoint& mouse_cursor_position,
                       CGMouseButton mouse_button,
                       int click_count) noexcept {
  if (auto e = cf::adopt_cf_ptr(CGEventCreateMouseEvent(nullptr, mouse_type, mouse_cursor_position, mouse_button))) {
    CGEventSetIntegerValueField(*e, kCGMouseEventClickState, click_count);
    CGEventPost(kCGHIDEventTap, *e);
  }
}

inline void post_single_click(const CGPoint& mouse_cursor_position,
                              CGMouseButton mouse_button) noexcept {
  post_event(mouse_down_event_type(mouse_button),
             mouse_cursor_position,
             mouse_button,
             1);

  post_event(mouse_up_event_type(mouse_button),
             mouse_cursor_position,
             mouse_button,
             1);
}

inline void post_double_click(const CGPoint& mouse_cursor_position,
                              CGMouseButton mouse_button) noexcept {
  for (auto i = 1; i <= 2; ++i) {
    post_event(mouse_down_event_type(mouse_button),
               mouse_cursor_position,
               mouse_button,
               i);

    post_event(mouse_up_event_type(mouse_button),
               mouse_cursor_position,
               mouse_button,
               i);
  }
}
} // namespace pqrs::osx::cg_event::mouse
