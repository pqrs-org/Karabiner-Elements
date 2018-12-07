#pragma once

#include "queue.hpp"
#include "types.hpp"
#include "utility.hpp"
#include <pqrs/cf_ptr.hpp>

namespace krbn {
namespace event_queue {
namespace osx {
inline std::shared_ptr<queue> make_queue(device_id device_id,
                                         std::shared_ptr<std::vector<pqrs::cf_ptr<IOHIDValueRef>>> values) {
  auto result = std::make_shared<queue>();

  if (values) {
    std::vector<hid_value> hid_values;
    for (const auto& v : *values) {
      hid_values.emplace_back(*v);
    }

    result = utility::make_queue(device_id, hid_values);
  }

  return result;
}
} // namespace osx
} // namespace event_queue
} // namespace krbn
