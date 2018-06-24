#pragma once

#include "boost_defs.hpp"

#include "gcd_utility.hpp"
#include "grabbable_state_manager.hpp"
#include "grabber_client.hpp"
#include "hid_manager.hpp"
#include "logger.hpp"
#include "types.hpp"

namespace krbn {
class device_observer final {
public:
  device_observer(const device_observer&) = delete;

  device_observer(const grabber_client& grabber_client) : grabber_client_(grabber_client) {
    grabbable_state_manager_.grabbable_state_updated.connect([&](auto&& registry_entry_id,
                                                                 auto&& grabbable_state,
                                                                 auto&& ungrabbable_temporarily_reason,
                                                                 auto&& time_stamp) {
      grabber_client_.grabbable_state_updated(registry_entry_id,
                                              grabbable_state,
                                              ungrabbable_temporarily_reason,
                                              time_stamp);
    });

    hid_manager_.device_detecting.connect([](auto&& device) {
      if (iokit_utility::is_karabiner_virtual_hid_device(device)) {
        return false;
      }
      return true;
    });

    hid_manager_.device_detected.connect([&](auto&& human_interface_device) {
      human_interface_device.set_value_callback([&](auto&& human_interface_device,
                                                    auto&& event_queue) {
        value_callback(human_interface_device, event_queue);
      });
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

  const grabber_client& grabber_client_;

  hid_manager hid_manager_;
  grabbable_state_manager grabbable_state_manager_;
};
} // namespace krbn
