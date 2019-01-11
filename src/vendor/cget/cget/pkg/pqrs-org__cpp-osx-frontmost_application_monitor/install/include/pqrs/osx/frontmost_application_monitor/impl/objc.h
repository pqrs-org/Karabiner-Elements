#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#ifdef __cplusplus
extern "C" {
#endif

// Do not use these functions directly.

typedef void pqrs_osx_frontmost_application_monitor_objc;
typedef void (*pqrs_osx_frontmost_application_monitor_callback)(const char* bundle_identifier,
                                                                const char* file_path,
                                                                void* context);
bool pqrs_osx_frontmost_application_monitor_initialize(pqrs_osx_frontmost_application_monitor_objc** monitor,
                                                       pqrs_osx_frontmost_application_monitor_callback callback,
                                                       void* context);
void pqrs_osx_frontmost_application_monitor_terminate(pqrs_osx_frontmost_application_monitor_objc** monitor);

#ifdef __cplusplus
}
#endif
