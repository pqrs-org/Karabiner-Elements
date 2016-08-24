#pragma once

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>

#define krbn_distributed_notification_observed_object CFSTR("org.pqrs.karabiner")
#define krbn_distributed_notification_console_user_socket_directory_is_ready CFSTR("console_user_socket_directory_is_ready")

enum krbn_operation_type {
  krbn_operation_type_none,
  // console_user_server -> grabber
  krbn_operation_type_connect,
  // console_user_server -> grabber
  krbn_operation_type_define_simple_modifications,
  // grabber -> console_user_server
  krbn_operation_type_stop_key_repeat,
  // grabber -> console_user_server
  krbn_operation_type_post_modifier_flags,
  // grabber -> console_user_server
  krbn_operation_type_post_key,
};

enum krbn_event_type {
  krbn_event_type_key_down,
  krbn_event_type_key_up,
};

enum krbn_key_code {
  krbn_key_code_f1,
  krbn_key_code_f2,
  krbn_key_code_f3,
  krbn_key_code_f4,
  krbn_key_code_f5,
  krbn_key_code_f6,
  krbn_key_code_f7,
  krbn_key_code_f8,
  krbn_key_code_f9,
  krbn_key_code_f10,
  krbn_key_code_f11,
  krbn_key_code_f12,
  krbn_key_code_fn_f1,
  krbn_key_code_fn_f2,
  krbn_key_code_fn_f3,
  krbn_key_code_fn_f4,
  krbn_key_code_fn_f5,
  krbn_key_code_fn_f6,
  krbn_key_code_fn_f7,
  krbn_key_code_fn_f8,
  krbn_key_code_fn_f9,
  krbn_key_code_fn_f10,
  krbn_key_code_fn_f11,
  krbn_key_code_fn_f12,
};

struct krbn_operation_type_connect_struct {
  uint8_t operation_type;
  pid_t console_user_server_pid;
};

struct krbn_operation_type_define_simple_modifications_struct {
  uint8_t operation_type;
  size_t size;
  uint32_t data[0];
};

struct krbn_operation_type_stop_key_repeat_struct {
  uint8_t operation_type;
};

struct krbn_operation_type_post_modifier_flags_struct {
  uint8_t operation_type;
  IOOptionBits flags;
};

struct krbn_operation_type_post_key_struct {
  uint8_t operation_type;
  enum krbn_key_code key_code;
  enum krbn_event_type event_type;
  IOOptionBits flags;
};
