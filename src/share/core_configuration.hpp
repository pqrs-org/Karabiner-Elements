#pragma once

#include "constants.hpp"
#include "filesystem.hpp"
#include "logger.hpp"
#include "session.hpp"
#include "types.hpp"
#include <fstream>
#include <json/json.hpp>
#include <natural_sort/natural_sort.hpp>
#include <string>
#include <unordered_map>

// Example: tests/src/core_configuration/json/example.json

namespace krbn {
class core_configuration final {
public:
#include "core_configuration/global_configuration.hpp"

  class profile final {
  public:
#include "core_configuration/profile/complex_modifications.hpp"
#include "core_configuration/profile/device.hpp"
#include "core_configuration/profile/simple_modifications.hpp"
#include "core_configuration/profile/virtual_hid_keyboard.hpp"

    profile(const nlohmann::json& json) : json_(json),
                                          selected_(false),
                                          simple_modifications_(json.find("simple_modifications") != json.end() ? json["simple_modifications"] : nlohmann::json()),
                                          fn_function_keys_(nlohmann::json({
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
                                          })),
                                          complex_modifications_(json.find("complex_modifications") != json.end() ? json["complex_modifications"] : nlohmann::json()),
                                          virtual_hid_keyboard_(json.find("virtual_hid_keyboard") != json.end() ? json["virtual_hid_keyboard"] : nlohmann::json()) {
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
        const std::string key = "fn_function_keys";
        if (json.find(key) != json.end() && json[key].is_object()) {
          for (auto it = json[key].begin(); it != json[key].end(); ++it) {
            // it.key() is always std::string.
            if (it.value().is_string()) {
              fn_function_keys_.replace_second(it.key(), it.value());
            }
          }
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
      j["simple_modifications"] = simple_modifications_;
      j["fn_function_keys"] = fn_function_keys_;
      j["virtual_hid_keyboard"] = virtual_hid_keyboard_;
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
      return simple_modifications_.get_pairs();
    }
    void push_back_simple_modification(void) {
      simple_modifications_.push_back_pair();
    }
    void erase_simple_modification(size_t index) {
      simple_modifications_.erase_pair(index);
    }
    void replace_simple_modification(size_t index, const std::string& from, const std::string& to) {
      simple_modifications_.replace_pair(index, from, to);
    }
    const std::unordered_map<key_code, key_code> get_simple_modifications_key_code_map(void) const {
      return simple_modifications_.to_key_code_map();
    }

    const std::vector<std::pair<std::string, std::string>>& get_fn_function_keys(void) const {
      return fn_function_keys_.get_pairs();
    }
    void replace_fn_function_key(const std::string& from, const std::string& to) {
      fn_function_keys_.replace_second(from, to);
    }
    const std::unordered_map<key_code, key_code> get_fn_function_keys_key_code_map(void) const {
      return fn_function_keys_.to_key_code_map();
    }

    const complex_modifications& get_complex_modifications(void) const {
      return complex_modifications_;
    }

    const virtual_hid_keyboard& get_virtual_hid_keyboard(void) const {
      return virtual_hid_keyboard_;
    }
    virtual_hid_keyboard& get_virtual_hid_keyboard(void) {
      return virtual_hid_keyboard_;
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
    simple_modifications simple_modifications_;
    simple_modifications fn_function_keys_;
    complex_modifications complex_modifications_;
    virtual_hid_keyboard virtual_hid_keyboard_;
    std::vector<device> devices_;
  };

  core_configuration(const core_configuration&) = delete;

  core_configuration(const std::string& file_path) : loaded_(true),
                                                     global_configuration_(nlohmann::json()) {
    bool valid_file_owner = false;

    // Load karabiner.json only when the owner is root or current session user.
    if (filesystem::exists(file_path)) {
      if (filesystem::is_owned(file_path, 0)) {
        valid_file_owner = true;
      } else {
        if (auto console_user_id = session::get_current_console_user_id()) {
          if (filesystem::is_owned(file_path, *console_user_id)) {
            valid_file_owner = true;
          }
        }
      }

      if (!valid_file_owner) {
        logger::get_logger().warn("{0} is not owned by a valid user.", file_path);
        loaded_ = false;

      } else {
        std::ifstream input(file_path);
        if (input) {
          try {
            json_ = nlohmann::json::parse(input);

            {
              const std::string key = "global";
              if (json_.find(key) != json_.end()) {
                global_configuration_ = global_configuration(json_[key]);
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

          } catch (std::exception& e) {
            logger::get_logger().warn("parse error in {0}: {1}", file_path, e.what());
            json_ = nlohmann::json();
            loaded_ = false;
          }
        }
      }
    }

    // Fallbacks
    if (profiles_.empty()) {
      profiles_.emplace_back(nlohmann::json({
          {"name", "Default profile"},
          {"selected", true},
      }));
    }
  }

  nlohmann::json to_json(void) const {
    auto j = json_;
    j["global"] = global_configuration_;
    j["profiles"] = profiles_;
    return j;
  }

  bool is_loaded(void) const { return loaded_; }

  const global_configuration& get_global_configuration(void) const {
    return global_configuration_;
  }
  global_configuration& get_global_configuration(void) {
    return global_configuration_;
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

  // Note:
  // Be careful calling `save` method.
  // If the configuration file is corrupted temporarily (user editing the configuration file in editor),
  // the user data will be lost by the `save` method.
  // Thus, we should call the `save` method only when it is neccessary.

  bool save_to_file(const std::string& file_path) {
    filesystem::create_directory_with_intermediate_directories(filesystem::dirname(file_path), 0700);

    std::ofstream output(file_path);
    if (!output) {
      return false;
    }

    output << std::setw(4) << to_json() << std::endl;
    return true;
  }

private:
  nlohmann::json json_;
  bool loaded_;

  global_configuration global_configuration_;
  std::vector<profile> profiles_;
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

inline void to_json(nlohmann::json& json, const core_configuration::profile::complex_modifications& complex_modifications) {
  json = complex_modifications.to_json();
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
} // namespace krbn
