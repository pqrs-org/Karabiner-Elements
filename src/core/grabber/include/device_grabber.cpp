#include "device_grabber.hpp"

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
