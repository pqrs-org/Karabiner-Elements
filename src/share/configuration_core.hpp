#pragma once

#include "constants.hpp"
#include "types.hpp"
#include <fstream>
#include <json/json.hpp>
#include <spdlog/spdlog.h>

// Example:
//
// {
//     "profiles": [{
//         "name": "Default profile",
//         "selected": true,
//         "simple_modifications": {
//             "caps_lock": "delete_or_backspace",
//             "escape": "spacebar"
//         },
//         "fn_function_keys": {
//             "f1": "vk_consumer_brightness_down",
//             "f2": "vk_consumer_brightness_up",
//             "f3": "vk_mission_control",
//             "f4": "vk_launchpad",
//             "f5": "vk_consumer_illumination_down",
//             "f6": "vk_consumer_illumination_up",
//             "f7": "vk_consumer_previous",
//             "f8": "vk_consumer_play",
//             "f9": "vk_consumer_next",
//             "f10": "mute",
//             "f11": "volume_down",
//             "f12": "volume_up"
//         },
//         "devices": [{
//             "vendor_id": 1133,
//             "product_id": 50475,
//             "ignore": false
//         }, {
//             "vendor_id": 1452,
//             "product_id": 610,
//             "ignore": true
//         }]
//     }, {
//         "name": "Empty",
//         "selected": false
//     }]
// }

class configuration_core final {
public:
  configuration_core(const configuration_core&) = delete;

  configuration_core(spdlog::logger& logger, const std::string& file_path) : logger_(logger), file_path_(file_path), loaded_(false) {
    std::ifstream input(file_path_);
    if (input) {
      try {
        json_ = nlohmann::json::parse(input);
        loaded_ = true;
      } catch (std::exception& e) {
        logger_.warn("parse error in {0}: {1}", file_path_, e.what());
      }
    }
  }

  configuration_core(spdlog::logger& logger) : configuration_core(logger, get_file_path()) {
  }

  static std::string get_file_path(void) {
    std::string file_path;
    if (auto p = constants::get_configuration_directory()) {
      file_path = p;
      file_path += "/karabiner-elements.json";
    }
    return file_path;
  }

  bool is_loaded(void) const { return loaded_; }

  // std::vector<from,to>
  std::vector<std::pair<krbn::key_code, krbn::key_code>> get_current_profile_simple_modifications(void) {
    auto profile = get_current_profile();
    return get_key_code_pair_from_json_object(profile["simple_modifications"]);
  }

  // std::vector<f1,vk_consumer_brightness_down>
  std::vector<std::pair<krbn::key_code, krbn::key_code>> get_current_profile_fn_function_keys(void) {
    auto profile = get_current_profile();
    return get_key_code_pair_from_json_object(profile["fn_function_keys"]);
  }

  std::vector<std::pair<uint64_t, bool>> get_current_profile_devices(void) {
    std::vector<std::pair<uint64_t, bool>> v;

    auto profile = get_current_profile();
    if (profile["devices"].is_array()) {
      for (const auto& device : profile["devices"]) {
        if (device["vendor_id"].is_number() &&
            device["product_id"].is_number() &&
            device["ignore"].is_boolean()) {
          uint32_t vendor_id = device["vendor_id"];
          uint32_t product_id = device["product_id"];
          bool ignore = device["ignore"];
          uint64_t key = (static_cast<uint64_t>(vendor_id) << 32) | product_id;
          v.push_back(std::make_pair(key, ignore));
        }
      }
    }

    return v;
  }

  std::string get_current_profile_json(void) {
    return get_current_profile().dump();
  }

  // Note:
  // Be careful calling `save` method.
  // If the configuration file is corrupted temporarily (user editing the configuration file in editor),
  // the user data will be lost by the `save` method.
  // Thus, we should call the `save` method only when it is neccessary.

  bool save(void) {
    std::ofstream output(file_path_);
    if (!output) {
      return false;
    }

    output << std::setw(4) << json_ << std::endl;
    return true;
  }

private:
  nlohmann::json get_default_profile(void) const {
    nlohmann::json json;
    json["name"] = "Default profile";
    json["selected"] = true;
    json["simple_modifications"] = nlohmann::json::object();
    json["fn_function_keys"]["f1"] = "vk_consumer_brightness_down";
    json["fn_function_keys"]["f2"] = "vk_consumer_brightness_up";
    json["fn_function_keys"]["f3"] = "vk_mission_control";
    json["fn_function_keys"]["f4"] = "vk_launchpad";
    json["fn_function_keys"]["f5"] = "vk_consumer_illumination_down";
    json["fn_function_keys"]["f6"] = "vk_consumer_illumination_up";
    json["fn_function_keys"]["f7"] = "vk_consumer_previous";
    json["fn_function_keys"]["f8"] = "vk_consumer_play";
    json["fn_function_keys"]["f9"] = "vk_consumer_next";
    json["fn_function_keys"]["f10"] = "mute";
    json["fn_function_keys"]["f11"] = "volume_down";
    json["fn_function_keys"]["f12"] = "volume_up";
    json["device_configuration"] = nlohmann::json::array();
    return json;
  }

  nlohmann::json get_current_profile(void) {
    if (json_.is_object() && json_["profiles"].is_array()) {
      for (auto&& profile : json_["profiles"]) {
        if (profile.is_object() && profile["selected"]) {
          // Use default value if fn_function_keys is not set.
          if (!profile["fn_function_keys"].is_object()) {
            auto default_profile = get_default_profile();
            profile["fn_function_keys"] = default_profile["fn_function_keys"];
          }

          return profile;
        }
      }
    }
    return get_default_profile();
  }

  std::vector<std::pair<krbn::key_code, krbn::key_code>> get_key_code_pair_from_json_object(const nlohmann::json& json) {
    std::vector<std::pair<krbn::key_code, krbn::key_code>> v;

    if (json.is_object()) {
      for (auto it = json.begin(); it != json.end(); ++it) {
        std::string from = it.key();
        std::string to = it.value();

        auto from_key_code = krbn::types::get_key_code(from);
        if (!from_key_code) {
          logger_.warn("unknown key_code:{0} in {1}", from, file_path_);
          continue;
        }
        auto to_key_code = krbn::types::get_key_code(to);
        if (!to_key_code) {
          logger_.warn("unknown key_code:{0} in {1}", to, file_path_);
          continue;
        }

        v.push_back(std::make_pair(*from_key_code, *to_key_code));
      }
    }

    return v;
  }

  spdlog::logger& logger_;
  std::string file_path_;

  bool loaded_;
  nlohmann::json json_;
};
