#pragma once

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <IOKit/hid/IOHIDElement.h>

namespace pqrs {
namespace osx {
enum class iokit_hid_element_type {
  input_misc = kIOHIDElementTypeInput_Misc,
  input_button = kIOHIDElementTypeInput_Button,
  input_axis = kIOHIDElementTypeInput_Axis,
  input_scancodes = kIOHIDElementTypeInput_ScanCodes,
  input_null = kIOHIDElementTypeInput_NULL,
  output = kIOHIDElementTypeOutput,
  feature = kIOHIDElementTypeFeature,
  collection = kIOHIDElementTypeCollection,
};

inline const char* get_iokit_hid_element_type_name(iokit_hid_element_type type) {
  switch (type) {
#define GET_IOKIT_HID_ELEMENT_TYPE_NAME_RETURN_NAME(TYPE) \
  case iokit_hid_element_type::TYPE:                      \
    return #TYPE;

    GET_IOKIT_HID_ELEMENT_TYPE_NAME_RETURN_NAME(input_misc);
    GET_IOKIT_HID_ELEMENT_TYPE_NAME_RETURN_NAME(input_button);
    GET_IOKIT_HID_ELEMENT_TYPE_NAME_RETURN_NAME(input_axis);
    GET_IOKIT_HID_ELEMENT_TYPE_NAME_RETURN_NAME(input_scancodes);
    GET_IOKIT_HID_ELEMENT_TYPE_NAME_RETURN_NAME(input_null);
    GET_IOKIT_HID_ELEMENT_TYPE_NAME_RETURN_NAME(output);
    GET_IOKIT_HID_ELEMENT_TYPE_NAME_RETURN_NAME(feature);
    GET_IOKIT_HID_ELEMENT_TYPE_NAME_RETURN_NAME(collection);

#undef GET_IOKIT_HID_ELEMENT_TYPE_NAME_RETURN_NAME
  }

  return "unknown";
}
} // namespace osx
} // namespace pqrs
