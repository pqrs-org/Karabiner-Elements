#pragma once

// (C) Copyright Takayama Fumihiko 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#ifdef __cplusplus
extern "C" {
#endif

// Do not use these functions directly.

void pqrs_osx_workspace_find_application_url_by_bundle_identifier(const char* bundle_identifier,
                                                                  char* buffer,
                                                                  int buffer_size);

#ifdef __cplusplus
}
#endif
