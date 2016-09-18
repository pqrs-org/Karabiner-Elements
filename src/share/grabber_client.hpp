#pragma once

#include "constants.hpp"
#include "filesystem.hpp"
#include "local_datagram_client.hpp"
#include "types.hpp"
#include <unistd.h>
#include <vector>

class grabber_client final {
public:
  grabber_client(const grabber_client&) = delete;

  grabber_client(void) {
    if (!filesystem::exists(constants::get_grabber_socket_file_path())) {
      throw std::runtime_error("grabber socket is not found");
    }

    client_ = std::make_unique<local_datagram_client>(constants::get_grabber_socket_file_path());
  }

  void connect(krbn::connect_from connect_from) {
    krbn::operation_type_connect_struct s;
    s.connect_from = connect_from;
    s.pid = getpid();
    client_->send_to(reinterpret_cast<uint8_t*>(&s), sizeof(s));
  }

  void system_preferences_values_updated(const system_preferences::values values) {
    krbn::operation_type_system_preferences_values_updated_struct s;
    s.values = values;
    client_->send_to(reinterpret_cast<uint8_t*>(&s), sizeof(s));
  }

  void clear_simple_modifications(void) {
    krbn::operation_type_clear_simple_modifications_struct s;
    client_->send_to(reinterpret_cast<uint8_t*>(&s), sizeof(s));
  }

  void add_simple_modification(krbn::key_code from_key_code, krbn::key_code to_key_code) {
    krbn::operation_type_add_simple_modification_struct s;
    s.from_key_code = from_key_code;
    s.to_key_code = to_key_code;
    client_->send_to(reinterpret_cast<uint8_t*>(&s), sizeof(s));
  }

  void clear_fn_function_keys(void) {
    krbn::operation_type_clear_fn_function_keys_struct s;
    client_->send_to(reinterpret_cast<uint8_t*>(&s), sizeof(s));
  }

  void add_fn_function_key(krbn::key_code from_key_code, krbn::key_code to_key_code) {
    krbn::operation_type_add_fn_function_key_struct s;
    s.from_key_code = from_key_code;
    s.to_key_code = to_key_code;
    client_->send_to(reinterpret_cast<uint8_t*>(&s), sizeof(s));
  }

  void set_caps_lock_led_state(krbn::led_state led_state) {
    krbn::operation_type_set_caps_lock_led_state_struct s;
    s.led_state = led_state;
    client_->send_to(reinterpret_cast<uint8_t*>(&s), sizeof(s));
  }

private:
  std::unique_ptr<local_datagram_client> client_;
};
