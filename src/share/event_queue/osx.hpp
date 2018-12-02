#pragma once

#include "event_queue/queue.hpp"
#include "types.hpp"
#include <pqrs/cf_ptr.hpp>

namespace krbn {
namespace event_queue {
inline std::shared_ptr<queue> make_queue(device_id device_id,
                                         std::shared_ptr<std::vector<pqrs::cf_ptr<IOHIDValueRef>>> values) {
  auto result = std::make_shared<queue>();

  if (values) {
    std::vector<hid_value> hid_values;
    for (const auto& v : *values) {
      hid_values.emplace_back(*v);
    }

    for (const auto& pair : queue::make_entries(hid_values, device_id)) {
      auto& entry = pair.second;
      result->push_back_event(entry);
    }
  }

  return result;
}
} // namespace event_queue
} // namespace krbn
