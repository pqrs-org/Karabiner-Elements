#pragma once

// (C) Copyright Takayama Fumihiko 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <CoreGraphics/CoreGraphics.h>
#include <pqrs/cf/cf_ptr.hpp>

namespace pqrs {
namespace osx {
namespace cg_event {
namespace mouse {
inline CGPoint cursor_position() {
  auto point = CGPointZero;

  if (auto e = CGEventCreate(nullptr)) {
    point = CGEventGetLocation(e);
    CFRelease(e);
  }

  return point;
}

inline CGEventType mouse_down_event_type(CGMouseButton mouse_button) {
  switch (mouse_button) {
    case kCGMouseButtonLeft:
      return kCGEventLeftMouseDown;
    case kCGMouseButtonRight:
      return kCGEventRightMouseDown;
    default:
      return kCGEventOtherMouseDown;
  }
}

inline CGEventType mouse_up_event_type(CGMouseButton mouse_button) {
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
                       int click_count) {
  if (auto e = CGEventCreateMouseEvent(nullptr, mouse_type, mouse_cursor_position, mouse_button)) {
    CGEventSetIntegerValueField(e, kCGMouseEventClickState, click_count);
    CGEventPost(kCGHIDEventTap, e);

    CFRelease(e);
  }
}

inline void post_single_click(const CGPoint& mouse_cursor_position,
                              CGMouseButton mouse_button) {
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
                              CGMouseButton mouse_button) {
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
} // namespace mouse
} // namespace cg_event
} // namespace osx
} // namespace pqrs
