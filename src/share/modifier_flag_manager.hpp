#pragma once

#include "types.hpp"
#include <thread>
#include <vector>

namespace krbn {
class modifier_flag_manager final {
public:
  class active_modifier_flag final {
  public:
    enum class type {
      increase,
      decrease,
      increase_lock,
      decrease_lock,
    };

    active_modifier_flag(type type,
                         modifier_flag modifier_flag,
                         device_id device_id) : type_(type),
                                                modifier_flag_(modifier_flag),
                                                device_id_(device_id) {
    }

    type get_type(void) const {
      return type_;
    }

    modifier_flag get_modifier_flag(void) const {
      return modifier_flag_;
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
        case type::increase_lock:
          return type::decrease_lock;
        case type::decrease_lock:
          return type::increase_lock;
      }
    }

    int get_count(void) const {
      if (type_ == type::increase ||
          type_ == type::increase_lock) {
        return 1;
      } else {
        return -1;
      }
    }

    bool is_paired(const active_modifier_flag& other) const {
      return get_type() == other.get_inverse_type() &&
             get_modifier_flag() == other.get_modifier_flag() &&
             get_device_id() == other.get_device_id();
    }

    bool operator==(const active_modifier_flag& other) const {
      return get_type() == other.get_type() &&
             get_modifier_flag() == other.get_modifier_flag() &&
             get_device_id() == other.get_device_id();
    }

  private:
    type type_;
    modifier_flag modifier_flag_;
    device_id device_id_;
  };

  const std::vector<active_modifier_flag>& get_active_modifier_flags(void) const {
    return active_modifier_flags_;
  }

  void push_back_active_modifier_flag(const active_modifier_flag& flag) {
    switch (flag.get_type()) {
      case active_modifier_flag::type::increase:
      case active_modifier_flag::type::decrease:
      case active_modifier_flag::type::increase_lock:
        active_modifier_flags_.push_back(flag);
        erase_pairs();
        break;

      case active_modifier_flag::type::decrease_lock:
        // Remove all type::increase_lock
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

  void erase_all_active_modifier_flags_except_lock(device_id device_id) {
    active_modifier_flags_.erase(std::remove_if(std::begin(active_modifier_flags_),
                                                std::end(active_modifier_flags_),
                                                [&](const active_modifier_flag& f) {
                                                  return f.get_device_id() == device_id &&
                                                         (f.get_type() != active_modifier_flag::type::increase_lock &&
                                                          f.get_type() != active_modifier_flag::type::decrease_lock);
                                                }),
                                 std::end(active_modifier_flags_));
  }

  void reset(void) {
    active_modifier_flags_.clear();
  }

  bool is_pressed(modifier_flag modifier_flag) const {
    int count = 0;

    for (const auto& f : active_modifier_flags_) {
      if (f.get_modifier_flag() == modifier_flag) {
        count += f.get_count();
      }
    }

    return count > 0;
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
} // namespace krbn
