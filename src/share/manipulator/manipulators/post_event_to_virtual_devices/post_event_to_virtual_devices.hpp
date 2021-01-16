#pragma once

#include "../../types.hpp"
#include "../base.hpp"
#include "console_user_server_client.hpp"
#include "key_event_dispatcher.hpp"
#include "keyboard_repeat_detector.hpp"
#include "krbn_notification_center.hpp"
#include "mouse_key_handler.hpp"
#include "queue.hpp"
#include "types.hpp"
#include <pqrs/karabiner/driverkit/virtual_hid_device_service.hpp>

namespace krbn {
namespace manipulator {
namespace manipulators {
namespace post_event_to_virtual_devices {
class post_event_to_virtual_devices final : public base, public pqrs::dispatcher::extra::dispatcher_client {
public:
  post_event_to_virtual_devices(std::weak_ptr<console_user_server_client> weak_console_user_server_client) : base(),
                                                                                                             dispatcher_client(),
                                                                                                             weak_console_user_server_client_(weak_console_user_server_client) {
    mouse_key_handler_ = std::make_unique<mouse_key_handler>(queue_);
  }

  virtual ~post_event_to_virtual_devices(void) {
    detach_from_dispatcher([this] {
      mouse_key_handler_ = nullptr;
    });
  }

  virtual bool already_manipulated(const event_queue::entry& front_input_event) {
    return false;
  }

  virtual manipulate_result manipulate(event_queue::entry& front_input_event,
                                       const event_queue::queue& input_event_queue,
                                       std::shared_ptr<event_queue::queue> output_event_queue,
                                       absolute_time_point now) {
    if (output_event_queue) {
      if (front_input_event.get_validity() == validity::invalid) {
        return manipulate_result::passed;
      }

      // We have to synchronize the caps_lock state in the following variables when caps lock state is changed.
      // - output_event_queue::modifier_flag_manager_
      // - key_event_dispatcher_.pressed_modifier_flags_
      if (front_input_event.get_event().get_type() == event_queue::event::type::caps_lock_state_changed) {
        // We should synchronize the key_event_dispatcher_ state
        // in order to send caps_lock key event properly when caps_lock is used as modifiers.mandatory.
        //
        // Example: caps_lock+f9 -> f10
        //
        // 1. caps_lock key_down
        // 2. caps_lock key_up
        // 3. caps_lock_state_changed(on)
        // 4. f9 key_down
        // 5. set_modifier_flag_lock_state (caps_lock off)
        //    (from modifiers.mandatory)
        //
        // Without synchronization, the caps_lock event will not be sent at (5)
        // because the caps lock state of key_event_dispatcher_ is off.
        // We have to set the state on at (3).
        //
        // Note:
        // We should not change the key_event_dispatcher_ state while modifier_flag_manager contains flags except led_lock.
        // The modifiers.mandatory pushes sticky_modifier to modifier_flag_manager.
        // While modifiers.mandatory controls modifier_flag_manager, key_event_dispatcher_ should not be updated by LED state.
        // (The behavior is described in docs/DEVELOPMENT.md.)

        if (output_event_queue->get_modifier_flag_manager().sticky_size(modifier_flag::caps_lock) == 0) {
          if (auto state = front_input_event.get_event().get_integer_value()) {
            if (*state) {
              key_event_dispatcher_.insert_pressed_modifier_flag(modifier_flag::caps_lock);
            } else {
              key_event_dispatcher_.erase_pressed_modifier_flag(modifier_flag::caps_lock);
            }
          }
        }
      }

      // Dispatch modifier key event only when front_input_event is key_down or modifier key.

      bool dispatch_modifier_key_event = false;
      bool dispatch_modifier_key_event_before = false;
      bool front_input_event_modifier_key_event = false;

      if (auto e = front_input_event.get_event().get_if<momentary_switch_event>()) {
        if (e->modifier_flag()) {
          front_input_event_modifier_key_event = true;
        }
      }

      if (front_input_event_modifier_key_event) {
        // front_input_event is modifier key event.
        if (!front_input_event.get_lazy()) {
          dispatch_modifier_key_event = true;
          dispatch_modifier_key_event_before = false;
        }

      } else {
        if (front_input_event.get_event_type() == event_type::key_down) {
          dispatch_modifier_key_event = true;
          dispatch_modifier_key_event_before = true;

        } else if (front_input_event.get_event().get_type() == event_queue::event::type::pointing_motion) {
          // We should not dispatch modifier key events while key repeating.
          // (See comments in `handle_pointing_device_event_from_event_tap` for details.)
          if (!queue_.get_keyboard_repeat_detector().is_repeating() &&
              pressed_buttons_.empty() &&
              !mouse_key_handler_->active()) {
            dispatch_modifier_key_event = true;
            dispatch_modifier_key_event_before = true;
          }
        }
      }

      if (dispatch_modifier_key_event &&
          dispatch_modifier_key_event_before) {
        key_event_dispatcher_.dispatch_modifier_key_event(output_event_queue->get_modifier_flag_manager(),
                                                          queue_,
                                                          front_input_event.get_event_time_stamp().get_time_stamp());
      }

      // Note: output_event_queue::modifier_flag_manager_ will be changed by push_back_entry.
      output_event_queue->push_back_entry(front_input_event);
      front_input_event.set_validity(validity::invalid);

      switch (front_input_event.get_event().get_type()) {
        case event_queue::event::type::momentary_switch_event:
          if (auto e = front_input_event.get_event().get_if<momentary_switch_event>()) {
            if (e->valid()) {
              if (e->pointing_button()) {
                post_pointing_input_report(front_input_event,
                                           output_event_queue);

              } else if (!front_input_event_modifier_key_event) {
                switch (front_input_event.get_event_type()) {
                  case event_type::key_down:
                    key_event_dispatcher_.dispatch_key_down_event(front_input_event.get_device_id(),
                                                                  e->get_usage_pair(),
                                                                  queue_,
                                                                  front_input_event.get_event_time_stamp().get_time_stamp());
                    break;

                  case event_type::key_up:
                    key_event_dispatcher_.dispatch_key_up_event(e->get_usage_pair(),
                                                                queue_,
                                                                front_input_event.get_event_time_stamp().get_time_stamp());
                    break;

                  case event_type::single:
                    break;
                }
              }
            }
          }
          break;

        case event_queue::event::type::pointing_motion:
          post_pointing_input_report(front_input_event,
                                     output_event_queue);
          break;

        case event_queue::event::type::shell_command:
          if (auto shell_command = front_input_event.get_event().get_shell_command()) {
            if (front_input_event.get_event_type() == event_type::key_down) {
              queue_.push_back_shell_command_event(*shell_command,
                                                   front_input_event.get_event_time_stamp().get_time_stamp());
            }
          }
          break;

        case event_queue::event::type::select_input_source:
          if (auto input_source_specifiers = front_input_event.get_event().get_input_source_specifiers()) {
            if (front_input_event.get_event_type() == event_type::key_down) {
              queue_.push_back_select_input_source_event(*input_source_specifiers,
                                                         front_input_event.get_event_time_stamp().get_time_stamp());
            }
          }
          break;

        case event_queue::event::type::mouse_key:
          if (auto mouse_key = front_input_event.get_event().get_mouse_key()) {
            if (front_input_event.get_event_type() == event_type::key_down) {
              mouse_key_handler_->push_back_mouse_key(front_input_event.get_device_id(),
                                                      *mouse_key,
                                                      output_event_queue,
                                                      front_input_event.get_event_time_stamp().get_time_stamp());
            } else {
              mouse_key_handler_->erase_mouse_key(front_input_event.get_device_id(),
                                                  *mouse_key,
                                                  output_event_queue,
                                                  front_input_event.get_event_time_stamp().get_time_stamp());
            }
          }
          break;

        case event_queue::event::type::stop_keyboard_repeat:
          if (auto e = queue_.get_keyboard_repeat_detector().get_repeating_key()) {
            key_event_dispatcher_.dispatch_key_up_event(e->get_usage_pair(),
                                                        queue_,
                                                        front_input_event.get_event_time_stamp().get_time_stamp());
          }
          break;

        case event_queue::event::type::system_preferences_properties_changed:
          if (auto properties = front_input_event.get_event().get_if<pqrs::osx::system_preferences::properties>()) {
            mouse_key_handler_->set_system_preferences_properties(*properties);
          }
          break;

        case event_queue::event::type::virtual_hid_keyboard_configuration_changed:
          if (auto c = front_input_event.get_event().get_if<core_configuration::details::virtual_hid_keyboard>()) {
            mouse_key_handler_->set_virtual_hid_keyboard_configuration(*c);
          }
          break;

        case event_queue::event::type::none:
        case event_queue::event::type::set_variable:
        case event_queue::event::type::sticky_modifier:
        case event_queue::event::type::device_keys_and_pointing_buttons_are_released:
        case event_queue::event::type::device_grabbed:
        case event_queue::event::type::device_ungrabbed:
        case event_queue::event::type::caps_lock_state_changed:
        case event_queue::event::type::pointing_device_event_from_event_tap:
        case event_queue::event::type::frontmost_application_changed:
        case event_queue::event::type::input_source_changed:
          // Do nothing
          break;
      }

      if (dispatch_modifier_key_event &&
          !dispatch_modifier_key_event_before) {
        key_event_dispatcher_.dispatch_modifier_key_event(output_event_queue->get_modifier_flag_manager(),
                                                          queue_,
                                                          front_input_event.get_event_time_stamp().get_time_stamp());
      }
    }

    return manipulate_result::passed;
  }

  virtual bool active(void) const {
    return !queue_.empty();
  }

  virtual bool needs_virtual_hid_pointing(void) const {
    return false;
  }

  virtual void handle_device_keys_and_pointing_buttons_are_released_event(const event_queue::entry& front_input_event,
                                                                          event_queue::queue& output_event_queue) {
    // modifier flags

    key_event_dispatcher_.dispatch_modifier_key_event(output_event_queue.get_modifier_flag_manager(),
                                                      queue_,
                                                      front_input_event.get_event_time_stamp().get_time_stamp());

    // We have to keep sticky modifiers state except the caps lock
    // in order to keep the sticky modifiers state after all keys are released.
    //
    // About the exception of caps lock:
    // The caps lock will be handled as "toggle" in above dispatch_modifier_key_event.
    // In the following case, unless erasing stikcy modifiers,
    // the caps lock sticky modifier will be erased at (3) and
    // caps lock event is not sent at (4).
    //
    // (1) sticky_modifier caps_lock true
    // (2) device_keys_and_pointing_buttons_are_released
    //     - usage::caps_lock is sent to turn caps lock on.
    // (3) sticky_modifier caps_lock false
    // (4) Another key is pressed
    //     - We need to send usage::caps_lock at here.

    output_event_queue.get_modifier_flag_manager().erase_caps_lock_sticky_modifier_flags();

    // pointing buttons

    {
      auto buttons = output_event_queue.get_pointing_button_manager().make_hid_report_buttons();
      if (pressed_buttons_ != buttons) {
        pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::pointing_input report;
        report.buttons = buttons;
        queue_.emplace_back_pointing_input(report,
                                           event_type::key_up,
                                           front_input_event.get_event_time_stamp().get_time_stamp());

        // Save buttons for `handle_device_ungrabbed_event`.
        pressed_buttons_ = buttons;
      }
    }

    // mouse keys

    mouse_key_handler_->erase_mouse_keys_by_device_id(front_input_event.get_device_id(),
                                                      front_input_event.get_event_time_stamp().get_time_stamp());
  }

  virtual void handle_device_ungrabbed_event(device_id device_id,
                                             const event_queue::queue& output_event_queue,
                                             absolute_time_point time_stamp) {
    // Release pressed keys

    key_event_dispatcher_.dispatch_key_up_events_by_device_id(device_id,
                                                              queue_,
                                                              time_stamp);

    // Release buttons
    {
      auto buttons = output_event_queue.get_pointing_button_manager().make_hid_report_buttons();
      if (pressed_buttons_ != buttons) {
        pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::pointing_input report;
        report.buttons = buttons;
        queue_.emplace_back_pointing_input(report,
                                           event_type::key_up,
                                           time_stamp);

        pressed_buttons_ = buttons;
      }
    }

    // Release modifiers

    key_event_dispatcher_.dispatch_modifier_key_event(output_event_queue.get_modifier_flag_manager(),
                                                      queue_,
                                                      time_stamp);

    // Release mouse_key_handler_

    mouse_key_handler_->erase_mouse_keys_by_device_id(device_id,
                                                      time_stamp);
  }

  virtual void handle_pointing_device_event_from_event_tap(const event_queue::entry& front_input_event,
                                                           event_queue::queue& output_event_queue) {
    // We should not dispatch modifier key events while key repeating.
    //
    // macOS does not ignore the modifier state change while key repeating.
    // If you enabled `control-f -> right_arrow` configuration,
    // apps will catch control-right_arrow event if release the lazy modifier here while right_arrow key is repeating.

    // We should not dispatch modifier key events while mouse keys active.
    //
    // For example, we should not restore right_shift by physical mouse movement when we use "change right_shift+r to scroll",
    // because the right_shift key_up event interrupt scroll event.

    // We should not dispatch modifier key events while mouse buttons are pressed.
    // If you enabled `option-f -> button1` configuration,
    // apps will catch option-button1 event if release the lazy modifier here while button1 is pressed.

    switch (front_input_event.get_event_type()) {
      case event_type::key_down:
      case event_type::single:
        if (!queue_.get_keyboard_repeat_detector().is_repeating() &&
            pressed_buttons_.empty() &&
            !mouse_key_handler_->active()) {
          key_event_dispatcher_.dispatch_modifier_key_event(output_event_queue.get_modifier_flag_manager(),
                                                            queue_,
                                                            front_input_event.get_event_time_stamp().get_time_stamp());
        }
        break;

      case event_type::key_up:
        break;
    }
  }

  virtual void set_validity(validity value) {
    // This manipulator is always valid.
  }

  void async_post_events(std::weak_ptr<pqrs::karabiner::driverkit::virtual_hid_device_service::client> weak_virtual_hid_device_service_client) {
    enqueue_to_dispatcher(
        [this, weak_virtual_hid_device_service_client] {
          queue_.async_post_events(weak_virtual_hid_device_service_client,
                                   weak_console_user_server_client_);
        });
  }

  const queue& get_queue(void) const {
    return queue_;
  }

  void clear_queue(void) {
    return queue_.clear();
  }

  const key_event_dispatcher& get_key_event_dispatcher(void) const {
    return key_event_dispatcher_;
  }

private:
  void post_pointing_input_report(event_queue::entry& front_input_event,
                                  std::shared_ptr<event_queue::queue> output_event_queue) {
    pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::pointing_input report;
    report.buttons = output_event_queue->get_pointing_button_manager().make_hid_report_buttons();

    if (auto pointing_motion = front_input_event.get_event().get_pointing_motion()) {
      report.x = pointing_motion->get_x();
      report.y = pointing_motion->get_y();
      report.vertical_wheel = pointing_motion->get_vertical_wheel();
      report.horizontal_wheel = pointing_motion->get_horizontal_wheel();
    }

    queue_.emplace_back_pointing_input(report,
                                       front_input_event.get_event_type(),
                                       front_input_event.get_event_time_stamp().get_time_stamp());

    // Save buttons for `handle_device_ungrabbed_event`.
    pressed_buttons_ = report.buttons;
  }

  std::weak_ptr<console_user_server_client> weak_console_user_server_client_;

  queue queue_;
  key_event_dispatcher key_event_dispatcher_;
  std::unique_ptr<mouse_key_handler> mouse_key_handler_;
  std::unordered_set<modifier_flag> pressed_modifier_flags_;
  pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::buttons pressed_buttons_;
};
} // namespace post_event_to_virtual_devices
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
