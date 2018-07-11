#pragma once

#include "boost_defs.hpp"

#include "gcd_utility.hpp"
#include "grabbable_state_manager.hpp"
#include "grabber_client.hpp"
#include "hid_manager.hpp"
#include "hid_observer.hpp"
#include "logger.hpp"
#include "types.hpp"

namespace krbn {
class device_observer final {
public:
  device_observer(const device_observer&) = delete;

  device_observer(const grabber_client& grabber_client) : grabber_client_(grabber_client),
                                                          hid_manager_({
                                                              std::make_pair(hid_usage_page::generic_desktop, hid_usage::gd_keyboard),
                                                              std::make_pair(hid_usage_page::generic_desktop, hid_usage::gd_mouse),
                                                              std::make_pair(hid_usage_page::generic_desktop, hid_usage::gd_pointer),
                                                          }) {
    grabbable_state_manager_.grabbable_state_changed.connect([this](auto&& registry_entry_id,
                                                                    auto&& grabbable_state,
                                                                    auto&& ungrabbable_temporarily_reason,
                                                                    auto&& time_stamp) {
      grabber_client_.grabbable_state_changed(registry_entry_id,
                                              grabbable_state,
                                              ungrabbable_temporarily_reason,
                                              time_stamp);
    });

    hid_manager_.device_detecting.connect([](auto&& device) {
      if (iokit_utility::is_karabiner_virtual_hid_device(device)) {
        return false;
      }

      iokit_utility::log_matching_device(device);

      return true;
    });

    hid_manager_.device_detected.connect([this](auto&& human_interface_device) {
      logger::get_logger().info("{0} is detected.", human_interface_device->get_name_for_log());

      grabbable_state_manager_.update(human_interface_device->get_registry_entry_id(),
                                      grabbable_state::device_error,
                                      ungrabbable_temporarily_reason::none,
                                      mach_absolute_time());

      human_interface_device->values_arrived.connect([this](auto&& human_interface_device,
                                                            auto&& event_queue) {
        grabbable_state_manager_.update(event_queue);
      });

      auto observer = std::make_shared<hid_observer>(human_interface_device);
      observer->device_observed.connect([this](auto&& human_interface_device) {
        logger::get_logger().info("{0} is observed.",
                                  human_interface_device->get_name_for_log());

        if (auto state = grabbable_state_manager_.get_grabbable_state(human_interface_device->get_registry_entry_id())) {
          // Keep grabbable_state if the state is already changed by value_callback.
          if (state->first == grabbable_state::device_error) {
            grabbable_state_manager_.update(human_interface_device->get_registry_entry_id(),
                                            grabbable_state::grabbable,
                                            ungrabbable_temporarily_reason::none,
                                            mach_absolute_time());
          }
        }
      });

      observer->observe();

      hid_observers_[human_interface_device->get_registry_entry_id()] = observer;
    });

    hid_manager_.device_removed.connect([this](auto&& human_interface_device) {
      logger::get_logger().info("{0} is removed.", human_interface_device->get_name_for_log());

      hid_observers_.erase(human_interface_device->get_registry_entry_id());
    });

    hid_manager_.start();
  }

  ~device_observer(void) {
    hid_observers_.clear();
    hid_manager_.stop();
  }

private:
  const grabber_client& grabber_client_;

  hid_manager hid_manager_;
  std::unordered_map<registry_entry_id, std::shared_ptr<hid_observer>> hid_observers_;
  grabbable_state_manager grabbable_state_manager_;
};
} // namespace krbn
