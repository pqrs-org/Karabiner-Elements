#pragma once

// `krbn::monitor::service_monitor::services` can be used safely in a multi-threaded environment.

#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hidsystem/IOHIDLib.h>
#include <vector>

namespace krbn {
namespace monitor {
namespace service_monitor {
class services final {
public:
  services(io_iterator_t iterator) {
    if (iterator) {
      while (auto service = IOIteratorNext(iterator)) {
        if (service) {
          IOObjectRetain(service);
          services_.push_back(service);
        }
      }
    }
  }

  ~services(void) {
    for (const auto& s : services_) {
      IOObjectRelease(s);
    }
    services_.clear();
  }

  const std::vector<io_service_t>& get_services(void) {
    return services_;
  }

private:
  std::vector<io_service_t> services_;
};
} // namespace service_monitor
} // namespace monitor
} // namespace krbn
