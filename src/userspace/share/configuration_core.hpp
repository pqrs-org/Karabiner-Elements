#pragma once

#include "constants.hpp"
#include "userspace_types.hpp"
#include "json/json.hpp"
#include <fstream>
#include <spdlog/spdlog.h>

// Example:
//
// {
//     "profiles": [
//         {
//             "name": "Default profile",
//             "selected": true,
//             "simple_modifications": [
//                 {
//                     "from": "caps_lock",
//                     "to": "delete_or_backspace"
//                 },
//                 {
//                     "from": "escape",
//                     "to": "spacebar"
//                 }
//             ],
//             "fn_function_keys": {
//                 "f1":  "vk_consumer_brightness_down",
//                 "f2":  "vk_consumer_brightness_up",
//                 "f3":  "vk_mission_control",
//                 "f4":  "vk_launchpad",
//                 "f5":  "vk_consumer_illumination_down",
//                 "f6":  "vk_consumer_illumination_up";
//                 "f7":  "vk_consumer_previous",
//                 "f8":  "vk_consumer_play",
//                 "f9":  "vk_consumer_next",
//                 "f10": "vk_consumer_mute",
//                 "f11": "vk_consumer_sound_down",
//                 "f12": "vk_consumer_sound_up"
//             },
//         },
//         {
//             "name": "Empty",
//             "selected": false,
//             "simple_modifications": [
//             ],
//             "fn_function_keys": {
//             }
//         }
//     ]
// }

class configuration_core final {
public:
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
  std::vector<std::pair<krbn::key_code, krbn::key_code>> get_current_profile_simple_modifications(void) const {
    std::vector<std::pair<krbn::key_code, krbn::key_code>> v;

    auto profile = get_current_profile();
    if (profile["simple_modifications"].is_array()) {
      for (const auto& it : profile["simple_modifications"]) {
        std::string from = it["from"];
        std::string to = it["to"];

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
    json["simple_modifications"] = nlohmann::json::array();
    json["fn_function_keys"]["f1"] = "vk_consumer_brightness_down";
    json["fn_function_keys"]["f2"] = "vk_consumer_brightness_up";
    json["fn_function_keys"]["f3"] = "vk_mission_control";
    json["fn_function_keys"]["f4"] = "vk_launchpad";
    json["fn_function_keys"]["f5"] = "vk_consumer_illumination_down";
    json["fn_function_keys"]["f6"] = "vk_consumer_illumination_up";
    json["fn_function_keys"]["f7"] = "vk_consumer_previous";
    json["fn_function_keys"]["f8"] = "vk_consumer_play";
    json["fn_function_keys"]["f9"] = "vk_consumer_next";
    json["fn_function_keys"]["f10"] = "vk_consumer_mute";
    json["fn_function_keys"]["f11"] = "vk_consumer_sound_down";
    json["fn_function_keys"]["f12"] = "vk_consumer_sound_up";
    return json;
  }

  nlohmann::json get_current_profile(void) const {
    if (json_.is_object() && json_["profiles"].is_array()) {
      for (auto&& profile : json_["profiles"]) {
        if (profile.is_object() && profile["selected"]) {
          return profile;
        }
      }
    }
    return get_default_profile();
  }

  spdlog::logger& logger_;
  std::string file_path_;

  bool loaded_;
  nlohmann::json json_;
};
