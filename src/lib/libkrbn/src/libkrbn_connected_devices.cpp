#include "constants.hpp"
#include "libkrbn/libkrbn.h"
#include "monitor/connected_devices_monitor.hpp"

namespace {
class libkrbn_connected_devices_class final {
public:
  libkrbn_connected_devices_class(const krbn::connected_devices::connected_devices& connected_devices) : connected_devices_(connected_devices) {
  }

  krbn::connected_devices::connected_devices& get_connected_devices(void) {
    return connected_devices_;
  }

private:
  krbn::connected_devices::connected_devices connected_devices_;
};

class libkrbn_connected_devices_monitor_class final {
public:
  libkrbn_connected_devices_monitor_class(const libkrbn_connected_devices_monitor_class&) = delete;

  libkrbn_connected_devices_monitor_class(libkrbn_connected_devices_monitor_callback callback, void* refcon) {
    connected_devices_monitor_ = std::make_unique<krbn::connected_devices_monitor>(
        krbn::constants::get_devices_json_file_path());

    connected_devices_monitor_->connected_devices_updated.connect([callback, refcon](auto&& weak_connected_devices) {
      if (auto connected_devices = weak_connected_devices.lock()) {
        if (callback) {
          auto* p = new libkrbn_connected_devices_class(*connected_devices);
          callback(p, refcon);
        }
      }
    });

    connected_devices_monitor_->async_start();
  }

  ~libkrbn_connected_devices_monitor_class(void) {
    connected_devices_monitor_ = nullptr;
  }

private:
  std::unique_ptr<krbn::connected_devices_monitor> connected_devices_monitor_;
};
} // namespace

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

bool libkrbn_connected_devices_monitor_initialize(libkrbn_connected_devices_monitor** out,
                                                  libkrbn_connected_devices_monitor_callback callback,
                                                  void* refcon) {
  if (!out) return false;
  // return if already initialized.
  if (*out) return false;

  *out = reinterpret_cast<libkrbn_connected_devices_monitor*>(new libkrbn_connected_devices_monitor_class(callback, refcon));
  return true;
}

void libkrbn_connected_devices_monitor_terminate(libkrbn_connected_devices_monitor** p) {
  if (p && *p) {
    delete reinterpret_cast<libkrbn_connected_devices_monitor_class*>(*p);
    *p = nullptr;
  }
}
