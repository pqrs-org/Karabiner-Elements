#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <CoreGraphics/CoreGraphics.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Do not use this function directly.
bool pqrs_osx_cg_event_get_aux_control_button(CGEventRef event, uint16_t* value);

// Do not use this function directly.
bool pqrs_osx_cg_event_get_aux_control_button_event_type(CGEventRef event, uint8_t* value);

#ifdef __cplusplus
}
#endif
