#pragma once

#include "types.hpp"
#include <CoreGraphics/CoreGraphics.h>
#include <thread>
#include <vector>

namespace krbn {
namespace manipulator {
class modifier_flag_manager final {
public:
  modifier_flag_manager(void) {
    states_.resize(static_cast<size_t>(modifier_flag::end_));

    states_[static_cast<size_t>(modifier_flag::caps_lock)] = std::make_unique<state>("caps lock", "⇪");
    states_[static_cast<size_t>(modifier_flag::left_control)] = std::make_unique<state>("control", "⌃");
    states_[static_cast<size_t>(modifier_flag::left_shift)] = std::make_unique<state>("shift", "⇧");
    states_[static_cast<size_t>(modifier_flag::left_option)] = std::make_unique<state>("option", "⌥");
    states_[static_cast<size_t>(modifier_flag::left_command)] = std::make_unique<state>("command", "⌘");
    states_[static_cast<size_t>(modifier_flag::right_control)] = std::make_unique<state>("control", "⌃");
    states_[static_cast<size_t>(modifier_flag::right_shift)] = std::make_unique<state>("shift", "⇧");
    states_[static_cast<size_t>(modifier_flag::right_option)] = std::make_unique<state>("option", "⌥");
    states_[static_cast<size_t>(modifier_flag::right_command)] = std::make_unique<state>("command", "⌘");
    states_[static_cast<size_t>(modifier_flag::fn)] = std::make_unique<state>("fn", "fn");
  }

  void reset(void) {
    for (const auto& s : states_) {
      if (s) {
        s->reset();
      }
    }
  }

  void unlock(void) {
    for (const auto& s : states_) {
      if (s) {
        s->unlock();
      }
    }
  }

  enum class operation {
    increase,
    decrease,
    lock,
    unlock,
    toggle_lock,
  };

  void manipulate(modifier_flag k, operation operation) {
    auto i = static_cast<size_t>(k);
    if (states_[i]) {
      switch (operation) {
      case operation::increase:
        states_[i]->increase();
        break;
      case operation::decrease:
        states_[i]->decrease();
        break;
      case operation::lock:
        states_[i]->lock();
        break;
      case operation::unlock:
        states_[i]->unlock();
        break;
      case operation::toggle_lock:
        states_[i]->toggle_lock();
        break;
      }
    }
  }

  bool pressed(modifier_flag m) const {
    if (m == modifier_flag::zero) {
      return true;
    }

    auto i = static_cast<size_t>(m);
    if (i < states_.size() && states_[i]) {
      return states_[i]->pressed();
    }
    return false;
  }

  bool pressed(const std::vector<modifier_flag>& modifier_flags) const {
    // return true if all modifier flags are pressed.
    for (const auto& m : modifier_flags) {
      if (!pressed(m)) {
        return false;
      }
    }
    return true;
  }

private:
  class state final {
  public:
    state(const std::string& name, const std::string& symbol) : name_(name),
                                                                symbol_(symbol),
                                                                count_(0),
                                                                lock_count_(0) {}

    const std::string& get_name(void) const { return name_; }
    const std::string& get_symbol(void) const { return symbol_; }

    bool pressed(void) {
      std::lock_guard<std::mutex> guard(mutex_);

      return (count_ + lock_count_) > 0;
    }

    void reset(void) {
      std::lock_guard<std::mutex> guard(mutex_);

      count_ = 0;
    }

    void increase(void) {
      std::lock_guard<std::mutex> guard(mutex_);

      ++count_;
    }

    void decrease(void) {
      std::lock_guard<std::mutex> guard(mutex_);

      --count_;
    }

    void lock(void) {
      std::lock_guard<std::mutex> guard(mutex_);

      lock_count_ = 1;
    }

    void unlock(void) {
      std::lock_guard<std::mutex> guard(mutex_);

      lock_count_ = 0;
    }

    void toggle_lock(void) {
      std::lock_guard<std::mutex> guard(mutex_);

      lock_count_ = lock_count_ == 0 ? 1 : 0;
    }

  private:
    std::string name_;
    std::string symbol_;
    int count_;
    int lock_count_;

    std::mutex mutex_;
  };

  std::vector<std::unique_ptr<state>> states_;
};
} // namespace manipulator
} // namespace krbn
