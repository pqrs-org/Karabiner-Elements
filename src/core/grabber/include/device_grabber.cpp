#include "boost_defs.hpp"

#include "apple_hid_usage_tables.hpp"
#include "configuration_monitor.hpp"
#include "constants.hpp"
#include "event_manipulator.hpp"
#include "event_tap_manager.hpp"
#include "gcd_utility.hpp"
#include "human_interface_device.hpp"
#include "iokit_utility.hpp"
#include "logger.hpp"
#include "physical_keyboard_repeat_detector.hpp"
#include "pressed_physical_keys_counter.hpp"
#include "spdlog_utility.hpp"
#include "types.hpp"
#include "device_grabber.hpp"
#include <IOKit/hid/IOHIDManager.h>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <json/json.hpp>
#include <thread>
#include <time.h>

using namespace krbn;

device_grabber * _Nullable device_grabber::grabber = nullptr;

boost::optional<const core_configuration::profile::device::identifiers&> device_grabber::find_device_identifiers(vendor_id vid, product_id pid) {
  if (core_configuration_) {
    for (const auto& kv : hids_) {
      auto hid = kv.second;
      if (hid) {
        if (vid == hid->get_vendor_id() && pid == hid->get_product_id()) {
          return hid->get_connected_device().get_identifiers();
        }
      }
      
    }
  }
  return boost::none;
}

boost::optional<std::weak_ptr<human_interface_device>> device_grabber::get_hid_by_id(device_id device_id) {
  auto it = id2dev_.find(device_id);
  std::weak_ptr<human_interface_device> ptr;
  
  if (it != id2dev_.end()) {
    ptr = it->second;
    return boost::make_optional(ptr);
  } else {
    return boost::none;
  }
}
