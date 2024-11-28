#pragma once

// (C) Copyright Takayama Fumihiko 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#ifdef __cplusplus
extern "C" {
#endif

// Do not use these functions directly.

void pqrs_osx_workspace_open_application_by_bundle_identifier(const char* bundle_identifier);

void pqrs_osx_workspace_open_application_by_file_path(const char* file_path);

void pqrs_osx_workspace_find_application_url_by_bundle_identifier(const char* bundle_identifier,
                                                                  char* buffer,
                                                                  int buffer_size);

int pqrs_osx_workspace_application_running_by_bundle_identifier(const char* bundle_identifier);

int pqrs_osx_workspace_application_running_by_file_path(const char* file_path);

#ifdef __cplusplus
}
#endif
