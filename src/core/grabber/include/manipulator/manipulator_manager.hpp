#pragma once

#include "manipulator/manipulator_factory.hpp"

namespace krbn {
namespace manipulator {
class manipulator_manager final {
public:
  manipulator_manager(const manipulator_manager&) = delete;

  manipulator_manager(void) {
    manipulator_timer_connection_ = manipulator_timer::get_instance().timer_invoked.connect([&](auto timer_id) {
      for (auto&& m : manipulators_) {
        m->manipulator_timer_invoked(timer_id);
      }
    });
  }

  ~manipulator_manager(void) {
    manipulator_timer_connection_.disconnect();
  }

  void push_back_manipulator(const nlohmann::json& json,
                             const core_configuration::profile::complex_modifications::parameters& parameters) {
    manipulators_.push_back(manipulator_factory::make_manipulator(json, parameters));
  }

  void push_back_manipulator(std::shared_ptr<details::base> ptr) {
    manipulators_.push_back(ptr);
  }

  void manipulate(const std::shared_ptr<event_queue>& input_event_queue,
                  const std::shared_ptr<event_queue>& output_event_queue) {
    if (input_event_queue &&
        output_event_queue) {
      while (!input_event_queue->empty()) {
        auto& front_input_event = input_event_queue->get_front_event();

        switch (front_input_event.get_event().get_type()) {
          case event_queue::queued_event::event::type::device_keys_are_released:
            output_event_queue->erase_all_active_modifier_flags_except_lock(front_input_event.get_device_id());

            for (auto&& m : manipulators_) {
              m->force_post_modifier_key_event(front_input_event,
                                               *output_event_queue);
            }
            break;

          case event_queue::queued_event::event::type::device_pointing_buttons_are_released:
            output_event_queue->erase_all_active_pointing_buttons_except_lock(front_input_event.get_device_id());

            for (auto&& m : manipulators_) {
              m->force_post_pointing_button_event(front_input_event,
                                                  *output_event_queue);
            }
            break;

          case event_queue::queued_event::event::type::device_ungrabbed:
            // Reset modifier_flags and pointing_buttons before `handle_device_ungrabbed_event`
            // in order to send key_up events in `post_event_to_virtual_devices::handle_device_ungrabbed_event`.
            output_event_queue->erase_all_active_modifier_flags(front_input_event.get_device_id());
            output_event_queue->erase_all_active_pointing_buttons(front_input_event.get_device_id());
            for (auto&& m : manipulators_) {
              m->handle_device_ungrabbed_event(front_input_event.get_device_id(),
                                               *output_event_queue,
                                               front_input_event.get_time_stamp());
            }
            break;

          case event_queue::queued_event::event::type::event_from_ignored_device:
            for (auto&& m : manipulators_) {
              m->handle_event_from_ignored_device(front_input_event,
                                                  *output_event_queue);
            }
            break;

          case event_queue::queued_event::event::type::pointing_device_event_from_event_tap:
            for (auto&& m : manipulators_) {
              m->handle_pointing_device_event_from_event_tap(front_input_event,
                                                             *output_event_queue);
            }
            break;

          case event_queue::queued_event::event::type::none:
          case event_queue::queued_event::event::type::caps_lock_state_changed:
          case event_queue::queued_event::event::type::frontmost_application_changed:
          case event_queue::queued_event::event::type::input_source_changed:
          case event_queue::queued_event::event::type::keyboard_type_changed:
          case event_queue::queued_event::event::type::set_variable:
          case event_queue::queued_event::event::type::mouse_key:
            // Do nothing
            break;

          case event_queue::queued_event::event::type::key_code:
          case event_queue::queued_event::event::type::consumer_key_code:
          case event_queue::queued_event::event::type::pointing_button:
          case event_queue::queued_event::event::type::pointing_x:
          case event_queue::queued_event::event::type::pointing_y:
          case event_queue::queued_event::event::type::pointing_vertical_wheel:
          case event_queue::queued_event::event::type::pointing_horizontal_wheel:
          case event_queue::queued_event::event::type::shell_command:
          case event_queue::queued_event::event::type::select_input_source:
            for (auto&& m : manipulators_) {
              m->manipulate(front_input_event,
                            *input_event_queue,
                            output_event_queue);
            }
            break;
        }

        if (input_event_queue->get_front_event().get_valid()) {
          output_event_queue->push_back_event(input_event_queue->get_front_event());
        }

        input_event_queue->erase_front_event();
      }

      remove_invalid_manipulators();
    }
  }

  void invalidate_manipulators(void) {
    for (auto&& m : manipulators_) {
      m->set_valid(false);
    }

    remove_invalid_manipulators();
  }

  size_t get_manipulators_size(void) {
    return manipulators_.size();
  }

  bool needs_virtual_hid_pointing(void) const {
    for (auto&& m : manipulators_) {
      if (m->needs_virtual_hid_pointing()) {
        return true;
      }
    }
    return false;
  }

private:
  void remove_invalid_manipulators(void) {
    manipulators_.erase(std::remove_if(std::begin(manipulators_),
                                       std::end(manipulators_),
                                       [](const auto& it) {
                                         // Keep active manipulators.
                                         return !it->get_valid() && !it->active();
                                       }),
                        std::end(manipulators_));
  }

  std::vector<std::shared_ptr<details::base>> manipulators_;
  boost::signals2::connection manipulator_timer_connection_;
};
} // namespace manipulator
} // namespace krbn
