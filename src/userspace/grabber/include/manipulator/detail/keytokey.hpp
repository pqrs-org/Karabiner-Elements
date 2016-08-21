#pragma once

#include "types.hpp"

namespace manipulator {
class keytokey {
  bool manipulate(key_code key_code, bool key_down) {
    return false;
  }

private:
  key_code from_event_;
  std::vector<modifier_flag> from_modifier_flags_;
};
}
