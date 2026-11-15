#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <stdbool.h>
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
  int32_t application_detection_source;

  const char* role;
  const char* subrole;
  const char* role_description;
  const char* title;
  const char* description;
  const char* identifier;
  const char* window_title;

  int32_t has_window_position;
  double window_position_x;
  double window_position_y;

  int32_t has_window_size;
  double window_size_width;
  double window_size_height;
} pqrs_osx_accessibility_snapshot;

typedef void (*pqrs_osx_accessibility_monitor_callback)(int32_t force,
                                                        const pqrs_osx_accessibility_snapshot* snapshot);

typedef struct {
  bool frontmost_application;
  bool focused_ui_element;
  bool focused_window;
} pqrs_osx_accessibility_monitoring;

void pqrs_osx_accessibility_monitor_set_callback(pqrs_osx_accessibility_monitor_callback callback,
                                                 const pqrs_osx_accessibility_monitoring* monitoring);
void pqrs_osx_accessibility_monitor_unset_callback(void);
void pqrs_osx_accessibility_monitor_trigger(void);

#ifdef __cplusplus
}
#endif
