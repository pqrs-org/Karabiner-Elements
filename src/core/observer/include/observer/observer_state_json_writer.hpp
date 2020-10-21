#pragma once

// `observer_state_json_writer` can be used safely in a multi-threaded environment.

#include "constants.hpp"
#include "state_json_writer.hpp"

namespace krbn {
namespace observer {
class observer_state_json_writer final {
public:
  observer_state_json_writer(void) : state_json_writer_(constants::get_observer_state_json_file_path()) {
    set_hid_device_open_permitted(std::nullopt);
  }

  void set_hid_device_open_permitted(std::optional<bool> value) {
    state_json_writer_.set("hid_device_open_permitted", value);
  }

private:
  krbn::state_json_writer state_json_writer_;
};
} // namespace observer
} // namespace krbn
