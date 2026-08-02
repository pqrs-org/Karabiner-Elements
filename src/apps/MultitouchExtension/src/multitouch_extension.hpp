#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*krbn_core_service_connected_changed_callback)(bool connected);

typedef struct {
  int32_t finger_count_upper_quarter_area;
  int32_t finger_count_lower_quarter_area;
  int32_t finger_count_left_quarter_area;
  int32_t finger_count_right_quarter_area;
  int32_t finger_count_upper_half_area;
  int32_t finger_count_lower_half_area;
  int32_t finger_count_left_half_area;
  int32_t finger_count_right_half_area;
  int32_t finger_count_total;
  int32_t palm_count_upper_half_area;
  int32_t palm_count_lower_half_area;
  int32_t palm_count_left_half_area;
  int32_t palm_count_right_half_area;
  int32_t palm_count_total;
} krbn_multitouch_extension_variables;

void krbn_initialize(krbn_core_service_connected_changed_callback callback);
void krbn_terminate(void);

bool krbn_core_service_async_set_variables(krbn_multitouch_extension_variables variables);

#ifdef __cplusplus
}
#endif
