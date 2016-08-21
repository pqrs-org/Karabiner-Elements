#pragma once

#include "types.hpp"
#include <IOKit/IOKitLib.h>
#include <IOKit/hidsystem/IOHIDShared.h>
#include <memory>
#include <vector>

class modifier_flag_manager {
public:
  enum class physical_keys {
    left_control,
    left_shift,
    left_option,
    left_command,
    right_control,
    right_shift,
    right_option,
    right_command,
    fn,
    end_,
  };

  modifier_flag_manager(void) {
    states_.resize(static_cast<size_t>(physical_keys::end_));

    states_[static_cast<size_t>(physical_keys::left_control)] = std::make_unique<state>("control", "⌃");
    states_[static_cast<size_t>(physical_keys::left_shift)] = std::make_unique<state>("shift", "⇧");
    states_[static_cast<size_t>(physical_keys::left_option)] = std::make_unique<state>("option", "⌥");
    states_[static_cast<size_t>(physical_keys::left_command)] = std::make_unique<state>("command", "⌘");
    states_[static_cast<size_t>(physical_keys::right_control)] = std::make_unique<state>("control", "⌃");
    states_[static_cast<size_t>(physical_keys::right_shift)] = std::make_unique<state>("shift", "⇧");
    states_[static_cast<size_t>(physical_keys::right_option)] = std::make_unique<state>("option", "⌥");
    states_[static_cast<size_t>(physical_keys::right_command)] = std::make_unique<state>("command", "⌘");
    states_[static_cast<size_t>(physical_keys::fn)] = std::make_unique<state>("fn", "fn");
  }

  void reset(void) {
    for (const auto& s : states_) {
      if (s) {
        s->reset();
      }
    }
  }

  enum class operation {
    increase,
    decrease,
  };

  void manipulate(physical_keys k, operation operation) {
    auto i = static_cast<size_t>(k);
    if (states_[i]) {
      switch (operation) {
      case operation::increase:
        states_[i]->increase();
        break;
      case operation::decrease:
        states_[i]->decrease();
        break;
      }
    }
  }

  bool pressed(physical_keys k) const {
    auto i = static_cast<size_t>(k);
    if (!states_[i]) {
      return false;
    }
    return states_[i]->pressed();
  }

  bool pressed(const std::vector<modifier_flag>& modifier_flags) {
    for (const auto& modifier_flag : modifier_flags) {
      auto m = static_cast<uint32_t>(modifier_flag);
      if (m < states_.size() && !states_[m]->pressed()) {
        return false;
      }
    }
    return true;
  }

  uint8_t get_hid_report_bits(void) const {
    uint8_t bits = 0;

    if (pressed(physical_keys::left_control)) {
      bits |= (0x1 << 0);
    }
    if (pressed(physical_keys::left_shift)) {
      bits |= (0x1 << 1);
    }
    if (pressed(physical_keys::left_option)) {
      bits |= (0x1 << 2);
    }
    if (pressed(physical_keys::left_command)) {
      bits |= (0x1 << 3);
    }
    if (pressed(physical_keys::right_control)) {
      bits |= (0x1 << 4);
    }
    if (pressed(physical_keys::right_shift)) {
      bits |= (0x1 << 5);
    }
    if (pressed(physical_keys::right_option)) {
      bits |= (0x1 << 6);
    }
    if (pressed(physical_keys::right_command)) {
      bits |= (0x1 << 7);
    }

    return bits;
  }

  IOOptionBits get_io_option_bits(void) const {
    IOOptionBits bits = 0;
    if (pressed(physical_keys::left_control) ||
        pressed(physical_keys::right_control)) {
      bits |= NX_CONTROLMASK;
    }
    if (pressed(physical_keys::left_shift) ||
        pressed(physical_keys::right_shift)) {
      bits |= NX_SHIFTMASK;
    }
    if (pressed(physical_keys::left_option) ||
        pressed(physical_keys::right_option)) {
      bits |= NX_ALTERNATEMASK;
    }
    if (pressed(physical_keys::left_command) ||
        pressed(physical_keys::right_command)) {
      bits |= NX_COMMANDMASK;
    }
    if (pressed(physical_keys::fn)) {
      bits |= NX_SECONDARYFNMASK;
    }
    return bits;
  }

private:
  class state {
  public:
    state(const std::string& name, const std::string& symbol) : name_(name),
                                                                symbol_(symbol),
                                                                count_(0) {}

    const std::string& get_name(void) const { return name_; }
    const std::string& get_symbol(void) const { return symbol_; }

    bool pressed(void) const { return count_ > 0; }

    void reset(void) {
      count_ = 0;
    }

    void increase(void) {
      ++count_;
    }

    void decrease(void) {
      --count_;
    }

  private:
    std::string name_;
    std::string symbol_;
    int count_;
  };

  std::vector<std::unique_ptr<state>> states_;
};
