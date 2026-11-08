#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#ifdef __cplusplus
extern "C" {
#endif

// Do not use these functions directly.

void* _Nonnull pqrs_osx_autorelease_pool_push(void);
void pqrs_osx_autorelease_pool_pop(void* _Nonnull token);

#ifdef __cplusplus
}
#endif
