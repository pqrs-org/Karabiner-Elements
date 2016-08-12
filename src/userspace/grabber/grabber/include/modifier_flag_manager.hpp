#pragma once

#include <memory>
#include <vector>

#include "modifier_flag.hpp"

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
    modifier_flags_.resize(static_cast<size_t>(physical_keys::end_));

    modifier_flags_[static_cast<size_t>(physical_keys::left_control)] = std::make_unique<modifier_flag>("control", "⌃");
    modifier_flags_[static_cast<size_t>(physical_keys::left_shift)] = std::make_unique<modifier_flag>("shift", "⇧");
    modifier_flags_[static_cast<size_t>(physical_keys::left_option)] = std::make_unique<modifier_flag>("option", "⌥");
    modifier_flags_[static_cast<size_t>(physical_keys::left_command)] = std::make_unique<modifier_flag>("command", "⌘");
    modifier_flags_[static_cast<size_t>(physical_keys::right_control)] = std::make_unique<modifier_flag>("control", "⌃");
    modifier_flags_[static_cast<size_t>(physical_keys::right_shift)] = std::make_unique<modifier_flag>("shift", "⇧");
    modifier_flags_[static_cast<size_t>(physical_keys::right_option)] = std::make_unique<modifier_flag>("option", "⌥");
    modifier_flags_[static_cast<size_t>(physical_keys::right_command)] = std::make_unique<modifier_flag>("command", "⌘");
    modifier_flags_[static_cast<size_t>(physical_keys::fn)] = std::make_unique<modifier_flag>("fn", "fn");
  }

  void reset(void) {
    for (const auto& f : modifier_flags_) {
      if (f) {
        f->reset();
      }
    }
  }

  enum class operation {
    increase,
    decrease,
  };

  void manipulate(physical_keys k, operation operation) {
    auto i = static_cast<size_t>(k);
    if (modifier_flags_[i]) {
      switch (operation) {
        case operation::increase:
          modifier_flags_[i]->increase();
          break;
        case operation::decrease:
          modifier_flags_[i]->decrease();
          break;
      }
    }
  }

  bool pressed(physical_keys k) const {
    auto i = static_cast<size_t>(k);
    if (!modifier_flags_[i]) {
      return false;
    }
    return modifier_flags_[i]->pressed();
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
  std::vector<std::unique_ptr<modifier_flag>> modifier_flags_;
};
