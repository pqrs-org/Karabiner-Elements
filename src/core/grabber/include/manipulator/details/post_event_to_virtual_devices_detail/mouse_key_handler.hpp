#pragma once

#include "queue.hpp"

namespace krbn {
namespace manipulator {
namespace details {
namespace post_event_to_virtual_devices_detail {
class mouse_key_handler final : public pqrs::dispatcher::extra::dispatcher_client {
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
                    const system_preferences& system_preferences) : dispatcher_client(),
                                                                    queue_(queue),
                                                                    system_preferences_(system_preferences),
                                                                    active_(false),
                                                                    x_count_converter_(128),
                                                                    y_count_converter_(128),
                                                                    vertical_wheel_count_converter_(128),
                                                                    horizontal_wheel_count_converter_(128) {
  }

  virtual ~mouse_key_handler(void) {
    detach_from_dispatcher([] {
    });
  }

  void async_push_back_mouse_key(device_id device_id,
                                 const mouse_key& mouse_key,
                                 const std::weak_ptr<event_queue::queue>& weak_output_event_queue,
                                 absolute_time time_stamp) {
    enqueue_to_dispatcher(
        [this, device_id, mouse_key, weak_output_event_queue, time_stamp] {
          erase_entry(device_id, mouse_key);

          entries_.emplace_back(device_id, mouse_key);
          active_ = !entries_.empty();

          weak_output_event_queue_ = weak_output_event_queue;

          post_event(time_stamp);
        });
  }

  void async_erase_mouse_key(device_id device_id,
                             const mouse_key& mouse_key,
                             const std::weak_ptr<event_queue::queue>& weak_output_event_queue,
                             absolute_time time_stamp) {
    enqueue_to_dispatcher([this, device_id, mouse_key, weak_output_event_queue, time_stamp] {
      erase_entry(device_id, mouse_key);

      weak_output_event_queue_ = weak_output_event_queue;

      post_event(time_stamp);
    });
  }

  void async_erase_mouse_keys_by_device_id(device_id device_id,
                                           absolute_time time_stamp) {
    enqueue_to_dispatcher([this, device_id, time_stamp] {
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

        auto when = time_utility::to_milliseconds(time_stamp) + std::chrono::milliseconds(20);
        enqueue_to_dispatcher(
            [this, when] {
              post_event(time_utility::to_absolute_time(when));
              krbn_notification_center::get_instance().enqueue_input_event_arrived(*this);
            },
            when);
      }
    }
  }

  queue& queue_;
  const system_preferences& system_preferences_;

  std::vector<std::pair<device_id, mouse_key>> entries_;
  std::atomic<bool> active_;
  std::weak_ptr<event_queue::queue> weak_output_event_queue_;
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
