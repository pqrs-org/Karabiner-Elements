#pragma once

#include "core_configuration.hpp"
#include <algorithm>

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
//         "is_built_in_keyboard": false
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
//         "is_built_in_keyboard": true
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
        {
          const std::string key = "manufacturer";
          if (json.find(key) != json.end() && json[key].is_string()) {
            manufacturer_ = json[key];
          }
        }
        {
          const std::string key = "product";
          if (json.find(key) != json.end() && json[key].is_string()) {
            product_ = json[key];
          }
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
           const core_configuration::profile::device::identifiers& identifiers,
           bool is_built_in_keyboard) : descriptions_(descriptions),
                                        identifiers_(identifiers),
                                        is_built_in_keyboard_(is_built_in_keyboard) {
    }
    device(const nlohmann::json& json) : descriptions_(json.find("descriptions") != json.end() ? json["descriptions"] : nlohmann::json()),
                                         identifiers_(json.find("identifiers") != json.end() ? json["identifiers"] : nlohmann::json()),
                                         is_built_in_keyboard_(false) {
      {
        const std::string key = "is_built_in_keyboard";
        if (json.find(key) != json.end() && json[key].is_boolean()) {
          is_built_in_keyboard_ = json[key];
        }
      }
    }

    nlohmann::json to_json(void) const {
      return nlohmann::json({
          {"descriptions", descriptions_},
          {"identifiers", identifiers_},
          {"is_built_in_keyboard", is_built_in_keyboard_},
      });
    }

    const descriptions& get_descriptions(void) const {
      return descriptions_;
    }

    const core_configuration::profile::device::identifiers& get_identifiers(void) const {
      return identifiers_;
    }

    bool get_is_built_in_keyboard(void) const {
      return is_built_in_keyboard_;
    }

    bool operator==(const device& other) const {
      return identifiers_ == other.identifiers_;
    }

  private:
    descriptions descriptions_;
    core_configuration::profile::device::identifiers identifiers_;
    bool is_built_in_keyboard_;
  };

  connected_devices(void) : loaded_(true) {
  }

  connected_devices(spdlog::logger& logger, const std::string& file_path) : loaded_(true) {
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
        logger.warn("parse error in {0}: {1}", file_path, e.what());
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

    std::sort(devices_.begin(), devices_.end(), [](const class device& a,
                                                   const class device& b) {
      auto a_v = a.get_identifiers().get_vendor_id();
      auto a_p = a.get_identifiers().get_product_id();
      auto a_kb = a.get_identifiers().get_is_keyboard();
      auto a_pd = a.get_identifiers().get_is_pointing_device();

      auto b_v = b.get_identifiers().get_vendor_id();
      auto b_p = b.get_identifiers().get_product_id();
      auto b_kb = b.get_identifiers().get_is_keyboard();
      auto b_pd = b.get_identifiers().get_is_pointing_device();

      if (a_v == b_v) {
        if (a_p == b_p) {
          if (a_kb == b_kb) {
            if (a_pd == b_pd) {
              return true;
            } else {
              return a_pd;
            }
          } else {
            return a_kb;
          }
        } else {
          return a_p < b_p;
        }
      } else {
        return a_v < b_v;
      }
      return true;
    });
  }
  void clear(void) {
    devices_.clear();
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
}
