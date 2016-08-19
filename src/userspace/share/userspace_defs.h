#pragma once

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>

#define krbn_distributed_notification_observed_object CFSTR("org.pqrs.karabiner")
#define krbn_distributed_notification_console_user_socket_directory_is_ready CFSTR("console_user_socket_directory_is_ready")

enum krbn_operation_type {
  KRBN_OPERATION_TYPE_NONE,
  // console_user_server -> grabber
  KRBN_OPERATION_TYPE_CONNECT,
  // grabber -> console_user_server
  KRBN_OPERATION_TYPE_STOP_KEY_REPEAT,
  // grabber -> console_user_server
  KRBN_OPERATION_TYPE_POST_MODIFIER_FLAGS,
  // grabber -> console_user_server
  KRBN_OPERATION_TYPE_POST_KEY,
};

enum krbn_event_type {
  KRBN_EVENT_TYPE_KEY_DOWN,
  KRBN_EVENT_TYPE_KEY_UP,
};

enum krbn_key_code {
  KRBN_KEY_CODE_F1 = 0x7a,
  KRBN_KEY_CODE_F2 = 0x78,
  KRBN_KEY_CODE_F3 = 0x63,
  KRBN_KEY_CODE_F4 = 0x76,
  KRBN_KEY_CODE_F5 = 0x60,
  KRBN_KEY_CODE_F6 = 0x61,
  KRBN_KEY_CODE_F7 = 0x62,
  KRBN_KEY_CODE_F8 = 0x64,
  KRBN_KEY_CODE_F9 = 0x65,
  KRBN_KEY_CODE_F10 = 0x6d,
  KRBN_KEY_CODE_F11 = 0x67,
  KRBN_KEY_CODE_F12 = 0x6f,
  KRBN_KEY_CODE_F13 = 0x69,
  KRBN_KEY_CODE_F14 = 0x6b,
  KRBN_KEY_CODE_F15 = 0x71,
  KRBN_KEY_CODE_F16 = 0x6a,
  KRBN_KEY_CODE_F17 = 0x40,
  KRBN_KEY_CODE_F18 = 0x4f,
  KRBN_KEY_CODE_F19 = 0x50,
};

struct krbn_operation_type_connect {
  uint8_t operation_type;
  pid_t console_user_server_pid;
};

struct krbn_operation_type_stop_key_repeat {
  uint8_t operation_type;
};

struct krbn_operation_type_post_modifier_flags {
  uint8_t operation_type;
  IOOptionBits flags;
};

struct krbn_operation_type_post_key {
  uint8_t operation_type;
  enum krbn_key_code key_code;
  enum krbn_event_type event_type;
  IOOptionBits flags;
};
