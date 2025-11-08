#pragma once

// `grabber_state_json_writer` can be used safely in a multi-threaded environment.

#include "constants.hpp"
#include "state_json_writer.hpp"
#include <optional>

namespace krbn {
namespace core_service {
class grabber_state_json_writer final {
public:
  grabber_state_json_writer(void) : state_json_writer_(constants::get_grabber_state_json_file_path()) {
    set_hid_device_open_permitted(std::nullopt);
    set_driver_activated(std::nullopt);
    set_driver_connected(std::nullopt);
    set_driver_version_mismatched(std::nullopt);
  }

  void set_hid_device_open_permitted(std::optional<bool> value) {
    state_json_writer_.set("hid_device_open_permitted", value);
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
