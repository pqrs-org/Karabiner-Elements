#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*console_user_server_terminated_callback)(void);

void console_user_server_start(console_user_server_terminated_callback callback);
void console_user_server_terminate(void);

#ifdef __cplusplus
}
#endif
