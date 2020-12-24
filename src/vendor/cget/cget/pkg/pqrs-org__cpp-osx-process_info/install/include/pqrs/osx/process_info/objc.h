#pragma once

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#ifdef __cplusplus
extern "C" {
#endif

// Do not use these functions directly.

void pqrs_osx_process_info_create_globally_unique_string(char* buffer, size_t buffer_size);
int pqrs_osx_process_info_process_identifier(void);

void pqrs_osx_process_info_disable_sudden_termination(void);
void pqrs_osx_process_info_enable_sudden_termination(void);

#ifdef __cplusplus
}
#endif
