#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#ifdef __cplusplus
extern "C" {
#endif

// Do not use these functions directly.

typedef void* pqrs_osx_frontmost_application_monitor_callback_context;
typedef void (*pqrs_osx_frontmost_application_monitor_callback)(const char* bundle_identifier,
                                                                const char* file_path,
                                                                pqrs_osx_frontmost_application_monitor_callback_context context);

void pqrs_osx_frontmost_application_monitor_register(pqrs_osx_frontmost_application_monitor_callback callback,
                                                     void* context);
void pqrs_osx_frontmost_application_monitor_unregister(pqrs_osx_frontmost_application_monitor_callback callback,
                                                       void* context);

#ifdef __cplusplus
}
#endif
