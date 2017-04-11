#pragma once

#include "manipulator.hpp"
#include "types.hpp"
#include <thread>
#include <vector>

namespace krbn {
namespace manipulator {
class pointing_button_manager final {
public:
  class active_pointing_button final {
  public:
    enum class type {
      increase,
      decrease,
    };

    active_pointing_button(type type,
                           pointing_button pointing_button,
                           device_id device_id) : type_(type),
                                                  pointing_button_(pointing_button),
                                                  device_id_(device_id) {
    }

    type get_type(void) const {
      return type_;
    }

    pointing_button get_pointing_button(void) const {
      return pointing_button_;
    }

    device_id get_device_id(void) const {
      return device_id_;
    }

    int get_count(void) const {
      if (type_ == type::increase) {
        return 1;
      } else {
        return -1;
      }
    }

    bool operator==(const active_pointing_button& other) const {
      return get_type() == other.get_type() &&
             get_pointing_button() == other.get_pointing_button() &&
             get_device_id() == other.get_device_id();
    }

  private:
    type type_;
    pointing_button pointing_button_;
    device_id device_id_;
  };

  void push_back_active_pointing_button(const active_pointing_button& button) {
    active_pointing_buttons_.push_back(button);
  }

  void erase_active_pointing_button(const active_pointing_button& button) {
    auto it = std::find(std::begin(active_pointing_buttons_),
                        std::end(active_pointing_buttons_),
                        button);
    if (it != std::end(active_pointing_buttons_)) {
      active_pointing_buttons_.erase(it);
    }
  }

  void erase_all_active_pointing_buttons(device_id device_id) {
    active_pointing_buttons_.erase(std::remove_if(std::begin(active_pointing_buttons_),
                                                  std::end(active_pointing_buttons_),
                                                  [&](const active_pointing_button& b) {
                                                    return b.get_device_id() == device_id;
                                                  }),
                                   std::end(active_pointing_buttons_));
  }

  void erase_all_active_pointing_buttons_except_lock(device_id device_id) {
    erase_all_active_pointing_buttons(device_id);
  }

  void reset(void) {
    active_pointing_buttons_.clear();
  }

  bool is_pressed(pointing_button pointing_button) const {
    int count = 0;

    for (const auto& f : active_pointing_buttons_) {
      if (f.get_pointing_button() == pointing_button) {
        count += f.get_count();
      }
    }

    return count > 0;
  }

  uint32_t get_hid_report_bits(void) const {
    uint32_t bits = 0;

    auto button1 = static_cast<uint32_t>(pointing_button::button1);
    auto button32 = static_cast<uint32_t>(pointing_button::button32);

    for (size_t i = button1; i < button32; ++i) {
      if (is_pressed(pointing_button(i))) {
        bits |= static_cast<uint32_t>(1 << (i - button1));
      }
    }

    return bits;
  }

private:
  std::vector<active_pointing_button> active_pointing_buttons_;
};
} // namespace manipulator
} // namespace krbn
