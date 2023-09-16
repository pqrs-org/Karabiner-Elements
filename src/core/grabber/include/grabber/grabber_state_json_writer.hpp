#pragma once

// `grabber_state_json_writer` can be used safely in a multi-threaded environment.

#include "constants.hpp"
#include "state_json_writer.hpp"
#include <optional>

namespace krbn {
namespace grabber {
class grabber_state_json_writer final {
public:
  grabber_state_json_writer(void) : state_json_writer_(constants::get_grabber_state_json_file_path()) {
    set_hid_device_open_permitted(std::nullopt);
    set_driver_loaded(std::nullopt);
    set_driver_version_matched(std::nullopt);
  }

  void set_hid_device_open_permitted(std::optional<bool> value) {
    state_json_writer_.set("hid_device_open_permitted", value);
  }

  void set_driver_loaded(std::optional<bool> value) {
    state_json_writer_.set("driver_loaded", value);
  }

  void set_driver_version_matched(std::optional<bool> value) {
    state_json_writer_.set("driver_version_matched", value);
  }

private:
  krbn::state_json_writer state_json_writer_;
};
} // namespace grabber
} // namespace krbn
