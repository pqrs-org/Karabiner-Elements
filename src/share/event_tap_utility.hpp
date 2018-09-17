#pragma once

// `krbn::event_tap_utility` can be used safely in a multi-threaded environment.

#include "boost_defs.hpp"

#include "event_queue.hpp"
#include <CoreGraphics/CoreGraphics.h>
#include <boost/optional.hpp>

namespace krbn {
class event_tap_utility final {
public:
  static boost::optional<std::pair<event_type, event_queue::entry::event>> make_event(CGEventType type,
                                                                                      CGEventRef event) {
    switch (type) {
      case kCGEventLeftMouseDown:
        return std::make_pair(event_type::key_down,
                              event_queue::entry::event(pointing_button::button1));

      case kCGEventLeftMouseUp:
        return std::make_pair(event_type::key_up,
                              event_queue::entry::event(pointing_button::button1));

      case kCGEventRightMouseDown:
        return std::make_pair(event_type::key_down,
                              event_queue::entry::event(pointing_button::button2));

      case kCGEventRightMouseUp:
        return std::make_pair(event_type::key_up,
                              event_queue::entry::event(pointing_button::button2));

      case kCGEventOtherMouseDown:
        return std::make_pair(event_type::key_down,
                              event_queue::entry::event(pointing_button::button3));

      case kCGEventOtherMouseUp:
        return std::make_pair(event_type::key_up,
                              event_queue::entry::event(pointing_button::button3));

      case kCGEventMouseMoved:
      case kCGEventLeftMouseDragged:
      case kCGEventRightMouseDragged:
      case kCGEventOtherMouseDragged:
        return std::make_pair(event_type::single,
                              event_queue::entry::event(pointing_motion()));

      case kCGEventScrollWheel: {
        // Set non-zero value for `manipulator::details::base::unset_alone_if_needed`.
        pointing_motion pointing_motion;
        pointing_motion.set_vertical_wheel(1);
        return std::make_pair(event_type::single,
                              event_queue::entry::event(pointing_motion));
      }

      case kCGEventNull:
      case kCGEventKeyDown:
      case kCGEventKeyUp:
      case kCGEventFlagsChanged:
      case kCGEventTabletPointer:
      case kCGEventTabletProximity:
      case kCGEventTapDisabledByTimeout:
      case kCGEventTapDisabledByUserInput:
        break;
    }

    return boost::none;
  }
};
} // namespace krbn
