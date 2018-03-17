#pragma once

#include "Karabiner-VirtualHIDDevice/dist/include/karabiner_virtual_hid_device_methods.hpp"
#include "types.hpp"
#include <thread>
#include <vector>

namespace krbn {
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

    type get_inverse_type(void) const {
      switch (type_) {
        case type::increase:
          return type::decrease;
        case type::decrease:
          return type::increase;
      }
    }

    int get_count(void) const {
      if (type_ == type::increase) {
        return 1;
      } else {
        return -1;
      }
    }

    bool is_paired(const active_pointing_button& other) const {
      // ignore device_id_
      return get_type() == other.get_inverse_type() &&
             get_pointing_button() == other.get_pointing_button();
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
    switch (button.get_type()) {
      case active_pointing_button::type::increase:
        active_pointing_buttons_.push_back(button);
        break;

      case active_pointing_button::type::decrease:
        // Erase all paired entries to avoid button lock when same type::increase button pushed twice.
        active_pointing_buttons_.erase(std::remove_if(std::begin(active_pointing_buttons_),
                                                      std::end(active_pointing_buttons_),
                                                      [&](auto& b) {
                                                        return b.is_paired(button);
                                                      }),
                                       std::end(active_pointing_buttons_));
        break;
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

  pqrs::karabiner_virtual_hid_device::hid_report::buttons make_hid_report_buttons(void) const {
    pqrs::karabiner_virtual_hid_device::hid_report::buttons buttons;

    auto button1 = static_cast<uint32_t>(pointing_button::button1);
    auto button32 = static_cast<uint32_t>(pointing_button::button32);

    for (size_t i = button1; i < button32; ++i) {
      if (is_pressed(pointing_button(i))) {
        buttons.insert(i - button1 + 1);
      }
    }

    return buttons;
  }

  pqrs::karabiner_virtual_hid_device::hid_report::pointing_input make_pointing_input_report(void) const {
    pqrs::karabiner_virtual_hid_device::hid_report::pointing_input report;

    report.buttons = make_hid_report_buttons();

    return report;
  }

private:
  std::vector<active_pointing_button> active_pointing_buttons_;
};
} // namespace krbn
