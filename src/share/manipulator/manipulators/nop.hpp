#pragma once

#include "../types.hpp"
#include "base.hpp"

namespace krbn::manipulator::manipulators {
class nop final : public base {
public:
  nop() : base() {
  }

  ~nop() override {
  }

  bool already_manipulated(const event_queue::entry& front_input_event) override {
    return false;
  }

  manipulate_result manipulate(event_queue::entry& front_input_event,
                               const event_queue::queue& input_event_queue,
                               std::shared_ptr<event_queue::queue> output_event_queue,
                               absolute_time_point now) override {
    return manipulate_result::passed;
  }

  bool active() const override {
    return false;
  }

  bool needs_virtual_hid_pointing() const override {
    return false;
  }

  void handle_device_keys_and_pointing_buttons_are_released_event(const event_queue::entry& front_input_event,
                                                                  event_queue::queue& output_event_queue) override {
  }

  void handle_device_ungrabbed_event(device_id device_id,
                                     const event_queue::queue& output_event_queue,
                                     absolute_time_point time_stamp) override {
  }

  void handle_pointing_device_event_from_event_tap(const event_queue::entry& front_input_event,
                                                   event_queue::queue& output_event_queue) override {
  }
};
} // namespace krbn::manipulator::manipulators
