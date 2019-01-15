#include "libkrbn/impl/libkrbn_connected_devices_monitor.hpp"

void libkrbn_connected_devices_terminate(libkrbn_connected_devices** p) {
  if (p && *p) {
    delete reinterpret_cast<libkrbn_connected_devices_class*>(*p);
    *p = nullptr;
  }
}

size_t libkrbn_connected_devices_get_size(libkrbn_connected_devices* p) {
  if (auto c = reinterpret_cast<libkrbn_connected_devices_class*>(p)) {
    return c->get_connected_devices().get_devices().size();
  }
  return 0;
}

const char* libkrbn_connected_devices_get_descriptions_manufacturer(libkrbn_connected_devices* p, size_t index) {
  if (auto c = reinterpret_cast<libkrbn_connected_devices_class*>(p)) {
    const auto& devices = c->get_connected_devices().get_devices();
    if (index < devices.size()) {
      return devices[index].get_descriptions().get_manufacturer().c_str();
    }
  }
  return nullptr;
}

const char* libkrbn_connected_devices_get_descriptions_product(libkrbn_connected_devices* p, size_t index) {
  if (auto c = reinterpret_cast<libkrbn_connected_devices_class*>(p)) {
    const auto& devices = c->get_connected_devices().get_devices();
    if (index < devices.size()) {
      return devices[index].get_descriptions().get_product().c_str();
    }
  }
  return nullptr;
}

bool libkrbn_connected_devices_get_device_identifiers(libkrbn_connected_devices* p, size_t index, libkrbn_device_identifiers* device_identifiers) {
  if (auto c = reinterpret_cast<libkrbn_connected_devices_class*>(p)) {
    if (device_identifiers) {
      const auto& devices = c->get_connected_devices().get_devices();
      if (index < devices.size()) {
        auto identifiers = devices[index].get_identifiers();
        device_identifiers->vendor_id = static_cast<uint32_t>(identifiers.get_vendor_id());
        device_identifiers->product_id = static_cast<uint32_t>(identifiers.get_product_id());
        device_identifiers->is_keyboard = static_cast<uint32_t>(identifiers.get_is_keyboard());
        device_identifiers->is_pointing_device = static_cast<uint32_t>(identifiers.get_is_pointing_device());
        return true;
      }
    }
  }
  return false;
}

bool libkrbn_connected_devices_get_is_built_in_keyboard(libkrbn_connected_devices* p, size_t index) {
  if (auto c = reinterpret_cast<libkrbn_connected_devices_class*>(p)) {
    const auto& devices = c->get_connected_devices().get_devices();
    if (index < devices.size()) {
      return devices[index].get_is_built_in_keyboard();
    }
  }
  return 0;
}

bool libkrbn_connected_devices_get_is_built_in_trackpad(libkrbn_connected_devices* p, size_t index) {
  if (auto c = reinterpret_cast<libkrbn_connected_devices_class*>(p)) {
    const auto& devices = c->get_connected_devices().get_devices();
    if (index < devices.size()) {
      return devices[index].get_is_built_in_trackpad();
    }
  }
  return 0;
}
