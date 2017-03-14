#include "connected_devices_monitor.hpp"
#include "constants.hpp"
#include "file_monitor.hpp"
#include "libkrbn.h"
#include "libkrbn.hpp"

namespace {
class libkrbn_connected_devices_class final {
public:
  libkrbn_connected_devices_class(std::shared_ptr<krbn::connected_devices> connected_devices) : connected_devices_(connected_devices) {
  }

  krbn::connected_devices& get_connected_devices(void) {
    return *connected_devices_;
  }

private:
  std::shared_ptr<krbn::connected_devices> connected_devices_;
};

class libkrbn_connected_devices_monitor_class final {
public:
  libkrbn_connected_devices_monitor_class(const libkrbn_connected_devices_monitor_class&) = delete;

  libkrbn_connected_devices_monitor_class(libkrbn_connected_devices_monitor_callback callback, void* refcon) : callback_(callback), refcon_(refcon) {
    connected_devices_monitor_ = std::make_unique<krbn::connected_devices_monitor>(libkrbn::get_logger(),
                                                                                   [this](const std::shared_ptr<krbn::connected_devices> connected_devices) {
                                                                                     if (callback_) {
                                                                                       auto* p = new libkrbn_connected_devices_class(connected_devices);
                                                                                       callback_(p, refcon_);
                                                                                     }
                                                                                   });
  }

private:
  libkrbn_configuration_monitor_callback callback_;
  void* refcon_;

  std::unique_ptr<krbn::connected_devices_monitor> connected_devices_monitor_;
};

class libkrbn_device_monitor_class final {
public:
  libkrbn_device_monitor_class(const libkrbn_device_monitor_class&) = delete;

  libkrbn_device_monitor_class(libkrbn_device_monitor_callback callback, void* refcon) : callback_(callback), refcon_(refcon) {
    std::vector<std::pair<std::string, std::vector<std::string>>> targets = {
        {krbn::constants::get_tmp_directory(), {krbn::constants::get_devices_json_file_path()}},
    };

    file_monitor_ = std::make_unique<krbn::file_monitor>(libkrbn::get_logger(),
                                                         targets,
                                                         [this](const std::string&) {
                                                           if (callback_) {
                                                             callback_(refcon_);
                                                           }
                                                         });
  }

private:
  libkrbn_device_monitor_callback callback_;
  void* refcon_;

  std::unique_ptr<krbn::file_monitor> file_monitor_;
};
}

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

uint32_t libkrbn_connected_devices_get_identifiers_vendor_id(libkrbn_connected_devices* p, size_t index) {
  if (auto c = reinterpret_cast<libkrbn_connected_devices_class*>(p)) {
    const auto& devices = c->get_connected_devices().get_devices();
    if (index < devices.size()) {
      return static_cast<uint32_t>(devices[index].get_identifiers().get_vendor_id());
    }
  }
  return 0;
}

uint32_t libkrbn_connected_devices_get_identifiers_product_id(libkrbn_connected_devices* p, size_t index) {
  if (auto c = reinterpret_cast<libkrbn_connected_devices_class*>(p)) {
    const auto& devices = c->get_connected_devices().get_devices();
    if (index < devices.size()) {
      return static_cast<uint32_t>(devices[index].get_identifiers().get_product_id());
    }
  }
  return 0;
}

bool libkrbn_connected_devices_get_identifiers_is_keyboard(libkrbn_connected_devices* p, size_t index) {
  if (auto c = reinterpret_cast<libkrbn_connected_devices_class*>(p)) {
    const auto& devices = c->get_connected_devices().get_devices();
    if (index < devices.size()) {
      return devices[index].get_identifiers().get_is_keyboard();
    }
  }
  return 0;
}

bool libkrbn_connected_devices_get_identifiers_is_pointing_device(libkrbn_connected_devices* p, size_t index) {
  if (auto c = reinterpret_cast<libkrbn_connected_devices_class*>(p)) {
    const auto& devices = c->get_connected_devices().get_devices();
    if (index < devices.size()) {
      return devices[index].get_identifiers().get_is_pointing_device();
    }
  }
  return 0;
}

bool libkrbn_connected_devices_get_ignored(libkrbn_connected_devices* p, size_t index) {
  if (auto c = reinterpret_cast<libkrbn_connected_devices_class*>(p)) {
    const auto& devices = c->get_connected_devices().get_devices();
    if (index < devices.size()) {
      return devices[index].get_ignored();
    }
  }
  return 0;
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

bool libkrbn_device_monitor_initialize(libkrbn_device_monitor** out, libkrbn_device_monitor_callback callback, void* refcon) {
  if (!out) return false;
  // return if already initialized.
  if (*out) return false;

  *out = reinterpret_cast<libkrbn_device_monitor*>(new libkrbn_device_monitor_class(callback, refcon));
  return true;
}

void libkrbn_device_monitor_terminate(libkrbn_device_monitor** p) {
  if (p && *p) {
    delete reinterpret_cast<libkrbn_device_monitor_class*>(*p);
    *p = nullptr;
  }
}
