#pragma once

// `krbn::device_observer` can be used safely in a multi-threaded environment.

#include "event_queue.hpp"
#include "grabbable_state_manager/manager.hpp"
#include "grabber_client.hpp"
#include "iokit_utility.hpp"
#include "logger.hpp"
#include "time_utility.hpp"
#include "types.hpp"
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/iokit_hid_manager.hpp>
#include <pqrs/osx/iokit_hid_queue_value_monitor.hpp>
#include <pqrs/osx/iokit_types.hpp>

namespace krbn {
class device_observer final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  device_observer(const device_observer&) = delete;

  device_observer(std::weak_ptr<grabber_client> grabber_client) : dispatcher_client(),
                                                                  grabber_client_(grabber_client) {
    // grabbable_state_manager_

    grabbable_state_manager_ = std::make_unique<grabbable_state_manager::manager>();

    grabbable_state_manager_->grabbable_state_changed.connect([this](auto&& grabbable_state) {
      if (auto client = grabber_client_.lock()) {
        client->async_grabbable_state_changed(grabbable_state);
      }
    });

    // hid_manager_

    std::vector<pqrs::cf::cf_ptr<CFDictionaryRef>> matching_dictionaries{
        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::osx::iokit_hid_usage_page_generic_desktop,
            pqrs::osx::iokit_hid_usage_generic_desktop_keyboard),

        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::osx::iokit_hid_usage_page_generic_desktop,
            pqrs::osx::iokit_hid_usage_generic_desktop_mouse),

        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::osx::iokit_hid_usage_page_generic_desktop,
            pqrs::osx::iokit_hid_usage_generic_desktop_pointer),
    };

    hid_manager_ = std::make_unique<pqrs::osx::iokit_hid_manager>(weak_dispatcher_,
                                                                  matching_dictionaries);

    hid_manager_->device_matched.connect([this](auto&& registry_entry_id, auto&& device_ptr) {
      if (device_ptr) {
        iokit_utility::log_matching_device(registry_entry_id, *device_ptr);

        auto device_id = make_device_id(registry_entry_id);
        auto device_name = iokit_utility::make_device_name_for_log(device_id,
                                                                   *device_ptr);

        grabbable_state_manager_->update(grabbable_state(device_id,
                                                         grabbable_state::state::device_error,
                                                         grabbable_state::ungrabbable_temporarily_reason::none,
                                                         time_utility::mach_absolute_time_point()));

        auto hid_queue_value_monitor = std::make_shared<pqrs::osx::iokit_hid_queue_value_monitor>(weak_dispatcher_,
                                                                                                  *device_ptr);
        hid_queue_value_monitors_[device_id] = hid_queue_value_monitor;

        if (iokit_utility::is_karabiner_virtual_hid_device(*device_ptr)) {
          // Handle caps_lock_state_changed event only if the hid is Karabiner-VirtualHIDDevice.
          hid_queue_value_monitor->values_arrived.connect([this, device_id](auto&& values_ptr) {
            auto event_queue = event_queue::utility::make_queue(device_id,
                                                                iokit_utility::make_hid_values(values_ptr));

            for (const auto& e : event_queue->get_entries()) {
              if (e.get_event().get_type() == event_queue::event::type::caps_lock_state_changed) {
                if (auto client = grabber_client_.lock()) {
                  if (auto state = e.get_event().get_integer_value()) {
                    client->async_caps_lock_state_changed(*state);
                  }
                }
              }
            }
          });
        } else {
          hid_queue_value_monitor->values_arrived.connect([this, device_id](auto&& values_ptr) {
            auto event_queue = event_queue::utility::make_queue(device_id,
                                                                iokit_utility::make_hid_values(values_ptr));
            grabbable_state_manager_->update(*event_queue);
          });
        }

        hid_queue_value_monitor->started.connect([this, device_id, device_name] {
          logger::get_logger()->info("{0} is observed.", device_name);

          if (auto state = grabbable_state_manager_->get_grabbable_state(device_id)) {
            // Keep grabbable_state if the state is already changed by value_callback.
            if (state->get_state() == grabbable_state::state::device_error) {
              grabbable_state_manager_->update(grabbable_state(device_id,
                                                               grabbable_state::state::grabbable,
                                                               grabbable_state::ungrabbable_temporarily_reason::none,
                                                               time_utility::mach_absolute_time_point()));
            }
          }
        });

        hid_queue_value_monitor->async_start(kIOHIDOptionsTypeNone,
                                             std::chrono::milliseconds(3000));
      }
    });

    hid_manager_->device_terminated.connect([this](auto&& registry_entry_id) {
      auto device_id = make_device_id(registry_entry_id);

      logger::get_logger()->info("device_id:{0} is terminated.", type_safe::get(device_id));

      hid_queue_value_monitors_.erase(device_id);
    });

    hid_manager_->error_occurred.connect([](auto&& message, auto&& iokit_return) {
      logger::get_logger()->error("{0}: {1}", message, iokit_return.to_string());
    });

    hid_manager_->async_start();

    logger::get_logger()->info("device_observer is started.");
  }

  virtual ~device_observer(void) {
    detach_from_dispatcher([this] {
      hid_manager_ = nullptr;
      hid_queue_value_monitors_.clear();
      grabbable_state_manager_ = nullptr;
    });

    logger::get_logger()->info("device_observer is stopped.");
  }

  void async_post_all_states_to_grabber(void) {
    enqueue_to_dispatcher([this] {
      if (auto client = grabber_client_.lock()) {
        if (grabbable_state_manager_) {
          for (const auto& s : grabbable_state_manager_->make_grabbable_states()) {
            client->async_grabbable_state_changed(s);
          }
        }
      }
    });
  }

private:
  std::weak_ptr<grabber_client> grabber_client_;

  std::unique_ptr<pqrs::osx::iokit_hid_manager> hid_manager_;
  std::unordered_map<device_id, std::shared_ptr<pqrs::osx::iokit_hid_queue_value_monitor>> hid_queue_value_monitors_;
  std::unique_ptr<grabbable_state_manager::manager> grabbable_state_manager_;
};
} // namespace krbn
