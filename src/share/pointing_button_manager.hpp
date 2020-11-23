#pragma once

#include "types.hpp"
#include <pqrs/karabiner/driverkit/virtual_hid_device_driver.hpp>
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
                           pqrs::hid::usage_pair usage_pair,
                           device_id device_id) : type_(type),
                                                  usage_pair_(usage_pair),
                                                  device_id_(device_id) {
    }

    type get_type(void) const {
      return type_;
    }

    const pqrs::hid::usage_pair& get_usage_pair(void) const {
      return usage_pair_;
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
             usage_pair_ == other.usage_pair_;
    }

    constexpr auto operator<=>(const active_pointing_button&) const = default;

  private:
    type type_;
    pqrs::hid::usage_pair usage_pair_;
    device_id device_id_;
  };

  void push_back_active_pointing_button(const active_pointing_button& button) {
    if (button.get_usage_pair().get_usage() == pqrs::hid::usage::undefined) {
      return;
    }

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

  bool is_pressed(const pqrs::hid::usage_pair& usage_pair) const {
    int count = 0;

    for (const auto& f : active_pointing_buttons_) {
      if (f.get_usage_pair() == usage_pair) {
        count += f.get_count();
      }
    }

    return count > 0;
  }

  pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::buttons make_hid_report_buttons(void) const {
    pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::buttons buttons;

    pqrs::hid::usage_pair usage_pair(pqrs::hid::usage_page::button,
                                     pqrs::hid::usage::button::button_1);
    while (usage_pair.get_usage() <= pqrs::hid::usage::button::button_32) {
      if (is_pressed(usage_pair)) {
        buttons.insert(type_safe::get(usage_pair.get_usage()));
      }

      usage_pair.set_usage(
          pqrs::hid::usage::value_t(type_safe::get(usage_pair.get_usage()) + 1));
    }

    return buttons;
  }

private:
  std::vector<active_pointing_button> active_pointing_buttons_;
};
} // namespace krbn
