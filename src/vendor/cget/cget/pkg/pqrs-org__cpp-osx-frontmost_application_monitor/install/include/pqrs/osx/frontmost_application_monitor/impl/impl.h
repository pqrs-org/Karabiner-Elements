#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

// Do not use these functions directly.

typedef void (*pqrs_osx_frontmost_application_monitor_callback)(const char* bundle_identifier,
                                                                const char* bundle_path,
                                                                const char* file_path,
                                                                pid_t pid);

void pqrs_osx_frontmost_application_monitor_set_callback(pqrs_osx_frontmost_application_monitor_callback callback);
void pqrs_osx_frontmost_application_monitor_unset_callback(void);
void pqrs_osx_frontmost_application_monitor_trigger(void);

#ifdef __cplusplus
}
#endif
