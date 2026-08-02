#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool krbn_daemon_running(const char* service_name);

#ifdef __cplusplus
}
#endif
