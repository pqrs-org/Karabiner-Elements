#pragma once

#include <IOKit/hidsystem/IOHIDLib.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*game_pad_viewer_hid_value_arrived_callback)(uint64_t device_id,
                                                           bool is_keyboard,
                                                           bool is_pointing_device,
                                                           bool is_game_pad,
                                                           int32_t usage_page,
                                                           int32_t usage,
                                                           int64_t logical_max,
                                                           int64_t logical_min,
                                                           int64_t integer_value);

void game_pad_viewer_initialize(game_pad_viewer_hid_value_arrived_callback callback);
void game_pad_viewer_terminate(void);

bool game_pad_viewer_hid_value_monitor_observed(void);

#ifdef __cplusplus
}
#endif
