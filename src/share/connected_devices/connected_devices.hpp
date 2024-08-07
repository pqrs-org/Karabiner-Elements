#pragma once

#include "details/device.hpp"
#include "json_utility.hpp"
#include "json_writer.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <algorithm>
#include <fstream>
#include <gsl/gsl>
#include <nlohmann/json.hpp>
#include <pqrs/filesystem.hpp>

// Json example:
//
// [
//     {
//         "descriptions": {
//             "manufacturer": "Unknown",
//             "product": "HHKB-BT",
//             "transport": "Bluetooth"
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
//             "product": "Apple Internal Keyboard / Trackpad",
//             "transport": "USB"
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
namespace connected_devices {
class connected_devices final {
public:
  connected_devices(void) : loaded_(false) {
  }

  connected_devices(const std::string& file_path) : loaded_(false) {
    std::ifstream input(file_path);
    if (input) {
      try {
        auto json = json_utility::parse_jsonc(input);

        if (json.is_array()) {
          for (const auto& j : json) {
            devices_.push_back(std::make_shared<details::device>(j));
          }
        }

        loaded_ = true;

      } catch (std::exception& e) {
        logger::get_logger()->error("parse error in {0}: {1}", file_path, e.what());
      }
    }
  }

  nlohmann::json to_json(void) const {
    auto json = nlohmann::json::array();

    for (const auto& d : devices_) {
      json.push_back(*d);
    }

    return json;
  }

  bool is_loaded(void) const { return loaded_; }

  const std::vector<gsl::not_null<std::shared_ptr<details::device>>>& get_devices(void) const {
    return devices_;
  }

  std::shared_ptr<details::device> find_device(const device_identifiers& identifiers) const {
    auto it = std::find_if(std::begin(devices_),
                           std::end(devices_),
                           [&](const auto& d) {
                             return d->get_identifiers() == identifiers;
                           });
    if (it != std::end(devices_)) {
      return *it;
    }

    return nullptr;
  }

  void push_back_device(gsl::not_null<std::shared_ptr<details::device>> device) {
    if (find_device(device->get_identifiers())) {
      return;
    }

    devices_.push_back(device);

    std::sort(devices_.begin(),
              devices_.end(),
              [](const auto& a, const auto& b) {
                auto a_name = fmt::format("{0} {1} {2}",
                                          type_safe::get(a->get_descriptions().get_product()),
                                          type_safe::get(a->get_descriptions().get_manufacturer()),
                                          a->get_descriptions().get_transport());
                auto a_kb = a->get_identifiers().get_is_keyboard();
                auto a_pd = a->get_identifiers().get_is_pointing_device();

                auto b_name = fmt::format("{0} {1} {2}",
                                          type_safe::get(b->get_descriptions().get_product()),
                                          type_safe::get(b->get_descriptions().get_manufacturer()),
                                          b->get_descriptions().get_transport());
                auto b_kb = b->get_identifiers().get_is_keyboard();
                auto b_pd = b->get_identifiers().get_is_pointing_device();

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

  void async_save_to_file(const std::string& file_path) {
    json_writer::async_save_to_file(to_json(), file_path, 0755, 0644);
  }

private:
  bool loaded_;

  std::vector<gsl::not_null<std::shared_ptr<details::device>>> devices_;
};
} // namespace connected_devices
} // namespace krbn
