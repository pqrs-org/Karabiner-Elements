#pragma once

#include <memory>

#include "modifier_flag.hpp"

class modifier_flag_manager {
public:
  modifier_flag_manager(void) {
    // left control
    modifier_flags_.push(std::make_unique<modifier_flag>(0x1 << 0, "control", "⌃"));
    // left shift
    modifier_flags_.push(std::make_unique<modifier_flag>(0x1 << 1, "shift", "⇧"));
    // left option
    modifier_flags_.push(std::make_unique<modifier_flag>(0x1 << 2, "option", "⌥"));
    // left command
    modifier_flags_.push(std::make_unique<modifier_flag>(0x1 << 3, "command", "⌘"));
    // right control
    modifier_flags_.push(std::make_unique<modifier_flag>(0x1 << 4, "control", "⌃"));
    // right shift
    modifier_flags_.push(std::make_unique<modifier_flag>(0x1 << 5, "shift", "⇧"));
    // right option
    modifier_flags_.push(std::make_unique<modifier_flag>(0x1 << 6, "option", "⌥"));
    // right command
    modifier_flags_.push(std::make_unique<modifier_flag>(0x1 << 7, "command", "⌘"));
  }

private:
  std::vector<std::unique_ptr<modifier_flag>> modifier_flags_;
};
