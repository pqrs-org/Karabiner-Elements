#pragma once

#include "event_queue.hpp"
#include "types.hpp"
#include <unordered_map>

namespace krbn {
namespace manipulator {
namespace detail {
class basic final : public base {
public:
  basic(const nlohmann::json& json) : base() {
  }

  virtual ~basic(void) {
  }

  virtual manipulate(event_queue& event_queue) {
  }

private:

};
} // namespace detail
} // namespace manipulator
} // namespace krbn
