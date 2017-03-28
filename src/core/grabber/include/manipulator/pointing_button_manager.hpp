#pragma once

#include "types.hpp"
#include <thread>
#include <vector>

namespace krbn {
namespace manipulator {
class pointing_button_manager final {
public:
  pointing_button_manager(void) {
    const auto max_size = static_cast<size_t>(pointing_button::end_);
    states_.resize(max_size);
    for (size_t i = 0; i < max_size; ++i) {
      states_[i] = std::make_unique<state>();
    }
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

  void manipulate(pointing_button b, operation operation) {
    auto i = static_cast<size_t>(b);
    if (i < states_.size() && states_[i]) {
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

  bool pressed(pointing_button b) const {
    if (b == pointing_button::zero) {
      return true;
    }

    auto i = static_cast<size_t>(b);
    if (i < states_.size() && states_[i]) {
      return states_[i]->pressed();
    }
    return false;
  }

  bool pressed(const std::vector<pointing_button>& pointing_buttons) {
    // return true if all modifier flags are pressed.
    for (const auto& b : pointing_buttons) {
      if (!pressed(b)) {
        return false;
      }
    }
    return true;
  }

  uint32_t get_hid_report_bits(void) const {
    uint32_t bits = 0;

    auto button1 = static_cast<size_t>(pointing_button::button1);
    auto button32 = static_cast<size_t>(pointing_button::button32);

    for (size_t i = button1; i < button32; ++i) {
      if (states_[i] && states_[i]->pressed()) {
        bits |= static_cast<uint32_t>(1 << (i - button1));
      }
    }

    return bits;
  }

private:
  class state final {
  public:
    state(void) : count_(0),
                  lock_count_(0) {}

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
    int count_;
    int lock_count_;

    std::mutex mutex_;
  };

  std::vector<std::unique_ptr<state>> states_;
};
} // namespace manipulator
} // namespace krbn
