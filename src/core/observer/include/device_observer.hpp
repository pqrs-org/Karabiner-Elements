#pragma once

// `krbn::device_observer` can be used safely in a multi-threaded environment.

#include "boost_defs.hpp"

#include "dispatcher.hpp"
#include "grabbable_state_manager.hpp"
#include "grabber_client.hpp"
#include "hid_manager.hpp"
#include "hid_observer.hpp"
#include "logger.hpp"
#include "time_utility.hpp"
#include "types.hpp"

namespace krbn {
class device_observer final : public pqrs::dispatcher::dispatcher_client {
public:
  device_observer(const device_observer&) = delete;

  device_observer(std::weak_ptr<pqrs::dispatcher::dispatcher> weak_dispatcher,
                  std::weak_ptr<grabber_client> grabber_client) : dispatcher_client(weak_dispatcher),
                                                                  grabber_client_(grabber_client) {
    // grabbable_state_manager_

    grabbable_state_manager_ = std::make_unique<grabbable_state_manager>(weak_dispatcher);

    grabbable_state_manager_->grabbable_state_changed.connect([this](auto&& grabbable_state) {
      enqueue_to_dispatcher([this, grabbable_state] {
        if (auto client = grabber_client_.lock()) {
          client->async_grabbable_state_changed(grabbable_state);
        }
      });
    });

    // hid_manager_

    std::vector<std::pair<krbn::hid_usage_page, krbn::hid_usage>> targets({
        std::make_pair(hid_usage_page::generic_desktop, hid_usage::gd_keyboard),
        std::make_pair(hid_usage_page::generic_desktop, hid_usage::gd_mouse),
        std::make_pair(hid_usage_page::generic_desktop, hid_usage::gd_pointer),
    });

    hid_manager_ = std::make_unique<hid_manager>(targets);

    hid_manager_->device_detecting.connect([](auto&& device) {
      if (iokit_utility::is_karabiner_virtual_hid_device(device)) {
        return false;
      }

      iokit_utility::log_matching_device(device);

      return true;
    });

    hid_manager_->device_detected.connect([this](auto&& weak_hid) {
      enqueue_to_dispatcher([this, weak_hid] {
        if (auto hid = weak_hid.lock()) {
          logger::get_logger().info("{0} is detected.", hid->get_name_for_log());

          grabbable_state_manager_->update(grabbable_state(hid->get_registry_entry_id(),
                                                           grabbable_state::state::device_error,
                                                           grabbable_state::ungrabbable_temporarily_reason::none,
                                                           time_utility::mach_absolute_time()));

          hid->values_arrived.connect([this](auto&& shared_event_queue) {
            enqueue_to_dispatcher([this, shared_event_queue] {
              grabbable_state_manager_->update(*shared_event_queue);
            });
          });

          auto observer = std::make_shared<hid_observer>(weak_dispatcher_,
                                                         hid);

          observer->device_observed.connect([this, weak_hid] {
            enqueue_to_dispatcher([this, weak_hid] {
              if (auto hid = weak_hid.lock()) {
                logger::get_logger().info("{0} is observed.",
                                          hid->get_name_for_log());

                if (auto state = grabbable_state_manager_->get_grabbable_state(hid->get_registry_entry_id())) {
                  // Keep grabbable_state if the state is already changed by value_callback.
                  if (state->get_state() == grabbable_state::state::device_error) {
                    grabbable_state_manager_->update(grabbable_state(hid->get_registry_entry_id(),
                                                                     grabbable_state::state::grabbable,
                                                                     grabbable_state::ungrabbable_temporarily_reason::none,
                                                                     time_utility::mach_absolute_time()));
                  }
                }
              }
            });
          });

          observer->async_observe();

          hid_observers_[hid->get_registry_entry_id()] = observer;
        }
      });
    });

    hid_manager_->device_removed.connect([this](auto&& weak_hid) {
      enqueue_to_dispatcher([this, weak_hid] {
        if (auto hid = weak_hid.lock()) {
          logger::get_logger().info("{0} is removed.", hid->get_name_for_log());

          hid_observers_.erase(hid->get_registry_entry_id());
        }
      });
    });

    hid_manager_->async_start();

    logger::get_logger().info("device_observer is started.");
  }

  virtual ~device_observer(void) {
    detach_from_dispatcher([this] {
      hid_manager_ = nullptr;

      hid_observers_.clear();

      grabbable_state_manager_ = nullptr;
    });

    logger::get_logger().info("device_observer is stopped.");
  }

private:
  std::weak_ptr<grabber_client> grabber_client_;

  std::unique_ptr<hid_manager> hid_manager_;
  std::unordered_map<registry_entry_id, std::shared_ptr<hid_observer>> hid_observers_;
  std::unique_ptr<grabbable_state_manager> grabbable_state_manager_;
};
} // namespace krbn
