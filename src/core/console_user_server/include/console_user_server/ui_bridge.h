#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*console_user_server_string_callback)(const char* value);

void console_user_server_register_ui_state_callback(console_user_server_string_callback callback);
void console_user_server_register_notification_message_callback(console_user_server_string_callback callback);
void console_user_server_select_profile(size_t index);
void console_user_server_launch_settings(void);
void console_user_server_launch_event_viewer(void);
void console_user_server_check_for_updates(bool include_beta_versions);
void console_user_server_restart(void);
void console_user_server_quit(void);

#ifdef __cplusplus
}
#endif
