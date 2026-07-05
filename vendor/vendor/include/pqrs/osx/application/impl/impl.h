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

typedef enum {
  pqrs_osx_application_terminate_reply_now,
  pqrs_osx_application_terminate_reply_cancel,
} pqrs_osx_application_terminate_reply_t;

typedef pqrs_osx_application_terminate_reply_t (*pqrs_osx_application_should_terminate_callback_t)(void);

void pqrs_osx_application_set_activation_policy(pqrs_osx_application_activation_policy_t policy);
void pqrs_osx_application_set_should_terminate_callback(pqrs_osx_application_should_terminate_callback_t callback);
void pqrs_osx_application_finish_launching(void);
void pqrs_osx_application_run(void);
void pqrs_osx_application_stop(void);

#ifdef __cplusplus
}
#endif
