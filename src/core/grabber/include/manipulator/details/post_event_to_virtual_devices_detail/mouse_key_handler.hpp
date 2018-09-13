#pragma once

#include "manipulator/manipulator_dispatcher.hpp"
#include "manipulator/manipulator_timer.hpp"
#include "queue.hpp"

namespace krbn {
namespace manipulator {
namespace details {
namespace post_event_to_virtual_devices_detail {
class mouse_key_handler final {
public:
  class count_converter final {
  public:
    count_converter(int threshold) : threshold_(threshold),
                                     count_(0) {
    }

    uint8_t update(int value) {
      int result = 0;

      count_ += value;

      while (count_ <= -threshold_) {
        --result;
        count_ += threshold_;
      }
      while (count_ >= threshold_) {
        ++result;
        count_ -= threshold_;
      }

      return static_cast<uint8_t>(result);
    }

    void reset(void) {
      count_ = 0;
    }

  private:
    int threshold_;
    int count_;
  };

  mouse_key_handler(queue& queue,
                    std::weak_ptr<manipulator_dispatcher> weak_manipulator_dispatcher,
                    std::weak_ptr<manipulator_timer> weak_manipulator_timer,
                    const system_preferences& system_preferences) : queue_(queue),
                                                                    weak_manipulator_dispatcher_(weak_manipulator_dispatcher),
                                                                    weak_manipulator_timer_(weak_manipulator_timer),
                                                                    system_preferences_(system_preferences),
                                                                    manipulator_object_id_(make_new_manipulator_object_id()),
                                                                    active_(false),
                                                                    x_count_converter_(128),
                                                                    y_count_converter_(128),
                                                                    vertical_wheel_count_converter_(128),
                                                                    horizontal_wheel_count_converter_(128) {
    if (auto manipulator_dispatcher = weak_manipulator_dispatcher_.lock()) {
      manipulator_dispatcher->async_attach(manipulator_object_id_);
    }
    if (auto manipulator_timer = weak_manipulator_timer_.lock()) {
      manipulator_timer->async_attach(manipulator_object_id_);
    }
  }

  ~mouse_key_handler(void) {
    if (auto manipulator_timer = weak_manipulator_timer_.lock()) {
      manipulator_timer->detach(manipulator_object_id_);
    }

    if (auto manipulator_dispatcher = weak_manipulator_dispatcher_.lock()) {
      manipulator_dispatcher->detach(manipulator_object_id_);
    }
  }

  void async_push_back_mouse_key(device_id device_id,
                                 const mouse_key& mouse_key,
                                 const std::weak_ptr<event_queue>& weak_output_event_queue,
                                 absolute_time time_stamp) {
    if (auto manipulator_dispatcher = weak_manipulator_dispatcher_.lock()) {
      manipulator_dispatcher->enqueue(
          manipulator_object_id_,
          [this, device_id, mouse_key, weak_output_event_queue, time_stamp] {
            erase_entry(device_id, mouse_key);

            entries_.emplace_back(device_id, mouse_key);
            active_ = !entries_.empty();

            weak_output_event_queue_ = weak_output_event_queue;

            post_event(time_stamp);
          });
    }
  }

  void async_erase_mouse_key(device_id device_id,
                             const mouse_key& mouse_key,
                             const std::weak_ptr<event_queue>& weak_output_event_queue,
                             absolute_time time_stamp) {
    if (auto manipulator_dispatcher = weak_manipulator_dispatcher_.lock()) {
      manipulator_dispatcher->enqueue(
          manipulator_object_id_,
          [this, device_id, mouse_key, weak_output_event_queue, time_stamp] {
            erase_entry(device_id, mouse_key);

            weak_output_event_queue_ = weak_output_event_queue;

            post_event(time_stamp);
          });
    }
  }

  void async_erase_mouse_keys_by_device_id(device_id device_id,
                                           absolute_time time_stamp) {
    if (auto manipulator_dispatcher = weak_manipulator_dispatcher_.lock()) {
      manipulator_dispatcher->enqueue(
          manipulator_object_id_,
          [this, device_id, time_stamp] {
            entries_.erase(std::remove_if(std::begin(entries_),
                                          std::end(entries_),
                                          [&](const auto& pair) {
                                            return pair.first == device_id;
                                          }),
                           std::end(entries_));
            active_ = !entries_.empty();

            post_event(time_stamp);
          });
    }
  }

  bool active(void) const {
    return active_;
  }

private:
  void erase_entry(device_id device_id,
                   const mouse_key& mouse_key) {
    entries_.erase(std::remove_if(std::begin(entries_),
                                  std::end(entries_),
                                  [&](const auto& pair) {
                                    return pair.first == device_id &&
                                           pair.second == mouse_key;
                                  }),
                   std::end(entries_));
    active_ = !entries_.empty();
  }

  void post_event(absolute_time time_stamp) {
    if (auto oeq = weak_output_event_queue_.lock()) {
      mouse_key total;
      for (const auto& pair : entries_) {
        total += pair.second;
      }

      if (!system_preferences_.get_swipe_scroll_direction()) {
        total.invert_wheel();
      }

      if (total.is_zero()) {
        last_mouse_key_total_ = boost::none;

      } else {
        if (last_mouse_key_total_ != total) {
          last_mouse_key_total_ = total;

          x_count_converter_.reset();
          y_count_converter_.reset();
          vertical_wheel_count_converter_.reset();
          horizontal_wheel_count_converter_.reset();
        }

        pqrs::karabiner_virtual_hid_device::hid_report::pointing_input report;
        report.buttons = oeq->get_pointing_button_manager().make_hid_report_buttons();
        report.x = x_count_converter_.update(static_cast<int>(total.get_x() * total.get_speed_multiplier()));
        report.y = y_count_converter_.update(static_cast<int>(total.get_y() * total.get_speed_multiplier()));
        report.vertical_wheel = vertical_wheel_count_converter_.update(static_cast<int>(total.get_vertical_wheel() * total.get_speed_multiplier()));
        report.horizontal_wheel = horizontal_wheel_count_converter_.update(static_cast<int>(total.get_horizontal_wheel() * total.get_speed_multiplier()));

        queue_.emplace_back_pointing_input(report,
                                           event_type::single,
                                           time_stamp);

        if (auto manipulator_timer = weak_manipulator_timer_.lock()) {
          auto when = time_stamp + time_utility::to_absolute_time(std::chrono::milliseconds(20));

          manipulator_timer->enqueue(
              manipulator_object_id_,
              [this, when] {
                if (auto manipulator_dispatcher = weak_manipulator_dispatcher_.lock()) {
                  manipulator_dispatcher->enqueue(
                      manipulator_object_id_,
                      [this, when] {
                        post_event(when);
                        krbn_notification_center::get_instance().input_event_arrived();
                      });
                }
              },
              when);
          manipulator_timer->async_invoke(time_stamp);
        }
      }
    }
  }

  queue& queue_;
  std::weak_ptr<manipulator_dispatcher> weak_manipulator_dispatcher_;
  std::weak_ptr<manipulator_timer> weak_manipulator_timer_;
  const system_preferences& system_preferences_;

  manipulator_object_id manipulator_object_id_;
  std::vector<std::pair<device_id, mouse_key>> entries_;
  std::atomic<bool> active_;
  std::weak_ptr<event_queue> weak_output_event_queue_;
  boost::optional<mouse_key> last_mouse_key_total_;
  count_converter x_count_converter_;
  count_converter y_count_converter_;
  count_converter vertical_wheel_count_converter_;
  count_converter horizontal_wheel_count_converter_;
};
} // namespace post_event_to_virtual_devices_detail
} // namespace details
} // namespace manipulator
} // namespace krbn
