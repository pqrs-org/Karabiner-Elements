#pragma once

// (C) Copyright Takayama Fumihiko 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Do not use these functions directly.

struct pqrs_osx_workspace_open_configuration {
  bool activates;
  bool adds_to_recent_items;
  bool allows_running_application_substitution;
  bool creates_new_application_instance;
  bool hides;
  bool hides_others;
  const char** arguments;
};

void pqrs_osx_workspace_open_application_by_bundle_identifier(const char* bundle_identifier,
                                                              const struct pqrs_osx_workspace_open_configuration* configuration);

void pqrs_osx_workspace_open_application_by_file_path(const char* file_path,
                                                      const struct pqrs_osx_workspace_open_configuration* configuration);

void pqrs_osx_workspace_find_application_url_by_bundle_identifier(const char* bundle_identifier,
                                                                  char* buffer,
                                                                  int buffer_size);

int pqrs_osx_workspace_application_running_by_bundle_identifier(const char* bundle_identifier);

int pqrs_osx_workspace_application_running_by_file_path(const char* file_path);

#ifdef __cplusplus
}
#endif
