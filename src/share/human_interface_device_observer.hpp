#pragma once

#include "boost_defs.hpp"

#include "human_interface_device.hpp"
#include <boost/signals2.hpp>

namespace krbn {
class human_interface_device_observer final {
public:
  boost::signals2::signal<void(const human_interface_device_observer&,
                               const hid_value&)>
      hid_value_arrived;

  human_interface_device_observer(const human_interface_device_observer&) = delete;

  human_interface_device_observer(IOHIDDeviceRef _Nonnull device) : human_interface_device_(device) {
    human_interface_device_.hid_values_arrived.connect([&](auto& human_interface_device,
                                                           auto& hid_values) {
      hid_values_arrived_callback(human_interface_device,
                                  hid_values);
    });

    gcd_utility::dispatch_sync_in_main_queue(^{
      auto r = human_interface_device_.open();
      if (r != kIOReturnSuccess) {
        logger::get_logger().error("IOHIDDeviceOpen error: {0} ({1}) {2}",
                                   iokit_utility::get_error_name(r),
                                   r,
                                   human_interface_device_.get_name_for_log());

      } else {
        human_interface_device_.queue_start();
        human_interface_device_.schedule();
      }
    });
  }

  ~human_interface_device_observer(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      human_interface_device_.unschedule();
      human_interface_device_.queue_stop();
      human_interface_device_.close();

      hid_values_arrived_connection.disconnect();
    });
  }

  void reset_is_grabbable_log_reducer(void) {
    is_grabbable_log_reducer_.reset();
  }

  human_interface_device::grabbable_state is_grabbable(bool verbose = true) const {
    // ----------------------------------------
    // Ungrabbable while key repeating

    if (keyboard_repeat_detector_.is_repeating()) {
      if (verbose) {
        is_grabbable_log_reducer_.warn(fmt::format("We cannot grab {0} while a key is repeating.", human_interface_device_.get_name_for_log()));
      }
      return human_interface_device::grabbable_state::ungrabbable_temporarily;
    }

    // Ungrabbable while modifier keys are pressed
    //
    // We have to check the modifier keys state to avoid pressed physical modifiers affects in mouse events.
    // (See DEVELOPMENT.md > Modifier flags handling in kernel)

    if (!pressed_modifier_flags_.empty()) {
      if (verbose) {
        std::stringstream ss;
        ss << "We cannot grab " << human_interface_device_.get_name_for_log() << " while any modifier flags are pressed. (";
        for (const auto& m : pressed_modifier_flags_) {
          ss << m << " ";
        }
        ss << ")";
        is_grabbable_log_reducer_.warn(ss.str());
      }
      return human_interface_device::grabbable_state::ungrabbable_temporarily;
    }

    // ----------------------------------------
    // Ungrabbable while pointing button is pressed.

    if (!pressed_pointing_buttons_.empty()) {
      // We should not grab the device while a button is pressed since we cannot release the button.
      // (To release the button, we have to send a hid report to the device. But we cannot do it.)

      if (verbose) {
        is_grabbable_log_reducer_.warn(fmt::format("We cannot grab {0} while mouse buttons are pressed.", human_interface_device_.get_name_for_log()));
      }
      return human_interface_device::grabbable_state::ungrabbable_temporarily;
    }

    // ----------------------------------------

    return human_interface_device::grabbable_state::grabbable;
  }

private:
  void hid_values_arrived_callback(human_interface_device& device,
                                   const std::vector<hid_value>& hid_values) {
    for (const auto& hv : hid_values) {
      if (auto key_code = types::make_key_code(hv)) {
        // Update keyboard_repeat_detector_

        keyboard_repeat_detector_.set(hv);

        // Update pressed_modifier_flags_

        if (auto m = types::make_modifier_flag(*key_code)) {
          if (hv.get_integer_value()) {
            pressed_modifier_flags_.insert(*m);
          } else {
            pressed_modifier_flags_.erase(*m);
          }
        }
      }

      if (auto consumer_key_code = types::make_consumer_key_code(hv)) {
        // Update keyboard_repeat_detector_

        keyboard_repeat_detector_.set(hv);
      }

      if (auto pointing_button = types::make_pointing_button(hv)) {
        // Update pressed_pointing_buttons_

        if (hv.get_integer_value()) {
          pressed_pointing_buttons_.insert(*pointing_button);
        } else {
          pressed_pointing_buttons_.erase(*pointing_button);
        }
      }

      hid_value_arrived(*this, hv);
    }
  }

  human_interface_device human_interface_device_;

  boost::signals2::connection hid_values_arrived_connection;

  keyboard_repeat_detector keyboard_repeat_detector_;
  std::unordered_set<modifier_flag> pressed_modifier_flags_;
  std::unordered_set<pointing_button> pressed_pointing_buttons_;

  mutable spdlog_utility::log_reducer is_grabbable_log_reducer_;
};
} // namespace krbn
