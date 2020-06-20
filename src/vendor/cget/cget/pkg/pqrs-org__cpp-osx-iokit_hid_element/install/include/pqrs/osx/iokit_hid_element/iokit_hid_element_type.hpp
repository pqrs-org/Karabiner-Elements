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
}
} // namespace pqrs
