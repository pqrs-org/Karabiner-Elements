//
//  post_event_to_virtual_devices.cpp
//  karabiner_grabber
//
//  Created by Alex Song on 5/27/17.
//
//

#include "post_event_to_virtual_devices.hpp"

using namespace krbn::manipulator::details;
using krbn::stream_utility;

std::ostream& operator<<(std::ostream& stream, const post_event_to_virtual_devices::queue::event& event) {
  stream << "{"
  << "\"type\":";
  stream_utility::output_enum(stream, event.get_type());
  
  if (auto keyboard_event = event.get_keyboard_event()) {
    stream << ",\"keyboard_event.usage\":" << static_cast<uint32_t>(keyboard_event->usage);
    stream << ",\"keyboard_event.usage_page\":" << static_cast<uint32_t>(keyboard_event->usage_page);
    stream << ",\"keyboard_event.value\":" << static_cast<uint32_t>(keyboard_event->value);
  }
  
  if (auto pointing_input = event.get_pointing_input()) {
    stream << std::hex;
    stream << ",\"pointing_input.buttons[0]\":0x" << static_cast<int>(pointing_input->buttons[0]);
    stream << ",\"pointing_input.buttons[1]\":0x" << static_cast<int>(pointing_input->buttons[1]);
    stream << ",\"pointing_input.buttons[2]\":0x" << static_cast<int>(pointing_input->buttons[2]);
    stream << ",\"pointing_input.buttons[3]\":0x" << static_cast<int>(pointing_input->buttons[3]);
    stream << std::dec;
    stream << ",\"pointing_input.x\":" << static_cast<int>(pointing_input->x);
    stream << ",\"pointing_input.y\":" << static_cast<int>(pointing_input->y);
    stream << ",\"pointing_input.vertical_wheel\":" << static_cast<int>(pointing_input->vertical_wheel);
    stream << ",\"pointing_input.horizontal_wheel\":" << static_cast<int>(pointing_input->horizontal_wheel);
  }
  
  stream << ",\"time_stamp\":" << event.get_time_stamp();
  
  stream << "}";
  
  return stream;
}
