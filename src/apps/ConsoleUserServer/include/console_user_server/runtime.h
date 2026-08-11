#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*console_user_server_terminated_callback)(void);

void console_user_server_start(console_user_server_terminated_callback callback);
bool console_user_server_async_request_termination(void);
void console_user_server_finalize(void);

#ifdef __cplusplus
}
#endif
