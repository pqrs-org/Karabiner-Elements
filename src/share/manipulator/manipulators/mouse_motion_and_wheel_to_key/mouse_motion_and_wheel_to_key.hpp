#pragma once

#include "../shared_event_sender.hpp"
#include "krbn_notification_center.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <pqrs/dispatcher.hpp>
#include <unordered_map>

namespace krbn::manipulator::manipulators::mouse_motion_and_wheel_to_key {
// Manipulation, environment updates and timer callbacks run on the shared dispatcher.
class mouse_motion_and_wheel_to_key final : public base, public pqrs::dispatcher::extra::dispatcher_client {
  pqrs::dispatcher::extra::dispatcher_client_constructor_exception_guard dispatcher_client_constructor_guard_{*this};

public:
  mouse_motion_and_wheel_to_key(const nlohmann::json& json,
                                pqrs::not_null_shared_ptr_t<const core_configuration::details::complex_modifications_parameters> parameters)
      : timer_(*this) {
    dispatcher_client_constructor_guard_.initialize(
        [&] {
          pqrs::json::requires_object(json, "json");
          for (const auto& [key, value] : json.items()) {
            if (key == "from") {
              pqrs::json::requires_object(value, "`from`");

              for (const auto& [k, v] : value.items()) {
                if (k == "source") {
                  pqrs::json::requires_string(v, "`from.source`");

                  if (v == "xy") {
                    source_ = source::xy;
                  } else if (v == "wheels") {
                    source_ = source::wheels;
                  } else if (v == "vertical_wheel") {
                    source_ = source::vertical_wheel;
                  } else if (v == "horizontal_wheel") {
                    source_ = source::horizontal_wheel;
                  } else {
                    throw pqrs::json::unmarshal_error("unknown `from.source`: `" + v.get<std::string>() + "`");
                  }

                } else if (k == "threshold") {
                  pqrs::json::requires_number(v, "`from.threshold`");

                  threshold_ = v.get<int>();

                } else if (k == "sampling_interval_milliseconds") {
                  pqrs::json::requires_number(v, "`from.sampling_interval_milliseconds`");

                  sampling_interval_ = std::chrono::milliseconds(v.get<int>());

                } else if (k == "cooldown_milliseconds") {
                  pqrs::json::requires_number(v, "`from.cooldown_milliseconds`");

                  cooldown_ = std::chrono::milliseconds(v.get<int>());

                } else if (k == "modifiers") {
                  from_modifiers_definition_ = v.get<from_modifiers_definition>();

                } else {
                  throw pqrs::json::unmarshal_error("unknown key in `from`: `" + k + "`");
                }
              }

            } else if (key == "to") {
              pqrs::json::requires_object(value, "`to`");

              for (const auto& [name, events] : value.items()) {
                direction output_direction;
                if (name == "left") {
                  output_direction = left;
                } else if (name == "right") {
                  output_direction = right;
                } else if (name == "up") {
                  output_direction = up;
                } else if (name == "down") {
                  output_direction = down;
                } else {
                  throw pqrs::json::unmarshal_error("unknown direction in `to`: `" + name + "`");
                }

                pqrs::json::requires_array(events, "`to." + name + "`");
                for (const auto& j : events) {
                  to_[output_direction].push_back(std::make_shared<to_event_definition>(j));
                }
              }

            } else if (key != "type" &&
                       key != "description" &&
                       key != "conditions" &&
                       key != "parameters") {
              throw pqrs::json::unmarshal_error("unknown key in mouse_motion_and_wheel_to_key: `" + key + "`");
            }
          }

          if (source_ == source::xy && !json.at("from").contains("threshold")) {
            threshold_ = 20;
          }

          if (source_ == source::none) {
            throw pqrs::json::unmarshal_error("mouse_motion_and_wheel_to_key requires from.source");
          }
        });
  }

  ~mouse_motion_and_wheel_to_key() override {
    detach_from_dispatcher([this] {
      windows_.clear();
      cooldowns_.clear();
    });
  }

  [[nodiscard]] bool already_manipulated(const event_queue::entry&) override {
    return false;
  }

  manipulate_result manipulate(event_queue::entry& front_input_event,
                               const event_queue::queue& input_event_queue,
                               std::shared_ptr<event_queue::queue> output_event_queue,
                               absolute_time_point now) override {
    if (!output_event_queue ||
        validity_ == validity::invalid) {
      windows_.clear();
      cooldowns_.clear();
      schedule();
      return manipulate_result::passed;
    }

    auto motion = front_input_event.get_event().get_if<pointing_motion>();
    if (!motion) {
      return manipulate_result::passed;
    }

    if (front_input_event.get_validity() == validity::invalid) {
      return manipulate_result::passed;
    }

    int64_t x = 0;
    int64_t y = 0;
    // Pointer Y grows downwards, while positive vertical wheel deltas scroll up.
    switch (source_) {
      case source::xy:
        x = motion->get_x();
        y = motion->get_y();
        break;

      case source::wheels:
      case source::horizontal_wheel:
      case source::vertical_wheel:
        x = motion->get_horizontal_wheel();
        y = -static_cast<int64_t>(motion->get_vertical_wheel());

        if (source_ == source::vertical_wheel) {
          x = 0;
        }
        if (source_ == source::horizontal_wheel) {
          y = 0;
        }
        break;

      case source::none:
        break;
    }

    if (x == 0 && y == 0) {
      return manipulate_result::passed;
    }

    auto device = front_input_event.get_device_id();
    auto cooldown = cooldowns_.find(device);
    if (cooldown != cooldowns_.end() && cooldown->second <= when_now()) {
      cooldowns_.erase(cooldown);
      cooldown = cooldowns_.end();
    }

    // During cooldown, consume selected axes without accumulating or rechecking eligibility.
    if (cooldown == cooldowns_.end()) {
      auto it = windows_.find(front_input_event.get_device_id());
      if (it == windows_.end()) {
        const conditions::condition_context context{
            .device_id = front_input_event.get_device_id(),
            .state = front_input_event.get_state(),
        };
        auto modifiers = from_modifiers_definition_.test_modifiers(output_event_queue->get_modifier_flag_manager());
        if (!modifiers ||
            !condition_manager_.is_fulfilled(context, output_event_queue->get_manipulator_environment())) {
          return manipulate_result::passed;
        }
        auto output_modifiers = output_event_queue->get_modifier_flag_manager().make_modifier_flags();
        for (auto flag : *modifiers) {
          output_modifiers.erase(flag);
        }
        // Latch input eligibility for the entire fixed window. Later input from
        // this device is accumulated and consumed without rechecking conditions.
        it = windows_.emplace(front_input_event.get_device_id(),
                              pending_window{
                                  .deadline = when_now() + sampling_interval_,
                                  .context = context,
                                  .original_event = front_input_event.get_original_event(),
                                  .output = output_event_queue,
                                  .output_time_stamp = front_input_event.get_event_time_stamp().get_time_stamp() + pqrs::osx::chrono::make_absolute_time_duration(sampling_interval_),
                                  .output_modifiers = std::move(output_modifiers),
                              })
                 .first;
      }

      auto& window = it->second;
      // Keep signed sums and abs safe even with extreme deltas and long windows.
      constexpr auto limit = std::numeric_limits<int64_t>::max() / 2;
      window.x = std::clamp(window.x + x, -limit, limit);
      window.y = std::clamp(window.y + y, -limit, limit);
      schedule();
    }

    // Consume the entire selected group, even unmapped directions and deltas
    // below the threshold. Leave only the other group available to later rules.
    auto remaining = *motion;
    if (source_ == source::xy) {
      remaining.set_x(0);
      remaining.set_y(0);
    } else {
      if (source_ == source::wheels ||
          source_ == source::horizontal_wheel) {
        remaining.set_horizontal_wheel(0);
      }
      if (source_ == source::wheels ||
          source_ == source::vertical_wheel) {
        remaining.set_vertical_wheel(0);
      }
    }

    if (remaining.is_zero()) {
      front_input_event.set_validity(validity::invalid);
    } else {
      front_input_event = event_queue::entry(front_input_event.get_device_id(),
                                             front_input_event.get_event_time_stamp(),
                                             event_queue::event(remaining),
                                             front_input_event.get_event_type(),
                                             front_input_event.get_event_integer_value(),
                                             front_input_event.get_original_event(),
                                             event_queue::state::manipulated,
                                             front_input_event.get_lazy());
    }

    return manipulate_result::manipulated;
  }

  [[nodiscard]] bool active() const noexcept override {
    return false;
  }

  [[nodiscard]] bool needs_virtual_hid_pointing() const noexcept override {
    for (const auto& events : to_) {
      if (std::ranges::any_of(events, [](const auto& event) {
            return event->needs_virtual_hid_pointing();
          })) {
        return true;
      }
    }
    return false;
  }

  void handle_device_keys_and_pointing_buttons_are_released_event(const event_queue::entry&,
                                                                  event_queue::queue&) override {
  }

  void handle_device_ungrabbed_event(device_id device_id,
                                     const event_queue::queue&,
                                     absolute_time_point) override {
    windows_.erase(device_id);
    cooldowns_.erase(device_id);
    schedule();
  }

  void handle_pointing_device_event_from_event_tap(const event_queue::entry&,
                                                   event_queue::queue&) override {
  }

  void set_validity(validity value) override {
    base::set_validity(value);

    if (value == validity::invalid) {
      windows_.clear();
      cooldowns_.clear();
      schedule();
    }
  }

private:
  enum class source {
    none,
    xy,
    wheels,
    vertical_wheel,
    horizontal_wheel,
  };
  enum direction : size_t {
    left,
    right,
    up,
    down,
  };
  struct pending_window {
    pqrs::dispatcher::time_point deadline;
    conditions::condition_context context;
    event_queue::event original_event;
    std::weak_ptr<event_queue::queue> output;
    absolute_time_point output_time_stamp;
    std::unordered_set<modifier_flag> output_modifiers;
    int64_t x = 0;
    int64_t y = 0;
  };

  void schedule() {
    std::optional<pqrs::dispatcher::time_point> next;

    for (const auto& [device, window] : windows_) {
      if (!next || window.deadline < *next) {
        next = window.deadline;
      }
    }

    if (next == scheduled_at_) {
      return;
    }

    scheduled_at_ = next;
    if (!next) {
      timer_.cancel();
    } else {
      timer_.debounce_at(
          [this] {
            scheduled_at_.reset();

            std::vector<pending_window> expired;
            std::erase_if(windows_, [&](const auto& entry) {
              if (entry.second.deadline <= when_now()) {
                expired.push_back(entry.second);
                return true;
              }
              return false;
            });

            // A delayed callback can expire several windows at once.
            std::ranges::sort(expired, {}, &pending_window::deadline);
            for (const auto& window : expired) {
              if (post_window(window) && cooldown_ > std::chrono::milliseconds::zero()) {
                cooldowns_[window.context.device_id] = when_now() + cooldown_;
              }
            }

            schedule();
          },
          *next);
    }
  }

  bool post_window(const pending_window& window) {
    auto output = window.output.lock();
    if (!output || validity_ == validity::invalid) {
      return false;
    }

    auto x = window.x;
    auto y = window.y;
    if (source_ == source::xy) {
      if (std::abs(x) >= std::abs(y)) {
        y = 0;
      } else {
        x = 0;
      }
    }

    const std::array<int64_t, 4> amounts{-x, x, -y, y};
    std::array<bool, 4> selected{};
    for (size_t direction = 0; direction < amounts.size(); ++direction) {
      selected[direction] = (amounts[direction] >= threshold_) && !to_[direction].empty();
    }
    if (std::ranges::none_of(selected,
                             [](bool value) {
                               return value;
                             })) {
      return false;
    }

    event_queue::event_time_stamp time_stamp(window.output_time_stamp);
    absolute_time_duration delay(0);
    // Compute temporary changes with the shared modifier-state machinery, then
    // emit them so both the queue and the virtual device see the captured state.
    std::vector<modifier_flag_manager::active_modifier_flag> changes;
    std::vector<modifier_flag_manager::active_modifier_flag> inverse_changes;
    {
      modifier_flag_manager::scoped_modifier_flags scoped(output->get_modifier_flag_manager(),
                                                          window.output_modifiers);
      changes = scoped.get_scoped_active_modifier_flags();
      inverse_changes = scoped.get_inverse_active_modifier_flags();
    }

    shared_event_sender::post_active_modifier_flags(changes,
                                                    time_stamp,
                                                    delay,
                                                    window.original_event,
                                                    *output);

    bool emitted = false;

    // Wheel directions are evaluated independently, horizontal before vertical.
    for (size_t direction = 0; direction < selected.size(); ++direction) {
      if (!selected[direction]) {
        continue;
      }
      for (const auto& to : to_[direction]) {
        if (to->get_condition_manager().is_fulfilled(window.context,
                                                     output->get_manipulator_environment())) {
          // post_tap leaves delay unchanged when the definition produces no event.
          auto previous_delay = delay;
          shared_event_sender::post_tap(*to,
                                        window.context.device_id,
                                        time_stamp,
                                        delay,
                                        window.original_event,
                                        *output);
          emitted |= delay != previous_delay;
        }
      }
    }

    shared_event_sender::post_active_modifier_flags(inverse_changes,
                                                    time_stamp,
                                                    delay,
                                                    window.original_event,
                                                    *output);

    output->increase_time_stamp_delay(delay);
    krbn_notification_center::get_instance().enqueue_input_event_arrived(*this);
    return emitted;
  }

  source source_ = source::none;
  int threshold_ = 1;
  std::chrono::milliseconds sampling_interval_{100};
  std::chrono::milliseconds cooldown_{100};
  from_modifiers_definition from_modifiers_definition_;
  std::array<to_event_definitions, 4> to_;
  std::unordered_map<device_id, pending_window> windows_;
  std::unordered_map<device_id, pqrs::dispatcher::time_point> cooldowns_;
  std::optional<pqrs::dispatcher::time_point> scheduled_at_;
  pqrs::dispatcher::extra::debounced_task timer_;
};
} // namespace krbn::manipulator::manipulators::mouse_motion_and_wheel_to_key
