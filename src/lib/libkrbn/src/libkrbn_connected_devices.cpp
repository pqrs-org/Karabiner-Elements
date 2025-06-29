#include "libkrbn/impl/libkrbn_components_manager.hpp"
#include "libkrbn/impl/libkrbn_connected_devices_monitor.hpp"
#include "libkrbn/impl/libkrbn_cpp.hpp"

namespace {
std::shared_ptr<const krbn::connected_devices::connected_devices> get_current_connected_devices(void) {
  if (auto manager = libkrbn_cpp::get_components_manager()) {
    return manager->get_current_connected_devices();
  }

  return nullptr;
}
} // namespace

size_t libkrbn_connected_devices_get_size(void) {
  if (auto c = get_current_connected_devices()) {
    return c->get_devices().size();
  }
  return 0;
}

bool libkrbn_connected_devices_get_unique_identifier(size_t index,
                                                     char* buffer,
                                                     size_t length) {
  if (buffer && length > 0) {
    buffer[0] = '\0';
  }

  if (auto c = get_current_connected_devices()) {
    const auto& devices = c->get_devices();
    if (index < devices.size()) {
      auto& identifiers = devices[index]->get_device_identifiers();
      auto unique_idenfitier = nlohmann::json(identifiers).dump();
      strlcpy(buffer, unique_idenfitier.c_str(), length);
      return true;
    }
  }

  return false;
}

bool libkrbn_connected_devices_get_manufacturer(size_t index,
                                                char* buffer,
                                                size_t length) {
  if (buffer && length > 0) {
    buffer[0] = '\0';
  }

  if (auto c = get_current_connected_devices()) {
    const auto& devices = c->get_devices();
    if (index < devices.size()) {
      strlcpy(buffer, type_safe::get(devices[index]->get_manufacturer()).c_str(), length);
      return true;
    }
  }

  return false;
}

bool libkrbn_connected_devices_get_product(size_t index,
                                           char* buffer,
                                           size_t length) {
  if (buffer && length > 0) {
    buffer[0] = '\0';
  }

  if (auto c = get_current_connected_devices()) {
    const auto& devices = c->get_devices();
    if (index < devices.size()) {
      strlcpy(buffer, type_safe::get(devices[index]->get_product()).c_str(), length);
      return true;
    }
  }

  return false;
}

bool libkrbn_connected_devices_get_transport(size_t index,
                                             char* buffer,
                                             size_t length) {
  if (buffer && length > 0) {
    buffer[0] = '\0';
  }

  if (auto c = get_current_connected_devices()) {
    const auto& devices = c->get_devices();
    if (index < devices.size()) {
      strlcpy(buffer, devices[index]->get_transport().c_str(), length);
      return true;
    }
  }

  return false;
}

bool libkrbn_connected_devices_get_device_identifiers(size_t index, libkrbn_device_identifiers* device_identifiers) {
  if (auto c = get_current_connected_devices()) {
    if (device_identifiers) {
      const auto& devices = c->get_devices();
      if (index < devices.size()) {
        auto& identifiers = devices[index]->get_device_identifiers();
        device_identifiers->vendor_id = type_safe::get(identifiers.get_vendor_id());
        device_identifiers->product_id = type_safe::get(identifiers.get_product_id());
        device_identifiers->is_keyboard = identifiers.get_is_keyboard();
        device_identifiers->is_pointing_device = identifiers.get_is_pointing_device();
        device_identifiers->is_game_pad = identifiers.get_is_game_pad();
        device_identifiers->is_consumer = identifiers.get_is_consumer();
        device_identifiers->is_virtual_device = identifiers.get_is_virtual_device();
        return true;
      }
    }
  }
  return false;
}

uint64_t libkrbn_connected_devices_get_vendor_id(size_t index) {
  if (auto c = get_current_connected_devices()) {
    const auto& devices = c->get_devices();
    if (index < devices.size()) {
      return type_safe::get(devices[index]->get_device_identifiers().get_vendor_id());
    }
  }
  return 0;
}

uint64_t libkrbn_connected_devices_get_product_id(size_t index) {
  if (auto c = get_current_connected_devices()) {
    const auto& devices = c->get_devices();
    if (index < devices.size()) {
      return type_safe::get(devices[index]->get_device_identifiers().get_product_id());
    }
  }
  return 0;
}

bool libkrbn_connected_devices_get_device_address(size_t index,
                                                  char* buffer,
                                                  size_t length) {
  if (buffer && length > 0) {
    buffer[0] = '\0';
  }

  if (auto c = get_current_connected_devices()) {
    const auto& devices = c->get_devices();
    if (index < devices.size()) {
      strlcpy(buffer, devices[index]->get_device_identifiers().get_device_address().c_str(), length);
      return true;
    }
  }

  return false;
}

bool libkrbn_connected_devices_get_is_keyboard(size_t index) {
  if (auto c = get_current_connected_devices()) {
    const auto& devices = c->get_devices();
    if (index < devices.size()) {
      return devices[index]->get_device_identifiers().get_is_keyboard();
    }
  }
  return false;
}

bool libkrbn_connected_devices_get_is_pointing_device(size_t index) {
  if (auto c = get_current_connected_devices()) {
    const auto& devices = c->get_devices();
    if (index < devices.size()) {
      return devices[index]->get_device_identifiers().get_is_pointing_device();
    }
  }
  return false;
}

bool libkrbn_connected_devices_get_is_game_pad(size_t index) {
  if (auto c = get_current_connected_devices()) {
    const auto& devices = c->get_devices();
    if (index < devices.size()) {
      return devices[index]->get_device_identifiers().get_is_game_pad();
    }
  }
  return false;
}

bool libkrbn_connected_devices_get_is_consumer(size_t index) {
  if (auto c = get_current_connected_devices()) {
    const auto& devices = c->get_devices();
    if (index < devices.size()) {
      return devices[index]->get_device_identifiers().get_is_consumer();
    }
  }
  return false;
}

bool libkrbn_connected_devices_get_is_virtual_device(size_t index) {
  if (auto c = get_current_connected_devices()) {
    const auto& devices = c->get_devices();
    if (index < devices.size()) {
      return devices[index]->get_device_identifiers().get_is_virtual_device();
    }
  }
  return false;
}

bool libkrbn_connected_devices_get_is_built_in_keyboard(size_t index) {
  if (auto c = get_current_connected_devices()) {
    const auto& devices = c->get_devices();
    if (index < devices.size()) {
      return devices[index]->get_is_built_in_keyboard();
    }
  }
  return 0;
}

bool libkrbn_connected_devices_is_apple(size_t index) {
  if (auto c = get_current_connected_devices()) {
    const auto& devices = c->get_devices();
    if (index < devices.size()) {
      return devices[index]->get_is_apple();
    }
  }
  return 0;
}
