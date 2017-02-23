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

    nlohmann::json to_json(void) const {
      auto j = json_;
      j["check_for_updates_on_startup"] = check_for_updates_on_startup_;
      j["show_in_menu_bar"] = show_in_menu_bar_;
      j["show_profile_name_in_menu_bar"] = show_profile_name_in_menu_bar_;
      return j;
    }

    bool get_check_for_updates_on_startup(void) const {
      return check_for_updates_on_startup_;
    }
    void set_check_for_updates_on_startup(bool value) {
      check_for_updates_on_startup_ = value;
    }

    bool get_show_in_menu_bar(void) const {
      return show_in_menu_bar_;
    }
    void set_show_in_menu_bar(bool value) {
      show_in_menu_bar_ = value;
    }

    bool get_show_profile_name_in_menu_bar(void) const {
      return show_profile_name_in_menu_bar_;
    }
    void set_show_profile_name_in_menu_bar(bool value) {
      show_profile_name_in_menu_bar_ = value;
    }

  private:
    nlohmann::json json_;
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
              pairs_.emplace_back(it.key(), value);
            }
          }

          std::sort(pairs_.begin(), pairs_.end(), [](const std::pair<std::string, std::string>& a,
                                                     const std::pair<std::string, std::string>& b) {
            return SI::natural::compare<std::string>(a.first, b.first);
          });
        }
      }

      nlohmann::json to_json(void) const {
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

      const std::vector<std::pair<std::string, std::string>>& get_pairs(void) const {
        return pairs_;
      }

      void push_back_pair(void) {
        pairs_.emplace_back("", "");
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

      std::unordered_map<krbn::key_code, krbn::key_code> to_key_code_map(spdlog::logger& logger) const {
        std::unordered_map<krbn::key_code, krbn::key_code> map;

        for (const auto& it : pairs_) {
          auto& from_string = it.first;
          auto& to_string = it.second;

          auto from_key_code = krbn::types::get_key_code(from_string);
          if (!from_key_code) {
            logger.warn("unknown key_code:{0}", from_string);
            continue;
          }

          auto to_key_code = krbn::types::get_key_code(to_string);
          if (!to_key_code) {
            logger.warn("unknown key_code:{0}", to_string);
            continue;
          }

          map[*from_key_code] = *to_key_code;
        }

        return map;
      }

    private:
      std::vector<std::pair<std::string, std::string>> pairs_;
    };

    class virtual_hid_keyboard final {
    public:
      virtual_hid_keyboard(const nlohmann::json& json) : json_(json),
                                                         caps_lock_delay_milliseconds_(0) {
        {
          const std::string key = "keyboard_type";
          if (json.find(key) != json.end() && json[key].is_string()) {
            keyboard_type_ = json[key];
          } else {
            keyboard_type_ = "ansi";
          }
        }
        {
          const std::string key = "caps_lock_delay_milliseconds";
          if (json.find(key) != json.end() && json[key].is_number()) {
            caps_lock_delay_milliseconds_ = json[key];
          }
        }
      }

      nlohmann::json to_json(void) const {
        auto j = json_;
        j["keyboard_type"] = keyboard_type_;
        j["caps_lock_delay_milliseconds"] = caps_lock_delay_milliseconds_;
        return j;
      }

      const std::string& get_keyboard_type(void) const {
        return keyboard_type_;
      }
      void set_keyboard_type(const std::string& value) {
        keyboard_type_ = value;
      }

      uint32_t get_caps_lock_delay_milliseconds(void) const {
        return caps_lock_delay_milliseconds_;
      }
      void set_caps_lock_delay_milliseconds(uint32_t value) {
        caps_lock_delay_milliseconds_ = value;
      }

    private:
      nlohmann::json json_;
      std::string keyboard_type_;
      uint32_t caps_lock_delay_milliseconds_;
    };

    class device final {
    public:
      class identifiers final {
      public:
        identifiers(const nlohmann::json& json) : json_(json),
                                                  vendor_id_(krbn::vendor_id(0)),
                                                  product_id_(krbn::product_id(0)),
                                                  is_keyboard_(false),
                                                  is_pointing_device_(false) {
          {
            const std::string key = "vendor_id";
            if (json.find(key) != json.end() && json[key].is_number()) {
              vendor_id_ = krbn::vendor_id(static_cast<uint32_t>(json[key]));
            }
          }
          {
            const std::string key = "product_id";
            if (json.find(key) != json.end() && json[key].is_number()) {
              product_id_ = krbn::product_id(static_cast<uint32_t>(json[key]));
            }
          }
          {
            const std::string key = "is_keyboard";
            if (json.find(key) != json.end() && json[key].is_boolean()) {
              is_keyboard_ = json[key];
            }
          }
          {
            const std::string key = "is_pointing_device";
            if (json.find(key) != json.end() && json[key].is_boolean()) {
              is_pointing_device_ = json[key];
            }
          }
        }

        identifiers(krbn::vendor_id vendor_id,
                    krbn::product_id product_id,
                    bool is_keyboard,
                    bool is_pointing_device) : identifiers(nlohmann::json({
                                                   {"vendor_id", static_cast<uint32_t>(vendor_id)},
                                                   {"product_id", static_cast<uint32_t>(product_id)},
                                                   {"is_keyboard", is_keyboard},
                                                   {"is_pointing_device", is_pointing_device},
                                               })) {
        }

        nlohmann::json to_json(void) const {
          auto j = json_;
          j["vendor_id"] = static_cast<uint32_t>(vendor_id_);
          j["product_id"] = static_cast<uint32_t>(product_id_);
          j["is_keyboard"] = is_keyboard_;
          j["is_pointing_device"] = is_pointing_device_;
          return j;
        }

        krbn::vendor_id get_vendor_id(void) const {
          return vendor_id_;
        }
        void set_vendor_id(krbn::vendor_id value) {
          vendor_id_ = value;
        }

        krbn::product_id get_product_id(void) const {
          return product_id_;
        }
        void set_product_id(krbn::product_id value) {
          product_id_ = value;
        }

        bool get_is_keyboard(void) const {
          return is_keyboard_;
        }
        void set_is_keyboard(bool value) {
          is_keyboard_ = value;
        }

        bool get_is_pointing_device(void) const {
          return is_pointing_device_;
        }
        void set_is_pointing_device(bool value) {
          is_pointing_device_ = value;
        }

        bool operator==(const identifiers& other) const {
          return vendor_id_ == other.vendor_id_ &&
                 product_id_ == other.product_id_ &&
                 is_keyboard_ == other.is_keyboard_ &&
                 is_pointing_device_ == other.is_pointing_device_;
        }

      private:
        nlohmann::json json_;
        krbn::vendor_id vendor_id_;
        krbn::product_id product_id_;
        bool is_keyboard_;
        bool is_pointing_device_;
      };

      device(const nlohmann::json& json) : json_(json),
                                           ignore_(false),
                                           disable_built_in_keyboard_if_exists_(false) {
        {
          const std::string key = "identifiers";
          if (json.find(key) != json.end() && json[key].is_object()) {
            identifiers_ = std::make_unique<identifiers>(json[key]);
          } else {
            identifiers_ = std::make_unique<identifiers>(nullptr);
          }
        }
        {
          const std::string key = "ignore";
          if (json.find(key) != json.end() && json[key].is_boolean()) {
            ignore_ = json[key];
          }
        }
        {
          const std::string key = "disable_built_in_keyboard_if_exists";
          if (json.find(key) != json.end() && json[key].is_boolean()) {
            disable_built_in_keyboard_if_exists_ = json[key];
          }
        }
      }

      nlohmann::json to_json(void) const {
        auto j = json_;
        j["identifiers"] = *identifiers_;
        j["ignore"] = ignore_;
        j["disable_built_in_keyboard_if_exists"] = disable_built_in_keyboard_if_exists_;
        return j;
      }

      const identifiers& get_identifiers(void) const {
        return *identifiers_;
      }
      identifiers& get_identifiers(void) {
        return *identifiers_;
      }

      bool get_ignore(void) const {
        return ignore_;
      }
      void set_ignore(bool value) {
        ignore_ = value;
      }

      bool get_disable_built_in_keyboard_if_exists(void) const {
        return disable_built_in_keyboard_if_exists_;
      }
      void set_disable_built_in_keyboard_if_exists(bool value) {
        disable_built_in_keyboard_if_exists_ = value;
      }

    private:
      nlohmann::json json_;
      std::unique_ptr<identifiers> identifiers_;
      bool ignore_;
      bool disable_built_in_keyboard_if_exists_;
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
          simple_modifications_ = std::make_unique<simple_modifications>(nullptr);
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
      {
        const std::string key = "virtual_hid_keyboard";
        if (json.find(key) != json.end()) {
          virtual_hid_keyboard_ = std::make_unique<virtual_hid_keyboard>(json[key]);
        } else {
          virtual_hid_keyboard_ = std::make_unique<virtual_hid_keyboard>(nullptr);
        }
      }
      {
        const std::string key = "devices";
        if (json.find(key) != json.end() && json[key].is_array()) {
          for (const auto& device_json : json[key]) {
            devices_.emplace_back(device_json);
          }
        }
      }
    }

    nlohmann::json to_json(void) const {
      auto j = json_;
      j["name"] = name_;
      j["selected"] = selected_;
      j["simple_modifications"] = *simple_modifications_;
      j["fn_function_keys"] = *fn_function_keys_;
      j["virtual_hid_keyboard"] = *virtual_hid_keyboard_;
      j["devices"] = devices_;
      return j;
    }

    const std::string& get_name(void) const {
      return name_;
    }
    void set_name(const std::string& value) {
      name_ = value;
    }

    bool get_selected(void) const {
      return selected_;
    }
    void set_selected(bool value) {
      selected_ = value;
    }

    const std::vector<std::pair<std::string, std::string>>& get_simple_modifications(void) const {
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
    const std::unordered_map<krbn::key_code, krbn::key_code> get_simple_modifications_key_code_map(spdlog::logger& logger) const {
      return simple_modifications_->to_key_code_map(logger);
    }

    const std::vector<std::pair<std::string, std::string>>& get_fn_function_keys(void) const {
      return fn_function_keys_->get_pairs();
    }
    void replace_fn_function_key(const std::string& from, const std::string& to) {
      fn_function_keys_->replace_second(from, to);
    }
    const std::unordered_map<krbn::key_code, krbn::key_code> get_fn_function_keys_key_code_map(spdlog::logger& logger) const {
      return fn_function_keys_->to_key_code_map(logger);
    }

    const virtual_hid_keyboard& get_virtual_hid_keyboard(void) const {
      return *virtual_hid_keyboard_;
    }
    virtual_hid_keyboard& get_virtual_hid_keyboard(void) {
      return *virtual_hid_keyboard_;
    }

    const std::vector<device>& get_devices(void) const {
      return devices_;
    }
    bool get_device_ignore(const device::identifiers& identifiers) {
      for (const auto& d : devices_) {
        if (d.get_identifiers() == identifiers) {
          return d.get_ignore();
        }
      }
      return false;
    }
    void set_device_ignore(const device::identifiers& identifiers,
                           bool ignore) {
      for (auto&& device : devices_) {
        if (device.get_identifiers() == identifiers) {
          device.set_ignore(ignore);
          return;
        }
      }

      auto json = nlohmann::json({
          {"identifiers", identifiers.to_json()},
          {"ignore", ignore},
      });
      devices_.emplace_back(json);
    }
    bool get_device_disable_built_in_keyboard_if_exists(const device::identifiers& identifiers) {
      for (const auto& d : devices_) {
        if (d.get_identifiers() == identifiers) {
          return d.get_disable_built_in_keyboard_if_exists();
        }
      }
      return false;
    }
    void set_device_disable_built_in_keyboard_if_exists(const device::identifiers& identifiers,
                                                        bool disable_built_in_keyboard_if_exists) {
      for (auto&& device : devices_) {
        if (device.get_identifiers() == identifiers) {
          device.set_disable_built_in_keyboard_if_exists(disable_built_in_keyboard_if_exists);
          return;
        }
      }

      auto json = nlohmann::json({
          {"identifiers", identifiers.to_json()},
          {"disable_built_in_keyboard_if_exists", disable_built_in_keyboard_if_exists},
      });
      devices_.emplace_back(json);
    }

  private:
    nlohmann::json json_;
    std::string name_;
    bool selected_;
    std::unique_ptr<simple_modifications> simple_modifications_;
    std::unique_ptr<simple_modifications> fn_function_keys_;
    std::unique_ptr<virtual_hid_keyboard> virtual_hid_keyboard_;
    std::vector<device> devices_;
  };

  core_configuration(const core_configuration&) = delete;

  core_configuration(spdlog::logger& logger, const std::string& file_path) : logger_(logger), file_path_(file_path), loaded_(false) {
    std::ifstream input(file_path_);
    if (input) {
      try {
        json_ = nlohmann::json::parse(input);

        {
          const std::string key = "global";
          if (json_.find(key) != json_.end()) {
            global_configuration_ = std::make_unique<global_configuration>(json_[key]);
          }
        }
        {
          const std::string key = "profiles";
          if (json_.find(key) != json_.end() && json_[key].is_array()) {
            for (const auto& profile_json : json_[key]) {
              profiles_.emplace_back(profile_json);
            }
          }
        }

        loaded_ = true;
      } catch (std::exception& e) {
        logger_.warn("parse error in {0}: {1}", file_path_, e.what());
        json_ = nlohmann::json();
      }

    } else {
      // If file is not found, use default values.
      loaded_ = true;
      json_ = nlohmann::json();
    }

    if (!global_configuration_) {
      global_configuration_ = std::make_unique<global_configuration>(nullptr);
    }
    if (profiles_.empty()) {
      profiles_.emplace_back(nlohmann::json({
          {"name", "Default profile"},
          {"selected", true},
      }));
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

  nlohmann::json to_json(void) const {
    auto j = json_;
    j["global"] = *global_configuration_;
    j["profiles"] = profiles_;
    return j;
  }

  bool is_loaded(void) const { return loaded_; }

  const global_configuration& get_global_configuration(void) const {
    return *global_configuration_;
  }
  global_configuration& get_global_configuration(void) {
    return *global_configuration_;
  }

  const std::vector<profile>& get_profiles(void) const {
    return profiles_;
  }
  void set_profile_name(size_t index, const std::string name) {
    if (index < profiles_.size()) {
      profiles_[index].set_name(name);
    }
  }
  void select_profile(size_t index) {
    if (index < profiles_.size()) {
      for (size_t i = 0; i < profiles_.size(); ++i) {
        if (i == index) {
          profiles_[i].set_selected(true);
        } else {
          profiles_[i].set_selected(false);
        }
      }
    }
  }
  void push_back_profile(void) {
    profiles_.emplace_back(nlohmann::json({
        {"name", "New profile"},
    }));
  }
  void erase_profile(size_t index) {
    if (index < profiles_.size()) {
      if (profiles_.size() > 1) {
        profiles_.erase(profiles_.begin() + index);
      }
    }
  }

  profile& get_selected_profile(void) {
    for (auto&& profile : profiles_) {
      if (profile.get_selected()) {
        return profile;
      }
    }
    return profiles_[0];
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

    output << std::setw(4) << to_json() << std::endl;
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

  spdlog::logger& logger_;
  std::string file_path_;

  std::unique_ptr<global_configuration> global_configuration_;
  std::vector<profile> profiles_;

  bool loaded_;
  nlohmann::json json_;
};

inline void to_json(nlohmann::json& json, const core_configuration::global_configuration& global_configuration) {
  json = global_configuration.to_json();
}

inline void to_json(nlohmann::json& json, const core_configuration::profile& profile) {
  json = profile.to_json();
}

inline void to_json(nlohmann::json& json, const core_configuration::profile::simple_modifications& simple_modifications) {
  json = simple_modifications.to_json();
}

inline void to_json(nlohmann::json& json, const core_configuration::profile::virtual_hid_keyboard& virtual_hid_keyboard) {
  json = virtual_hid_keyboard.to_json();
}

inline void to_json(nlohmann::json& json, const core_configuration::profile::device::identifiers& identifiers) {
  json = identifiers.to_json();
}

inline void to_json(nlohmann::json& json, const core_configuration::profile::device& device) {
  json = device.to_json();
}
