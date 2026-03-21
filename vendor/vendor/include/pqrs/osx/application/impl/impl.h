#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#ifdef __cplusplus
extern "C" {
#endif

// Do not use these functions directly.

typedef enum {
  pqrs_osx_application_activation_policy_regular,
  pqrs_osx_application_activation_policy_accessory,
  pqrs_osx_application_activation_policy_prohibited,
} pqrs_osx_application_activation_policy_t;

void pqrs_osx_application_set_activation_policy(pqrs_osx_application_activation_policy_t policy);
void pqrs_osx_application_finish_launching(void);
void pqrs_osx_application_run(void);

#ifdef __cplusplus
}
#endif
