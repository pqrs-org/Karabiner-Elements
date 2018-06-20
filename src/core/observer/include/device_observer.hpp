#pragma once

#include "boost_defs.hpp"

#include "gcd_utility.hpp"
#include "grabbable_state_manager.hpp"
#include "hid_manager.hpp"
#include "human_interface_device.hpp"
#include "iokit_utility.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <json/json.hpp>
#include <thread>
#include <time.h>

namespace krbn {
class device_observer final {
public:
  device_observer(const device_observer&) = delete;

  device_observer(void) {
    hid_manager_.device_detecting.connect([](auto&& device) {
      if (iokit_utility::is_karabiner_virtual_hid_device(device)) {
        return false;
      }
      return true;
    });

    hid_manager_.device_detected.connect([&](auto&& human_interface_device) {
      human_interface_device.set_value_callback(std::bind(&device_observer::value_callback,
                                                          this,
                                                          std::placeholders::_1,
                                                          std::placeholders::_2));
      human_interface_device.observe();
    });

    hid_manager_.start({
        std::make_pair(hid_usage_page::generic_desktop, hid_usage::gd_keyboard),
        std::make_pair(hid_usage_page::generic_desktop, hid_usage::gd_mouse),
        std::make_pair(hid_usage_page::generic_desktop, hid_usage::gd_pointer),
    });
  }

  ~device_observer(void) {
    hid_manager_.stop();
  }

private:
  void value_callback(human_interface_device& device,
                      event_queue& event_queue) {
    grabbable_state_manager_.update(event_queue);
  }

  hid_manager hid_manager_;
  grabbable_state_manager grabbable_state_manager_;
};
} // namespace krbn
