#pragma once

// (C) Copyright Takayama Fumihiko 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#ifdef __cplusplus
extern "C" {
#endif

// Do not use these functions directly.

char* pqrs_osx_workspace_find_application_url_by_bundle_identifier(const char* bundle_identifier);

#ifdef __cplusplus
}
#endif
