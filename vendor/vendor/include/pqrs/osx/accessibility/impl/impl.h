#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <stdint.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

// Do not use these functions directly.

typedef struct {
  const char* application_name;
  const char* bundle_identifier;
  const char* bundle_path;
  const char* file_path;
  pid_t pid;

  const char* role;
  const char* subrole;
  const char* role_description;
  const char* title;
  const char* description;
  const char* identifier;

  int32_t has_window_position;
  double window_position_x;
  double window_position_y;

  int32_t has_window_size;
  double window_size_width;
  double window_size_height;
} pqrs_osx_accessibility_snapshot;

typedef void (*pqrs_osx_accessibility_monitor_callback)(int32_t force,
                                                        const pqrs_osx_accessibility_snapshot* snapshot);

void pqrs_osx_accessibility_monitor_set_callback(pqrs_osx_accessibility_monitor_callback callback);
void pqrs_osx_accessibility_monitor_unset_callback(void);
void pqrs_osx_accessibility_monitor_async_trigger(void);

#ifdef __cplusplus
}
#endif
