#include "hid_manager.hpp"
#include "libkrbn.h"
#include <mutex>

namespace {
class libkrbn_hid_value_observer_class final {
public:
  libkrbn_hid_value_observer_class(const libkrbn_hid_value_observer_class&) = delete;

  libkrbn_hid_value_observer_class(libkrbn_hid_value_observer_callback callback,
                                   void* refcon) : callback_(callback),
                                                   refcon_(refcon),
                                                   observed_device_count_(0) {
    hid_manager_.device_detected.connect([this](auto&& human_interface_device) {
      human_interface_device.set_value_callback([this](auto&& human_interface_device,
                                                       auto&& event_queue) {
        value_callback(human_interface_device, event_queue);
      });
      human_interface_device.observe();

      std::lock_guard<std::mutex> lock(observed_device_count_mutex_);
      ++observed_device_count_;
    });

    hid_manager_.device_removed.connect([this](auto&& registry_entry_id,
                                               auto&& human_interface_device) {
      std::lock_guard<std::mutex> lock(observed_device_count_mutex_);
      --observed_device_count_;
    });

    hid_manager_.start({
        std::make_pair(krbn::hid_usage_page::generic_desktop, krbn::hid_usage::gd_keyboard),
    });
  }

  ~libkrbn_hid_value_observer_class(void) {
    hid_manager_.stop();
  }

  size_t calculate_observed_device_count(void) const {
    std::lock_guard<std::mutex> lock(observed_device_count_mutex_);
    return observed_device_count_;
  }

private:
  void value_callback(krbn::human_interface_device& device,
                      krbn::event_queue& event_queue) {
    for (const auto& queued_event : event_queue.get_events()) {
      libkrbn_hid_value_event_type event_type = libkrbn_hid_value_event_type_key_down;
      switch (queued_event.get_event_type()) {
        case krbn::event_type::key_down:
          event_type = libkrbn_hid_value_event_type_key_down;
          break;
        case krbn::event_type::key_up:
          event_type = libkrbn_hid_value_event_type_key_up;
          break;
        case krbn::event_type::single:
          event_type = libkrbn_hid_value_event_type_single;
          break;
      }

      switch (queued_event.get_event().get_type()) {
        case krbn::event_queue::queued_event::event::type::key_code:
          if (auto key_code = queued_event.get_event().get_key_code()) {
            callback_(libkrbn_hid_value_type_key_code,
                      static_cast<uint32_t>(*key_code),
                      event_type,
                      refcon_);
          }
          break;

        case krbn::event_queue::queued_event::event::type::consumer_key_code:
          if (auto consumer_key_code = queued_event.get_event().get_consumer_key_code()) {
            callback_(libkrbn_hid_value_type_consumer_key_code,
                      static_cast<uint32_t>(*consumer_key_code),
                      event_type,
                      refcon_);
          }
          break;

        case krbn::event_queue::queued_event::event::type::none:
        case krbn::event_queue::queued_event::event::type::pointing_button:
        case krbn::event_queue::queued_event::event::type::pointing_motion:
        case krbn::event_queue::queued_event::event::type::shell_command:
        case krbn::event_queue::queued_event::event::type::select_input_source:
        case krbn::event_queue::queued_event::event::type::set_variable:
        case krbn::event_queue::queued_event::event::type::mouse_key:
        case krbn::event_queue::queued_event::event::type::stop_keyboard_repeat:
        case krbn::event_queue::queued_event::event::type::device_keys_and_pointing_buttons_are_released:
        case krbn::event_queue::queued_event::event::type::device_ungrabbed:
        case krbn::event_queue::queued_event::event::type::caps_lock_state_changed:
        case krbn::event_queue::queued_event::event::type::event_from_ignored_device:
        case krbn::event_queue::queued_event::event::type::pointing_device_event_from_event_tap:
        case krbn::event_queue::queued_event::event::type::frontmost_application_changed:
        case krbn::event_queue::queued_event::event::type::input_source_changed:
        case krbn::event_queue::queued_event::event::type::keyboard_type_changed:
          break;
      }
    }

    event_queue.clear_events();
  }

  libkrbn_hid_value_observer_callback callback_;
  void* refcon_;

  krbn::hid_manager hid_manager_;
  size_t observed_device_count_;
  mutable std::mutex observed_device_count_mutex_;
};
} // namespace

bool libkrbn_hid_value_observer_initialize(libkrbn_hid_value_observer** out, libkrbn_hid_value_observer_callback callback, void* refcon) {
  if (!out) return false;
  // return if already initialized.
  if (*out) return false;

  *out = reinterpret_cast<libkrbn_hid_value_observer*>(new libkrbn_hid_value_observer_class(callback, refcon));
  return true;
}

void libkrbn_hid_value_observer_terminate(libkrbn_hid_value_observer** p) {
  if (p && *p) {
    delete reinterpret_cast<libkrbn_hid_value_observer_class*>(*p);
    *p = nullptr;
  }
}

size_t libkrbn_hid_value_observer_calculate_observed_device_count(libkrbn_hid_value_observer* p) {
  if (p) {
    if (auto o = reinterpret_cast<libkrbn_hid_value_observer_class*>(p)) {
      return o->calculate_observed_device_count();
    }
  }
  return 0;
}
