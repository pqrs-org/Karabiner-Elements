#pragma once

class entry final {
public:
  std::pair<grabbable_state, ungrabbable_temporarily_reason> get_grabbable_state(void) const {
    // Ungrabbable while key repeating

    if (keyboard_repeat_detector_.is_repeating()) {
      return std::make_pair(grabbable_state::ungrabbable_temporarily,
                            ungrabbable_temporarily_reason::key_repeating);
    }

    // Ungrabbable while modifier keys are pressed
    //
    // We have to check the modifier keys state to avoid pressed physical modifiers affects in mouse events.
    // (See DEVELOPMENT.md > Modifier flags handling in kernel)

    if (!pressed_modifier_flags_.empty()) {
      return std::make_pair(grabbable_state::ungrabbable_temporarily,
                            ungrabbable_temporarily_reason::modifier_key_pressed);
    }

    // Ungrabbable while pointing button is pressed.
    //
    // We should not grab the device while a button is pressed since we cannot release the button.
    // (To release the button, we have to send a hid report to the device. But we cannot do it.)

    if (!pressed_pointing_buttons_.empty()) {
      return std::make_pair(grabbable_state::ungrabbable_temporarily,
                            ungrabbable_temporarily_reason::pointing_button_pressed);
    }

    return std::make_pair(grabbable_state::grabbable,
                          ungrabbable_temporarily_reason::none);
  }

  void update(const event_queue::queued_event& queued_event) {
    if (auto key_code = queued_event.get_event().get_key_code()) {
      if (auto hid_usage_page = types::make_hid_usage_page(*key_code)) {
        if (auto hid_usage = types::make_hid_usage(*key_code)) {
          keyboard_repeat_detector_.set(*hid_usage_page,
                                        *hid_usage,
                                        queued_event.get_event_type());

          if (auto m = types::make_modifier_flag(*hid_usage_page,
                                                 *hid_usage)) {
            if (queued_event.get_event_type() == event_type::key_down) {
              pressed_modifier_flags_.insert(*m);
            } else if (queued_event.get_event_type() == event_type::key_up) {
              pressed_modifier_flags_.erase(*m);
            }
          }
        }
      }
    }

    if (auto consumer_key_code = queued_event.get_event().get_consumer_key_code()) {
      if (auto hid_usage_page = types::make_hid_usage_page(*consumer_key_code)) {
        if (auto hid_usage = types::make_hid_usage(*consumer_key_code)) {
          keyboard_repeat_detector_.set(*hid_usage_page,
                                        *hid_usage,
                                        queued_event.get_event_type());
        }
      }
    }

    if (auto pointing_button = queued_event.get_event().get_pointing_button()) {
      if (queued_event.get_event_type() == event_type::key_down) {
        pressed_pointing_buttons_.insert(*pointing_button);
      } else if (queued_event.get_event_type() == event_type::key_up) {
        pressed_pointing_buttons_.erase(*pointing_button);
      }
    }
  }

private:
  keyboard_repeat_detector keyboard_repeat_detector_;
  std::unordered_set<modifier_flag> pressed_modifier_flags_;
  std::unordered_set<pointing_button> pressed_pointing_buttons_;
};
