#pragma once

// `krbn::manipulator::manipulator_managers_connector` can be used safely in a multi-threaded environment.

#include "event_queue.hpp"
#include "logger.hpp"
#include "manipulator/manipulator_manager.hpp"
#include <mutex>
#include <pqrs/gsl.hpp>

namespace krbn::manipulator {
class manipulator_managers_connector final {
public:
  class connection final {
    friend class manipulator_managers_connector;

  public:
    connection(std::weak_ptr<manipulator_manager> weak_manipulator_manager,
               std::weak_ptr<event_queue::queue> weak_input_event_queue,
               std::weak_ptr<event_queue::queue> weak_output_event_queue) : weak_manipulator_manager_(weak_manipulator_manager),
                                                                            weak_input_event_queue_(weak_input_event_queue),
                                                                            weak_output_event_queue_(weak_output_event_queue) {
    }

  private:
    [[nodiscard]] std::weak_ptr<event_queue::queue> get_weak_output_event_queue() const {
      return weak_output_event_queue_;
    }

    [[nodiscard]] std::weak_ptr<manipulator_manager> get_weak_manipulator_manager() const {
      return weak_manipulator_manager_;
    }

    void manipulate(absolute_time_point now,
                    pqrs::not_null_shared_ptr_t<const core_configuration::core_configuration> core_configuration,
                    std::shared_ptr<const manipulators::deferred_output_session> deferred_output_session) const {
      if (auto manipulator_manager = weak_manipulator_manager_.lock()) {
        while (true) {
          auto processed = manipulator_manager->manipulate(weak_input_event_queue_,
                                                           weak_output_event_queue_,
                                                           now,
                                                           core_configuration,
                                                           deferred_output_session);
          if (!processed) {
            break;
          }
        }
      }
    }

    void invalidate_manipulators() const {
      if (auto manipulator_manager = weak_manipulator_manager_.lock()) {
        manipulator_manager->invalidate_manipulators();
      }
    }

    void set_manipulator_environment_core_configuration(pqrs::not_null_shared_ptr_t<const core_configuration::core_configuration> core_configuration) {
      if (auto queue = weak_input_event_queue_.lock()) {
        queue->get_manipulator_environment().set_core_configuration(core_configuration);
      }
      if (auto queue = weak_output_event_queue_.lock()) {
        queue->get_manipulator_environment().set_core_configuration(core_configuration);
      }
    }

    [[nodiscard]] bool needs_virtual_hid_pointing() const {
      if (auto manipulator_manager = weak_manipulator_manager_.lock()) {
        return manipulator_manager->needs_virtual_hid_pointing();
      }
      return false;
    }

    [[nodiscard]] bool uses_caps_lock_led_override() const {
      if (auto manipulator_manager = weak_manipulator_manager_.lock()) {
        return manipulator_manager->uses_caps_lock_led_override();
      }
      return false;
    }

    [[nodiscard]] std::optional<absolute_time_point> make_input_event_time_stamp_with_input_delay() const {
      if (auto input_event_queue = weak_input_event_queue_.lock()) {
        if (!input_event_queue->get_entries().empty()) {
          return input_event_queue->get_entries().front().get_event_time_stamp().make_time_stamp_with_input_delay();
        }
      }
      return std::nullopt;
    }

    std::weak_ptr<manipulator_manager> weak_manipulator_manager_;
    std::weak_ptr<event_queue::queue> weak_input_event_queue_;
    std::weak_ptr<event_queue::queue> weak_output_event_queue_;
  };

  void emplace_back_connection(std::weak_ptr<manipulator_manager> weak_manipulator_manager,
                               std::weak_ptr<event_queue::queue> weak_input_event_queue,
                               std::weak_ptr<event_queue::queue> weak_output_event_queue) {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    connections_.emplace_back(weak_manipulator_manager,
                              weak_input_event_queue,
                              weak_output_event_queue);
  }

  void emplace_back_connection(std::weak_ptr<manipulator_manager> weak_manipulator_manager,
                               std::weak_ptr<event_queue::queue> weak_output_event_queue) {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    if (connections_.empty()) {
      throw std::runtime_error("manipulator_managers_connector::connections_ is empty");
    }

    connections_.emplace_back(weak_manipulator_manager,
                              connections_.back().get_weak_output_event_queue(),
                              weak_output_event_queue);
  }

  void manipulate(absolute_time_point now,
                  pqrs::not_null_shared_ptr_t<const core_configuration::core_configuration> core_configuration) const {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    for (std::size_t current_index = 0; current_index < connections_.size(); ++current_index) {
      struct group final {
        manipulators::deferred_output_session::callbacks callbacks;
        std::vector<std::weak_ptr<event_queue::queue>> skipped_output_queues;
      };

      std::vector<group> groups;

      for (std::size_t owner_index = 0; owner_index <= current_index; ++owner_index) {
        if (auto manager = connections_[owner_index].get_weak_manipulator_manager().lock()) {
          group g{
              .callbacks = manager->make_deferred_output_session_callbacks(),
          };

          // Deferred events owned by an upstream manager are posted directly
          // into the current manager's output queue. Keep the modifier state of
          // every skipped queue in sync with those events.
          for (std::size_t queue_index = owner_index; queue_index < current_index; ++queue_index) {
            g.skipped_output_queues.push_back(connections_[queue_index].get_weak_output_event_queue());
          }

          groups.push_back(std::move(g));
        }
      }

      auto deferred_output_session = std::make_shared<manipulators::deferred_output_session>(
          manipulators::deferred_output_session::callbacks{
              .post_deferred_modifier_key_up_events = [groups](const event_queue::entry& event,
                                                               event_queue::queue& target_queue) {
                for (const auto& group : groups) {
                  auto first_new_entry = target_queue.get_entries().size();

                  group.callbacks.post_deferred_modifier_key_up_events(event,
                                                                        target_queue);

                  const auto& entries = target_queue.get_entries();
                  for (std::size_t i = first_new_entry; i < entries.size(); ++i) {
                    for (const auto& weak_queue : group.skipped_output_queues) {
                      if (auto queue = weak_queue.lock()) {
                        queue->apply_modifier_state(entries[i]);
                      }
                    }
                  }
                } },
              .cancel_to_if_alone = [groups](const event_queue::entry& event) {
                for (const auto& group : groups) {
                  group.callbacks.cancel_to_if_alone(event);
                } },
          });

      connections_[current_index].manipulate(now,
                                             core_configuration,
                                             deferred_output_session);
    }
  }

  void invalidate_manipulators() const {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    for (auto&& c : connections_) {
      c.invalidate_manipulators();
    }
  }

  void set_manipulator_environment_core_configuration(pqrs::not_null_shared_ptr_t<const core_configuration::core_configuration> core_configuration) {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    for (auto&& c : connections_) {
      c.set_manipulator_environment_core_configuration(core_configuration);
    }
  }

  [[nodiscard]] bool needs_virtual_hid_pointing() const {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    return std::any_of(std::begin(connections_),
                       std::end(connections_),
                       [](auto& c) {
                         return c.needs_virtual_hid_pointing();
                       });
  }

  [[nodiscard]] bool uses_caps_lock_led_override() const {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    return std::any_of(std::begin(connections_),
                       std::end(connections_),
                       [](auto& c) {
                         return c.uses_caps_lock_led_override();
                       });
  }

  [[nodiscard]] std::optional<absolute_time_point> min_input_event_time_stamp() const {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    std::optional<absolute_time_point> result;

    for (const auto& c : connections_) {
      if (auto t = c.make_input_event_time_stamp_with_input_delay()) {
        if (!result || *t < *result) {
          result = t;
        }
      }
    }

    return result;
  }

private:
  std::vector<connection> connections_;
  mutable std::mutex connections_mutex_;
};
} // namespace krbn::manipulator
