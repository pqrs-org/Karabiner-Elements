#pragma once

#include "filesystem.hpp"
#include "json_utility.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <algorithm>
#include <fstream>
#include <json/json.hpp>

// Json example:
//
// [
//     {
//         "descriptions": {
//             "manufacturer": "Unknown",
//             "product": "HHKB-BT"
//         },
//         "identifiers": {
//             "is_keyboard": true,
//             "is_pointing_device": false,
//             "product_id": 515,
//             "vendor_id": 1278
//         },
//         "is_built_in_keyboard": false,
//         "is_built_in_trackpad": false
//     },
//     {
//         "descriptions": {
//             "manufacturer": "Apple Inc.",
//             "product": "Apple Internal Keyboard / Trackpad"
//         },
//         "identifiers": {
//             "is_keyboard": true,
//             "is_pointing_device": false,
//             "product_id": 610,
//             "vendor_id": 1452
//         },
//         "is_built_in_keyboard": true,
//         "is_built_in_trackpad": false
//     }
// ]

namespace krbn {
class connected_devices final {
public:
  class device final {
  public:
    class descriptions {
    public:
      descriptions(const std::string& manufacturer,
                   const std::string& product) : manufacturer_(manufacturer),
                                                 product_(product) {
      }
      descriptions(const nlohmann::json& json) {
        if (auto v = json_utility::find_optional<std::string>(json, "manufacturer")) {
          manufacturer_ = *v;
        }

        if (auto v = json_utility::find_optional<std::string>(json, "product")) {
          product_ = *v;
        }
      }

      nlohmann::json to_json(void) const {
        return nlohmann::json({
            {"manufacturer", manufacturer_},
            {"product", product_},
        });
      }

      const std::string& get_manufacturer(void) const {
        return manufacturer_;
      }

      const std::string& get_product(void) const {
        return product_;
      }

      bool operator==(const descriptions& other) const {
        return manufacturer_ == other.manufacturer_ &&
               product_ == other.product_;
      }
      bool operator!=(const descriptions& other) const {
        return !(*this == other);
      }

    private:
      std::string manufacturer_;
      std::string product_;
    };

    device(const descriptions& descriptions,
           const device_identifiers& identifiers,
           bool is_built_in_keyboard,
           bool is_built_in_trackpad) : descriptions_(descriptions),
                                        identifiers_(identifiers),
                                        is_built_in_keyboard_(is_built_in_keyboard),
                                        is_built_in_trackpad_(is_built_in_trackpad) {
    }
    device(const nlohmann::json& json) : descriptions_(json_utility::find_copy(json, "descriptions", nlohmann::json())),
                                         identifiers_(json_utility::find_copy(json, "identifiers", nlohmann::json())),
                                         is_built_in_keyboard_(false),
                                         is_built_in_trackpad_(false) {
      if (auto v = json_utility::find_optional<bool>(json, "is_built_in_keyboard")) {
        is_built_in_keyboard_ = *v;
      }

      if (auto v = json_utility::find_optional<bool>(json, "is_built_in_trackpad")) {
        is_built_in_trackpad_ = *v;
      }
    }

    nlohmann::json to_json(void) const {
      return nlohmann::json({
          {"descriptions", descriptions_},
          {"identifiers", identifiers_},
          {"is_built_in_keyboard", is_built_in_keyboard_},
          {"is_built_in_trackpad", is_built_in_trackpad_},
      });
    }

    const descriptions& get_descriptions(void) const {
      return descriptions_;
    }

    const device_identifiers& get_identifiers(void) const {
      return identifiers_;
    }

    bool get_is_built_in_keyboard(void) const {
      return is_built_in_keyboard_;
    }

    bool get_is_built_in_trackpad(void) const {
      return is_built_in_trackpad_;
    }

    bool operator==(const device& other) const {
      return identifiers_ == other.identifiers_;
    }

  private:
    descriptions descriptions_;
    device_identifiers identifiers_;
    bool is_built_in_keyboard_;
    bool is_built_in_trackpad_;
  };

  connected_devices(void) : loaded_(true) {
  }

  connected_devices(const std::string& file_path) : loaded_(true) {
    std::ifstream input(file_path);
    if (input) {
      try {
        auto json = nlohmann::json::parse(input);
        if (json.is_array()) {
          for (const auto& j : json) {
            devices_.emplace_back(device(j));
          }
        }
      } catch (std::exception& e) {
        logger::get_logger().error("parse error in {0}: {1}", file_path, e.what());
        loaded_ = false;
      }
    }
  }

  nlohmann::json to_json(void) const {
    return nlohmann::json(devices_);
  }

  bool is_loaded(void) const { return loaded_; }

  const std::vector<device>& get_devices(void) const {
    return devices_;
  }
  void push_back_device(const device& device) {
    if (std::find(std::begin(devices_), std::end(devices_), device) != std::end(devices_)) {
      return;
    }

    devices_.push_back(device);

    std::sort(devices_.begin(), devices_.end(), [](const class device& a, const class device& b) {
      auto a_name = a.get_descriptions().get_product() + a.get_descriptions().get_manufacturer();
      auto a_kb = a.get_identifiers().get_is_keyboard();
      auto a_pd = a.get_identifiers().get_is_pointing_device();

      auto b_name = b.get_descriptions().get_product() + b.get_descriptions().get_manufacturer();
      auto b_kb = b.get_identifiers().get_is_keyboard();
      auto b_pd = b.get_identifiers().get_is_pointing_device();

      if (a_name == b_name) {
        if (a_kb == b_kb) {
          if (a_pd == b_pd) {
            return false;
          } else {
            return a_pd;
          }
        } else {
          return a_kb;
        }
      } else {
        return a_name < b_name;
      }
      return true;
    });
  }
  void clear(void) {
    devices_.clear();
  }

  void save_to_file(const std::string& file_path) {
    json_utility::save_to_file(to_json(), file_path, 0755, 0644);
  }

private:
  bool loaded_;

  std::vector<device> devices_;
};

inline void to_json(nlohmann::json& json, const connected_devices::device::descriptions& descriptions) {
  json = descriptions.to_json();
}

inline void to_json(nlohmann::json& json, const connected_devices::device& device) {
  json = device.to_json();
}
} // namespace krbn
