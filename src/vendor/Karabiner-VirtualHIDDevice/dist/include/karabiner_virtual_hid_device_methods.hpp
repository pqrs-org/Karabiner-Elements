// -*- Mode: c++ -*-

#pragma once

#include "karabiner_virtual_hid_device.hpp"
#include <IOKit/IOKitLib.h>

namespace pqrs {
class karabiner_virtual_hid_device_methods final {
public:
  // VirtualHIDKeyboard

  static IOReturn initialize_virtual_hid_keyboard(mach_port_t connection, const karabiner_virtual_hid_device::properties::keyboard_initialization& properties) {
    return IOConnectCallStructMethod(connection,
                                     static_cast<uint32_t>(karabiner_virtual_hid_device::user_client_method::initialize_virtual_hid_keyboard),
                                     &properties, sizeof(properties),
                                     nullptr, 0);
  }

  static IOReturn terminate_virtual_hid_keyboard(mach_port_t connection) {
    return IOConnectCallStructMethod(connection,
                                     static_cast<uint32_t>(karabiner_virtual_hid_device::user_client_method::terminate_virtual_hid_keyboard),
                                     nullptr, 0,
                                     nullptr, 0);
  }

  static IOReturn dispatch_keyboard_event(mach_port_t connection, const karabiner_virtual_hid_device::hid_event_service::keyboard_event& keyboard_event) {
    return IOConnectCallStructMethod(connection,
                                     static_cast<uint32_t>(karabiner_virtual_hid_device::user_client_method::dispatch_keyboard_event),
                                     &keyboard_event, sizeof(keyboard_event),
                                     nullptr, 0);
  }

  static IOReturn post_keyboard_input_report(mach_port_t connection, const karabiner_virtual_hid_device::hid_report::keyboard_input& report) {
    return IOConnectCallStructMethod(connection,
                                     static_cast<uint32_t>(karabiner_virtual_hid_device::user_client_method::post_keyboard_input_report),
                                     &report, sizeof(report),
                                     nullptr, 0);
  }

  static IOReturn reset_virtual_hid_keyboard(mach_port_t connection) {
    return IOConnectCallStructMethod(connection,
                                     static_cast<uint32_t>(karabiner_virtual_hid_device::user_client_method::reset_virtual_hid_keyboard),
                                     nullptr, 0,
                                     nullptr, 0);
  }

  // VirtualHIDPointing

  static IOReturn initialize_virtual_hid_pointing(mach_port_t connection) {
    return IOConnectCallStructMethod(connection,
                                     static_cast<uint32_t>(karabiner_virtual_hid_device::user_client_method::initialize_virtual_hid_pointing),
                                     nullptr, 0,
                                     nullptr, 0);
  }

  static IOReturn terminate_virtual_hid_pointing(mach_port_t connection) {
    return IOConnectCallStructMethod(connection,
                                     static_cast<uint32_t>(karabiner_virtual_hid_device::user_client_method::terminate_virtual_hid_pointing),
                                     nullptr, 0,
                                     nullptr, 0);
  }

  static IOReturn post_pointing_input_report(mach_port_t connection, const karabiner_virtual_hid_device::hid_report::pointing_input& report) {
    return IOConnectCallStructMethod(connection,
                                     static_cast<uint32_t>(karabiner_virtual_hid_device::user_client_method::post_pointing_input_report),
                                     &report, sizeof(report),
                                     nullptr, 0);
  }

  static IOReturn reset_virtual_hid_pointing(mach_port_t connection) {
    return IOConnectCallStructMethod(connection,
                                     static_cast<uint32_t>(karabiner_virtual_hid_device::user_client_method::reset_virtual_hid_pointing),
                                     nullptr, 0,
                                     nullptr, 0);
  }
};
}
