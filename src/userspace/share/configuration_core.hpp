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
  configuration_core(spdlog::logger& logger) : logger_(logger) {
    std::ifstream input(get_file_path());
    if (!input) {
      logger.info("The configuration file is not found.");
      json_["profiles"][0] = get_default_profile();

    } else {
      json_ = nlohmann::json::parse(input);
    }
    std::cout << std::setw(4) << json_ << std::endl;
  }

  std::string get_file_path(void) {
    std::string file_path;
    if (auto p = constants::get_home_dot_karabiner_directory()) {
      file_path = p;
      file_path += "/karabiner-elements.json";
    }
    return file_path;
  }

  nlohmann::json get_default_profile(void) const {
    nlohmann::json json;
    json["name"] = "Default profile";
    json["selected"] = true;
    json["simple_modifications"] = nlohmann::json::array();
    json["fn_function_keys"]["f1"] = "consumer_brightness_down";
    json["fn_function_keys"]["f2"] = "consumer_brightness_up";
    json["fn_function_keys"]["f3"] = "mission_control";
    json["fn_function_keys"]["f4"] = "launchpad";
    json["fn_function_keys"]["f5"] = "consumer_illumination_down";
    json["fn_function_keys"]["f6"] = "consumer_illumination_up";
    json["fn_function_keys"]["f7"] = "consumer_previous";
    json["fn_function_keys"]["f8"] = "consumer_play";
    json["fn_function_keys"]["f9"] = "consumer_next";
    json["fn_function_keys"]["f10"] = "consumer_mute";
    json["fn_function_keys"]["f11"] = "consumer_sound_down";
    json["fn_function_keys"]["f12"] = "consumer_sound_up";
    return json;
  }

  // Note:
  // Be careful calling `save` method.
  // If the configuration file is corrupted temporarily (user editing the configuration file in editor),
  // the user data will be lost by the `save` method.
  // Thus, we should call the `save` method only when it is neccessary.

  bool save(void) {
    std::ofstream output(get_file_path());
    if (!output) {
      return false;
    }

    output << std::setw(4) << json_ << std::endl;
    return true;
  }

private:
  spdlog::logger& logger_;

  nlohmann::json json_;
};
