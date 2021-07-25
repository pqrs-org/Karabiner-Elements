#pragma once

#include "queue.hpp"
#include <pqrs/osx/cg_display.hpp>
#include <pqrs/osx/cg_event.hpp>
#include <pqrs/osx/system_preferences.hpp>

namespace krbn {
namespace manipulator {
namespace manipulators {
namespace post_event_to_virtual_devices {
class software_function_handler final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  software_function_handler(void) : dispatcher_client() {
  }

  virtual ~software_function_handler(void) {
    detach_from_dispatcher();
  }

  void execute_software_function(const software_function& software_function) const {
    if (auto v = software_function.get_if<software_function_details::cg_event_double_click>()) {
      execute_cg_event_double_click(*v);
    } else if (auto v = software_function.get_if<software_function_details::set_mouse_cursor_position>()) {
      execute_set_mouse_cursor_position(*v);
    }
  }

private:
  void execute_cg_event_double_click(const software_function_details::cg_event_double_click& cg_event_double_click) const {
    pqrs::osx::cg_event::mouse::post_double_click(
        pqrs::osx::cg_event::mouse::cursor_position(),
        CGMouseButton(cg_event_double_click.get_button()));
  }

  void execute_set_mouse_cursor_position(const software_function_details::set_mouse_cursor_position& set_mouse_cursor_position) const {
    if (auto screen = set_mouse_cursor_position.get_screen()) {
      auto active_displays = pqrs::osx::cg_display::active_displays();
      if (*screen < active_displays.size()) {
        CGDisplayMoveCursorToPoint(active_displays[*screen],
                                   set_mouse_cursor_position.get_point(
                                       CGDisplayBounds(active_displays[*screen])));
      }
    } else {
      CGWarpMouseCursorPosition(set_mouse_cursor_position.get_point(
          CGDisplayBounds(CGMainDisplayID())));
    }
  }
};
} // namespace post_event_to_virtual_devices
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
