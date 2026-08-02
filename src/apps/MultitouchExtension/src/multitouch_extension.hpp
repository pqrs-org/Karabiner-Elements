#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*krbn_core_service_connected_changed_callback)(bool connected);

void krbn_initialize(krbn_core_service_connected_changed_callback callback);
void krbn_terminate(void);

void krbn_core_service_async_set_variable(const char* name, int32_t value);

#ifdef __cplusplus
}
#endif
