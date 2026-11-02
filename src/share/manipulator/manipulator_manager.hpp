#pragma once

#include "manipulator/manipulator_factory.hpp"

namespace krbn::manipulator {
class manipulator_manager final {
private:
  struct manipulator_store final {
    std::vector<pqrs::not_null_shared_ptr_t<manipulators::base>> manipulators;

    // A deferred output can synchronously ask its own manager to visit the
    // current manipulators while manipulate already holds this mutex.
    std::recursive_mutex mutex;
  };

public:
  manipulator_manager(const manipulator_manager&) = delete;

  manipulator_manager() = default;

  ~manipulator_manager() {
  }

  void push_back_manipulator(const nlohmann::json& json,
                             pqrs::not_null_shared_ptr_t<const core_configuration::details::complex_modifications_parameters> parameters) {
    try {
      auto m = manipulator_factory::make_manipulator(json,
                                                     parameters);

      {
        std::lock_guard<std::recursive_mutex> lock(manipulator_store_->mutex);

        manipulator_store_->manipulators.push_back(m);
      }

    } catch (const pqrs::json::unmarshal_error& e) {
      logger::get_logger()->error(fmt::format("karabiner.json error: {0}", e.what()));

    } catch (const std::exception& e) {
      logger::get_logger()->error(e.what());
    }
  }

  void push_back_manipulator(pqrs::not_null_shared_ptr_t<manipulators::base> ptr) {
    std::lock_guard<std::recursive_mutex> lock(manipulator_store_->mutex);

    manipulator_store_->manipulators.push_back(ptr);
  }

  /**
   * Return true if caller needs to `manipulate` again.
   */
  bool manipulate(std::weak_ptr<event_queue::queue> weak_input_event_queue,
                  std::weak_ptr<event_queue::queue> weak_output_event_queue,
                  absolute_time_point now,
                  pqrs::not_null_shared_ptr_t<const core_configuration::core_configuration> core_configuration,
                  std::shared_ptr<const manipulators::deferred_output_session> pipeline_deferred_output_session = nullptr) {
    if (auto input_event_queue = weak_input_event_queue.lock()) {
      if (auto output_event_queue = weak_output_event_queue.lock()) {
        if (!input_event_queue->empty()) {
          auto& front_input_event = input_event_queue->get_front_event();

          switch (front_input_event.get_event().get_type()) {
            case event_queue::event::type::device_keys_and_pointing_buttons_are_released:
              output_event_queue->erase_all_active_modifier_flags_except_lock_and_sticky(front_input_event.get_device_id());
              output_event_queue->erase_all_active_pointing_buttons_except_lock(front_input_event.get_device_id());

              {
                std::lock_guard<std::recursive_mutex> lock(manipulator_store_->mutex);

                for (auto&& m : manipulator_store_->manipulators) {
                  m->handle_device_keys_and_pointing_buttons_are_released_event(front_input_event,
                                                                                *output_event_queue);
                }
              }
              break;

            case event_queue::event::type::device_ungrabbed:
              // Reset modifier_flags and pointing_buttons before `handle_device_ungrabbed_event`
              // in order to send key_up events in `post_event_to_virtual_devices::handle_device_ungrabbed_event`.
              output_event_queue->erase_all_active_modifier_flags(front_input_event.get_device_id());
              output_event_queue->erase_all_active_pointing_buttons(front_input_event.get_device_id());

              {
                std::lock_guard<std::recursive_mutex> lock(manipulator_store_->mutex);

                for (auto&& m : manipulator_store_->manipulators) {
                  m->handle_device_ungrabbed_event(front_input_event.get_device_id(),
                                                   *output_event_queue,
                                                   front_input_event.get_event_time_stamp().get_time_stamp());
                }
              }
              break;

            case event_queue::event::type::pointing_device_event_from_event_tap: {
              std::lock_guard<std::recursive_mutex> lock(manipulator_store_->mutex);

              for (auto&& m : manipulator_store_->manipulators) {
                m->handle_pointing_device_event_from_event_tap(front_input_event,
                                                               *output_event_queue);
              }
            } break;

            case event_queue::event::type::none:
            case event_queue::event::type::device_grabbed:
            case event_queue::event::type::frontmost_application_changed:
            case event_queue::event::type::input_source_changed:
            case event_queue::event::type::set_variable:
            case event_queue::event::type::virtual_hid_devices_state_changed:
              // Do nothing
              break;

            case event_queue::event::type::momentary_switch_event:
            case event_queue::event::type::pointing_motion:
            case event_queue::event::type::shell_command:
            case event_queue::event::type::send_user_command:
            case event_queue::event::type::select_input_source:
            case event_queue::event::type::set_notification_message:
            case event_queue::event::type::set_caps_lock_led:
            case event_queue::event::type::mouse_key:
            case event_queue::event::type::sticky_modifier:
            case event_queue::event::type::software_function:
            case event_queue::event::type::stop_keyboard_repeat:
            case event_queue::event::type::caps_lock_state_changed:
            case event_queue::event::type::system_preferences_properties_changed: {
              bool skip = false;

              if (front_input_event.get_validity() == validity::valid) {
                std::lock_guard<std::recursive_mutex> lock(manipulator_store_->mutex);

                for (auto&& m : manipulator_store_->manipulators) {
                  if (m->already_manipulated(front_input_event)) {
                    front_input_event.set_validity(validity::invalid);
                    skip = true;
                    break;
                  }
                }
              }

              if (!skip) {
                std::lock_guard<std::recursive_mutex> lock(manipulator_store_->mutex);

                bool should_cancel_to_if_alone = true;

                // Invalid momentary switch events such as `vk_none` may be passed from
                // an upstream manipulator manager, but they do not produce output.
                if (auto e = front_input_event.get_event().get_if<momentary_switch_event>();
                    e && !e->valid()) {
                  should_cancel_to_if_alone = false;
                }

                auto deferred_output_session = pipeline_deferred_output_session;
                if (!deferred_output_session) {
                  deferred_output_session = std::make_shared<manipulators::deferred_output_session>(
                      make_deferred_output_session_callbacks());
                }

                for (auto&& m : manipulator_store_->manipulators) {
                  auto r = m->manipulate(front_input_event,
                                         *input_event_queue,
                                         output_event_queue,
                                         now,
                                         deferred_output_session);

                  switch (r) {
                    case manipulate_result::passed:
                    case manipulate_result::manipulated:
                      // Do nothing
                      break;

                    case manipulate_result::manipulated_without_cancelling_to_if_alone:
                      should_cancel_to_if_alone = false;
                      break;

                    case manipulate_result::needs_wait_until_time_stamp:
                      goto finish;
                  }
                }

                if (should_cancel_to_if_alone) {
                  for (auto&& m : manipulator_store_->manipulators) {
                    m->cancel_to_if_alone(front_input_event);
                  }
                }
              }
              break;
            }
          }

          if (input_event_queue->get_front_event().get_validity() == validity::valid) {
            {
              std::lock_guard<std::recursive_mutex> lock(manipulator_store_->mutex);

              for (auto&& m : manipulator_store_->manipulators) {
                m->post_deferred_modifier_key_up_events(front_input_event,
                                                        *output_event_queue,
                                                        manipulators::deferred_modifier_key_up_event_scope::except_front_input_event);
              }
            }

            output_event_queue->push_back_entry(input_event_queue->get_front_event());
          }

          if (core_configuration->get_global_configuration().get_reorder_same_timestamp_input_events_to_prioritize_modifiers()) {
            output_event_queue->sort_events();
          }

          input_event_queue->erase_front_event();

          return true;
        }

      finish:
        remove_invalid_manipulators();
      }
    }

    return false;
  }

  void invalidate_manipulators() {
    {
      std::lock_guard<std::recursive_mutex> lock(manipulator_store_->mutex);

      for (auto&& m : manipulator_store_->manipulators) {
        m->set_validity(validity::invalid);
      }
    }

    remove_invalid_manipulators();
  }

  [[nodiscard]] size_t get_manipulators_size() {
    std::lock_guard<std::recursive_mutex> lock(manipulator_store_->mutex);

    return manipulator_store_->manipulators.size();
  }

  [[nodiscard]] manipulators::deferred_output_session::callbacks make_deferred_output_session_callbacks() const {
    std::weak_ptr<manipulator_store> weak_store = manipulator_store_;

    return {
        .post_deferred_modifier_key_up_events = [weak_store](const event_queue::entry& event,
                                                             event_queue::queue& queue) {
          if (auto store = weak_store.lock()) {
            std::lock_guard<std::recursive_mutex> lock(store->mutex);

            for (const auto& manipulator : store->manipulators) {
              manipulator->post_deferred_modifier_key_up_events(
                  event,
                  queue,
                  manipulators::deferred_modifier_key_up_event_scope::all);
            }
          } },
        .cancel_to_if_alone = [weak_store](const event_queue::entry& event) {
          if (auto store = weak_store.lock()) {
            std::lock_guard<std::recursive_mutex> lock(store->mutex);

            for (const auto& manipulator : store->manipulators) {
              manipulator->cancel_to_if_alone(event);
            }
          } },
    };
  }

  [[nodiscard]] bool needs_virtual_hid_pointing() const {
    std::lock_guard<std::recursive_mutex> lock(manipulator_store_->mutex);

    // Include invalid manipulators that remain active after a configuration reload because they
    // might still need the virtual pointing device to finish delayed actions or release events.
    return std::any_of(std::begin(manipulator_store_->manipulators),
                       std::end(manipulator_store_->manipulators),
                       [](auto& m) {
                         return m->needs_virtual_hid_pointing();
                       });
  }

  [[nodiscard]] bool uses_caps_lock_led_override() const {
    std::lock_guard<std::recursive_mutex> lock(manipulator_store_->mutex);

    // Unlike `needs_virtual_hid_pointing`, only the current configuration may control the LED.
    // Exclude invalid manipulators so that their delayed actions cannot keep an old override active.
    return std::any_of(std::begin(manipulator_store_->manipulators),
                       std::end(manipulator_store_->manipulators),
                       [](auto& m) {
                         return m->get_validity() == validity::valid &&
                                m->uses_caps_lock_led_override();
                       });
  }

private:
  void remove_invalid_manipulators() {
    std::lock_guard<std::recursive_mutex> lock(manipulator_store_->mutex);

    auto& manipulators = manipulator_store_->manipulators;
    manipulators.erase(std::remove_if(std::begin(manipulators),
                                      std::end(manipulators),
                                      [](const auto& it) {
                                        // Keep active manipulators.
                                        return it->get_validity() == validity::invalid && !it->active();
                                      }),
                       std::end(manipulators));
  }

  std::shared_ptr<manipulator_store> manipulator_store_{std::make_shared<manipulator_store>()};
};
} // namespace krbn::manipulator
