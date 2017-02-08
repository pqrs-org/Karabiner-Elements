#pragma once

#include "constants.hpp"
#include "types.hpp"
#include <fstream>
#include <json/json.hpp>
#include <natural_sort/natural_sort.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <unordered_map>

// Example:
//
// {
//     "global": {
//         "check_for_updates_on_startup": true,
//         "show_in_menu_bar": true,
//         "show_profile_name_in_menu_bar": true
//     },
//     "profiles": [
//         {
//             "name": "Default profile",
//             "selected": true,
//             "simple_modifications": {
//                 "caps_lock": "delete_or_backspace",
//                 "escape": "spacebar"
//             },
//             "fn_function_keys": {
//                 "f1": "display_brightness_decrement",
//                 "f2": "display_brightness_increment",
//                 "f3": "mission_control",
//                 "f4": "launchpad",
//                 "f5": "illumination_decrement",
//                 "f6": "illumination_increment",
//                 "f7": "rewind",
//                 "f8": "play_or_pause",
//                 "f9": "fastforward",
//                 "f10": "mute",
//                 "f11": "volume_decrement",
//                 "f12": "volume_increment"
//             },
//             "virtual_hid_keyboard": {
//                 "keyboard_type": "ansi",
//                 "caps_lock_delay_milliseconds": 100
//             },
//             "devices": [
//                 {
//                     "identifiers": {
//                         "vendor_id": 1133,
//                         "product_id": 50475,
//                         "is_keyboard": true,
//                         "is_pointing_device": false
//                     },
//                     "ignore": false
//                 },
//                 {
//                     "identifiers": {
//                         "vendor_id": 1452,
//                         "product_id": 610,
//                         "is_keyboard": true,
//                         "is_pointing_device": false
//                     },
//                     "ignore": true,
//                     "disable_built_in_keyboard_if_exists": true
//                 }
//             ]
//         },
//         {
//             "name": "Empty",
//             "selected": false
//         }
//     ]
// }

class core_configuration final {
public:
  class global_configuration final {
  public:
    global_configuration(const nlohmann::json& json) : json_(json),
                                                       check_for_updates_on_startup_(true),
                                                       show_in_menu_bar_(true),
                                                       show_profile_name_in_menu_bar_(false) {
      {
        const std::string key = "check_for_updates_on_startup";
        if (json.find(key) != json.end() && json[key].is_boolean()) {
          check_for_updates_on_startup_ = json[key];
        }
      }
      {
        const std::string key = "show_in_menu_bar";
        if (json.find(key) != json.end() && json[key].is_boolean()) {
          show_in_menu_bar_ = json[key];
        }
      }
      {
        const std::string key = "show_profile_name_in_menu_bar";
        if (json.find(key) != json.end() && json[key].is_boolean()) {
          show_profile_name_in_menu_bar_ = json[key];
        }
      }
    }

    nlohmann::json to_json(void) {
      auto j = json_;
      j["check_for_updates_on_startup"] = check_for_updates_on_startup_;
      j["show_in_menu_bar"] = show_in_menu_bar_;
      j["show_profile_name_in_menu_bar"] = show_profile_name_in_menu_bar_;
      return j;
    }

    bool get_check_for_updates_on_startup(void) {
      return check_for_updates_on_startup_;
    }
    void set_check_for_updates_on_startup(bool value) {
      check_for_updates_on_startup_ = value;
    }

    bool get_show_in_menu_bar(void) {
      return show_in_menu_bar_;
    }
    void set_show_in_menu_bar(bool value) {
      show_in_menu_bar_ = value;
    }

    bool get_show_profile_name_in_menu_bar(void) {
      return show_profile_name_in_menu_bar_;
    }
    void set_show_profile_name_in_menu_bar(bool value) {
      show_profile_name_in_menu_bar_ = value;
    }

  private:
    const nlohmann::json json_;
    bool check_for_updates_on_startup_;
    bool show_in_menu_bar_;
    bool show_profile_name_in_menu_bar_;
  };

  class profile final {
  public:
    class simple_modifications final {
    public:
      simple_modifications(const nlohmann::json& json) {
        if (json.is_object()) {
          for (auto it = json.begin(); it != json.end(); ++it) {
            // it.key() is always std::string.
            if (it.value().is_string()) {
              std::string value = it.value();
              pairs_.push_back(std::make_pair(it.key(), value));
            }
          }

          std::sort(pairs_.begin(), pairs_.end(), [](const std::pair<std::string, std::string>& a,
                                                     const std::pair<std::string, std::string>& b) {
            return SI::natural::compare<std::string>(a.first, b.first);
          });
        }
      }

      nlohmann::json to_json(void) {
        auto json = nlohmann::json::object();
        for (const auto& it : pairs_) {
          if (!it.first.empty() &&
              !it.second.empty() &&
              json.find(it.first) == json.end()) {
            json[it.first] = it.second;
          }
        }
        return json;
      }

      const std::vector<std::pair<std::string, std::string>>& get_pairs(void) {
        return pairs_;
      }

      void push_back_pair(void) {
        pairs_.push_back(std::make_pair("", ""));
      }

      void erase_pair(size_t index) {
        if (index < pairs_.size()) {
          pairs_.erase(pairs_.begin() + index);
        }
      }

      void replace_pair(size_t index, const std::string& from, const std::string& to) {
        if (index < pairs_.size()) {
          pairs_[index].first = from;
          pairs_[index].second = to;
        }
      }

      void replace_second(const std::string& from, const std::string& to) {
        for (auto&& it : pairs_) {
          if (it.first == from) {
            it.second = to;
            return;
          }
        }
      }

    private:
      std::vector<std::pair<std::string, std::string>> pairs_;
    };

    profile(const nlohmann::json& json) : json_(json),
                                          selected_(false) {
      {
        const std::string key = "name";
        if (json.find(key) != json.end() && json[key].is_string()) {
          name_ = json[key];
        }
      }
      {
        const std::string key = "selected";
        if (json.find(key) != json.end() && json[key].is_boolean()) {
          selected_ = json[key];
        }
      }
      {
        const std::string key = "simple_modifications";
        if (json.find(key) != json.end()) {
          simple_modifications_ = std::make_unique<simple_modifications>(json[key]);
        } else {
          simple_modifications_ = std::make_unique<simple_modifications>(nlohmann::json());
        }
      }
      {
        const std::string key = "fn_function_keys";

        // default values
        fn_function_keys_ = std::make_unique<simple_modifications>(nlohmann::json({
            {"f1", "display_brightness_decrement"},
            {"f2", "display_brightness_increment"},
            {"f3", "mission_control"},
            {"f4", "launchpad"},
            {"f5", "illumination_decrement"},
            {"f6", "illumination_increment"},
            {"f7", "rewind"},
            {"f8", "play_or_pause"},
            {"f9", "fastforward"},
            {"f10", "mute"},
            {"f11", "volume_decrement"},
            {"f12", "volume_increment"},
        }));

        if (json.find(key) != json.end() && json[key].is_object()) {
          for (auto it = json[key].begin(); it != json[key].end(); ++it) {
            // it.key() is always std::string.
            if (it.value().is_string()) {
              fn_function_keys_->replace_second(it.key(), it.value());
            }
          }
        }
      }
    }

    nlohmann::json to_json(void) {
      auto j = json_;
      j["name"] = name_;
      j["selected"] = selected_;
      j["simple_modifications"] = simple_modifications_->to_json();
      j["fn_function_keys"] = fn_function_keys_->to_json();
      return j;
    }

    const std::string& get_name(void) {
      return name_;
    }
    void set_name(const std::string& value) {
      name_ = value;
    }

    bool get_selected(void) {
      return selected_;
    }
    void set_selected(bool value) {
      selected_ = value;
    }

    const std::vector<std::pair<std::string, std::string>>& get_simple_modifications(void) {
      return simple_modifications_->get_pairs();
    }
    void push_back_simple_modification(void) {
      simple_modifications_->push_back_pair();
    }
    void erase_simple_modification(size_t index) {
      simple_modifications_->erase_pair(index);
    }
    void replace_simple_modification(size_t index, const std::string& from, const std::string& to) {
      simple_modifications_->replace_pair(index, from, to);
    }

    const std::vector<std::pair<std::string, std::string>>& get_fn_function_keys(void) {
      return fn_function_keys_->get_pairs();
    }
    void replace_fn_function_keys(const std::string& from, const std::string& to) {
      fn_function_keys_->replace_second(from, to);
    }

  private:
    const nlohmann::json json_;
    std::string name_;
    bool selected_;
    std::unique_ptr<simple_modifications> simple_modifications_;
    std::unique_ptr<simple_modifications> fn_function_keys_;
  };

  core_configuration(const core_configuration&) = delete;

  core_configuration(spdlog::logger& logger, const std::string& file_path) : logger_(logger), file_path_(file_path), loaded_(false) {
    std::ifstream input(file_path_);
    if (input) {
      try {
        json_ = nlohmann::json::parse(input);
        loaded_ = true;
      } catch (std::exception& e) {
        logger_.warn("parse error in {0}: {1}", file_path_, e.what());
      }

    } else {
      // If file is not found, use default values.
      loaded_ = true;
    }

    // ----------------------------------------
    // Add default values if needed.

    if (!json_["profiles"].is_array()) {
      json_["profiles"] = nlohmann::json::array();
    }
    if (json_["profiles"].empty()) {
      json_["profiles"].push_back(get_default_profile());
    }

    for (auto&& profile : json_["profiles"]) {
      // Use default value if fn_function_keys is not set.
      if (!profile["fn_function_keys"].is_object() || profile["fn_function_keys"].empty()) {
        auto default_profile = get_default_profile();
        profile["fn_function_keys"] = default_profile["fn_function_keys"];
      }
    }
  }

  std::string to_json_string(void) const {
    return json_.dump();
  }

  bool is_loaded(void) const { return loaded_; }

  // std::vector<from,to>
  std::vector<std::pair<krbn::key_code, krbn::key_code>> get_current_profile_simple_modifications(void) {
    auto profile = get_current_profile();
    return get_key_code_pair_from_json_object(profile["simple_modifications"]);
  }

  // std::vector<f1,display_brightness_decrement>
  std::vector<std::pair<krbn::key_code, krbn::key_code>> get_current_profile_fn_function_keys(void) {
    auto profile = get_current_profile();
    return get_key_code_pair_from_json_object(profile["fn_function_keys"]);
  }

  krbn::virtual_hid_keyboard_configuration_struct get_current_profile_virtual_hid_keyboard(void) {
    krbn::virtual_hid_keyboard_configuration_struct virtual_hid_keyboard_configuration_struct;

    auto profile = get_current_profile();
    if (profile["virtual_hid_keyboard"].is_object()) {
      std::string keyboard_type_name = profile["virtual_hid_keyboard"]["keyboard_type"];
      if (auto keyboard_type = krbn::types::get_keyboard_type(keyboard_type_name)) {
        virtual_hid_keyboard_configuration_struct.keyboard_type = *keyboard_type;
      }

      if (profile["virtual_hid_keyboard"]["caps_lock_delay_milliseconds"].is_number()) {
        virtual_hid_keyboard_configuration_struct.caps_lock_delay_milliseconds = profile["virtual_hid_keyboard"]["caps_lock_delay_milliseconds"];
      }
    }

    return virtual_hid_keyboard_configuration_struct;
  }

  std::vector<std::pair<krbn::device_identifiers_struct, krbn::device_configuration_struct>> get_current_profile_devices(void) {
    std::vector<std::pair<krbn::device_identifiers_struct, krbn::device_configuration_struct>> v;

    auto profile = get_current_profile();
    if (profile["devices"].is_array()) {
      for (auto&& device : profile["devices"]) {
        if (device["identifiers"].is_object() &&
            device["identifiers"]["vendor_id"].is_number() &&
            device["identifiers"]["product_id"].is_number() &&
            device["identifiers"]["is_keyboard"].is_boolean() &&
            device["identifiers"]["is_pointing_device"].is_boolean() &&
            device["ignore"].is_boolean()) {
          krbn::device_identifiers_struct device_identifiers_struct;
          device_identifiers_struct.vendor_id = krbn::vendor_id(static_cast<uint32_t>(device["identifiers"]["vendor_id"]));
          device_identifiers_struct.product_id = krbn::product_id(static_cast<uint32_t>(device["identifiers"]["product_id"]));
          device_identifiers_struct.is_keyboard = device["identifiers"]["is_keyboard"];
          device_identifiers_struct.is_pointing_device = device["identifiers"]["is_pointing_device"];

          krbn::device_configuration_struct device_configuration_struct;
          device_configuration_struct.ignore = device["ignore"];
          device_configuration_struct.disable_built_in_keyboard_if_exists = false;
          // disable_built_in_keyboard_if_exists is optional
          if (device["disable_built_in_keyboard_if_exists"].is_boolean()) {
            device_configuration_struct.disable_built_in_keyboard_if_exists = static_cast<bool>(device["disable_built_in_keyboard_if_exists"]);
          }

          v.push_back(std::make_pair(device_identifiers_struct, device_configuration_struct));
        }
      }
    }

    return v;
  }

  std::string get_current_profile_json(void) {
    return get_current_profile().dump();
  }

  bool get_global_check_for_updates_on_startup(void) {
    if (json_["global"]["check_for_updates_on_startup"].is_boolean()) {
      return json_["global"]["check_for_updates_on_startup"];
    }
    return true;
  }

  bool get_global_show_in_menu_bar(void) {
    auto value = json_["global"]["show_in_menu_bar"];
    if (value.is_boolean()) {
      return value;
    }
    return true;
  }

  bool get_global_show_profile_name_in_menu_bar(void) {
    auto value = json_["global"]["show_profile_name_in_menu_bar"];
    if (value.is_boolean()) {
      return value;
    }
    return false;
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

  static nlohmann::json get_default_profile(void) {
    nlohmann::json json;
    json["name"] = "Default profile";
    json["selected"] = true;
    json["simple_modifications"] = nlohmann::json::object();
    json["fn_function_keys"]["f1"] = "display_brightness_decrement";
    json["fn_function_keys"]["f2"] = "display_brightness_increment";
    json["fn_function_keys"]["f3"] = "mission_control";
    json["fn_function_keys"]["f4"] = "launchpad";
    json["fn_function_keys"]["f5"] = "illumination_decrement";
    json["fn_function_keys"]["f6"] = "illumination_increment";
    json["fn_function_keys"]["f7"] = "rewind";
    json["fn_function_keys"]["f8"] = "play_or_pause";
    json["fn_function_keys"]["f9"] = "fastforward";
    json["fn_function_keys"]["f10"] = "mute";
    json["fn_function_keys"]["f11"] = "volume_decrement";
    json["fn_function_keys"]["f12"] = "volume_increment";
    json["devices"] = nlohmann::json::array();
    return json;
  }

private:
  nlohmann::json get_current_profile(void) {
    if (json_.is_object() && json_["profiles"].is_array()) {
      for (auto&& profile : json_["profiles"]) {
        if (profile.is_object() && profile["selected"]) {
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
