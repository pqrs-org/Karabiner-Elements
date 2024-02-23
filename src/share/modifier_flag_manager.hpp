#pragma once

#include "types.hpp"
#include <array>
#include <thread>
#include <unordered_set>
#include <vector>

namespace krbn {
class modifier_flag_manager final {
public:
#include "modifier_flag_manager/active_modifier_flag.hpp"
#include "modifier_flag_manager/scoped_modifier_flags.hpp"

  modifier_flag_manager(const modifier_flag_manager&) = delete;

  modifier_flag_manager(void) {
  }

  void push_back_active_modifier_flag(const active_modifier_flag& flag) {
    switch (flag.get_type()) {
      case active_modifier_flag::type::increase:
      case active_modifier_flag::type::decrease:
      case active_modifier_flag::type::increase_sticky:
      case active_modifier_flag::type::decrease_sticky:
        active_modifier_flags_.push_back(flag);
        erase_pairs();
        break;

      case active_modifier_flag::type::increase_lock:
      case active_modifier_flag::type::decrease_lock:
      case active_modifier_flag::type::increase_led_lock:
        // Remove same type entries.
        active_modifier_flags_.erase(std::remove_if(std::begin(active_modifier_flags_),
                                                    std::end(active_modifier_flags_),
                                                    [&](auto& f) {
                                                      return f == flag;
                                                    }),
                                     std::end(active_modifier_flags_));

        active_modifier_flags_.push_back(flag);
        erase_pairs();
        break;

      case active_modifier_flag::type::decrease_led_lock:
        // Remove all type::increase_led_lock.
        active_modifier_flags_.erase(std::remove_if(std::begin(active_modifier_flags_),
                                                    std::end(active_modifier_flags_),
                                                    [&](auto& f) {
                                                      return f.is_paired(flag);
                                                    }),
                                     std::end(active_modifier_flags_));
        break;
    }
  }

  void erase_all_active_modifier_flags(device_id device_id) {
    active_modifier_flags_.erase(std::remove_if(std::begin(active_modifier_flags_),
                                                std::end(active_modifier_flags_),
                                                [&](const active_modifier_flag& f) {
                                                  return f.get_device_id() == device_id;
                                                }),
                                 std::end(active_modifier_flags_));
  }

  void erase_all_active_modifier_flags_except_lock_and_sticky(device_id device_id) {
    active_modifier_flags_.erase(std::remove_if(std::begin(active_modifier_flags_),
                                                std::end(active_modifier_flags_),
                                                [&](const active_modifier_flag& f) {
                                                  return f.get_device_id() == device_id && !f.any_lock() && !f.sticky();
                                                }),
                                 std::end(active_modifier_flags_));
  }

  void erase_caps_lock_sticky_modifier_flags(void) {
    active_modifier_flags_.erase(std::remove_if(std::begin(active_modifier_flags_),
                                                std::end(active_modifier_flags_),
                                                [&](const active_modifier_flag& f) {
                                                  return f.get_modifier_flag() == modifier_flag::caps_lock && f.sticky();
                                                }),
                                 std::end(active_modifier_flags_));
  }

  void erase_all_sticky_modifier_flags(void) {
    active_modifier_flags_.erase(std::remove_if(std::begin(active_modifier_flags_),
                                                std::end(active_modifier_flags_),
                                                [&](const active_modifier_flag& f) {
                                                  return f.sticky();
                                                }),
                                 std::end(active_modifier_flags_));
  }

  void reset(void) {
    active_modifier_flags_.clear();
  }

  bool is_pressed(modifier_flag modifier_flag) const {
    int count = 0;
    size_t size = 0;

    int led_count = 0;

    for (const auto& f : active_modifier_flags_) {
      if (f.get_modifier_flag() == modifier_flag) {
        if (f.led_lock()) {
          led_count += f.get_count();
        } else {
          count += f.get_count();
          ++size;
        }
      }
    }

    if (size == 0) {
      // Use led lock if other flags do not exist.
      return led_count > 0;
    } else {
      // Ignore led lock if other flags exist.
      return count > 0;
    }
  }

  const std::vector<active_modifier_flag>& get_active_modifier_flags(void) const {
    return active_modifier_flags_;
  }

  size_t active_modifier_flags_size(void) const {
    return active_modifier_flags_.size();
  }

  size_t led_lock_size(modifier_flag modifier_flag) const {
    return std::count_if(std::begin(active_modifier_flags_),
                         std::end(active_modifier_flags_),
                         [&](const active_modifier_flag& f) {
                           return f.get_modifier_flag() == modifier_flag && f.led_lock();
                         });
  }

  size_t sticky_size(modifier_flag modifier_flag) const {
    return std::count_if(std::begin(active_modifier_flags_),
                         std::end(active_modifier_flags_),
                         [&](const active_modifier_flag& f) {
                           return f.get_modifier_flag() == modifier_flag && f.sticky();
                         });
  }

  bool is_sticky_active(modifier_flag modifier_flag) const {
    auto count = std::accumulate(std::begin(active_modifier_flags_),
                                 std::end(active_modifier_flags_),
                                 0,
                                 [&](const auto& count, const auto& f) {
                                   if (f.get_modifier_flag() == modifier_flag && f.sticky()) {
                                     return count + f.get_count();
                                   }
                                   return count;
                                 });
    return count > 0;
  }

  pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::modifiers make_hid_report_modifiers(void) const {
    pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::modifiers modifiers;

    std::array<modifier_flag, 8> modifier_flags{
        modifier_flag::left_control,
        modifier_flag::left_shift,
        modifier_flag::left_option,
        modifier_flag::left_command,
        modifier_flag::right_control,
        modifier_flag::right_shift,
        modifier_flag::right_option,
        modifier_flag::right_command,
    };
    for (const auto& m : modifier_flags) {
      if (is_pressed(m)) {
        if (auto r = make_hid_report_modifier(m)) {
          modifiers.insert(*r);
        }
      }
    }

    return modifiers;
  }

  std::unordered_set<modifier_flag> make_modifier_flags(void) const {
    std::unordered_set<modifier_flag> modifier_flags;

    for (const auto& m : {
             modifier_flag::caps_lock,
             modifier_flag::left_control,
             modifier_flag::left_shift,
             modifier_flag::left_option,
             modifier_flag::left_command,
             modifier_flag::right_control,
             modifier_flag::right_shift,
             modifier_flag::right_option,
             modifier_flag::right_command,
             modifier_flag::fn,
         }) {
      if (is_pressed(m)) {
        modifier_flags.insert(m);
      }
    }

    return modifier_flags;
  }

private:
  void erase_pairs(void) {
    for (size_t i1 = 0; i1 < active_modifier_flags_.size(); ++i1) {
      for (size_t i2 = i1 + 1; i2 < active_modifier_flags_.size(); ++i2) {
        if (active_modifier_flags_[i1].is_paired(active_modifier_flags_[i2])) {
          active_modifier_flags_.erase(std::begin(active_modifier_flags_) + i2);
          active_modifier_flags_.erase(std::begin(active_modifier_flags_) + i1);
          if (i1 > 0) {
            --i1;
          }
          break;
        }
      }
    }
  }

  std::vector<active_modifier_flag> active_modifier_flags_;
};

inline std::ostream& operator<<(std::ostream& stream, const modifier_flag_manager::active_modifier_flag& value) {
  stream << value.to_json();
  return stream;
}

inline std::ostream& operator<<(std::ostream& stream, const std::vector<modifier_flag_manager::active_modifier_flag>& value) {
  for (const auto& v : value) {
    stream << v.to_json() << std::endl;
  }
  return stream;
}
} // namespace krbn
