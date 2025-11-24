#pragma once

// `core_service_state_json_writer` can be used safely in a multi-threaded environment.

#include "constants.hpp"
#include "state_json_writer.hpp"
#include <optional>

namespace krbn {
namespace core_service {
class core_service_state_json_writer final {
public:
  core_service_state_json_writer(void) : state_json_writer_(constants::get_core_service_state_json_file_path()) {
    set_hid_device_open_permitted(std::nullopt);
    set_virtual_hid_device_service_client_connected(std::nullopt);
    set_driver_activated(std::nullopt);
    set_driver_connected(std::nullopt);
    set_driver_version_mismatched(std::nullopt);
  }

  void set_hid_device_open_permitted(std::optional<bool> value) {
    state_json_writer_.set("hid_device_open_permitted", value);
  }

  void set_virtual_hid_device_service_client_connected(std::optional<bool> value) {
    state_json_writer_.set("virtual_hid_device_service_client_connected", value);
  }

  void set_driver_activated(std::optional<bool> value) {
    state_json_writer_.set("driver_activated", value);
  }

  void set_driver_connected(std::optional<bool> value) {
    state_json_writer_.set("driver_connected", value);
  }

  void set_driver_version_mismatched(std::optional<bool> value) {
    state_json_writer_.set("driver_version_mismatched", value);
  }

private:
  krbn::state_json_writer state_json_writer_;
};
} // namespace core_service
} // namespace krbn
