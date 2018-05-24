#pragma once

#include "connected_devices.hpp"
#include "constants.hpp"
#include "filesystem.hpp"
#include "json_utility.hpp"
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
using namespace std::string_literals;

class core_configuration final {
public:
#include "core_configuration/global_configuration.hpp"

  class profile final {
  public:
#include "core_configuration/profile/complex_modifications.hpp"
#include "core_configuration/profile/simple_modifications.hpp"
#include "core_configuration/profile/virtual_hid_keyboard.hpp"

#include "core_configuration/profile/device.hpp"

    profile(const nlohmann::json& json) : json_(json),
                                          selected_(false),
                                          simple_modifications_(json_utility::find_copy(json, "simple_modifications", nlohmann::json::array())),
                                          fn_function_keys_(make_default_fn_function_keys_json()),
                                          complex_modifications_(json_utility::find_copy(json, "complex_modifications", nlohmann::json::object())),
                                          virtual_hid_keyboard_(json_utility::find_copy(json, "virtual_hid_keyboard", nlohmann::json::object())) {
      if (auto v = json_utility::find_optional<std::string>(json, "name")) {
        name_ = *v;
      }

      if (auto v = json_utility::find_optional<bool>(json, "selected")) {
        selected_ = *v;
      }

      if (auto v = json_utility::find_json(json, "fn_function_keys")) {
        fn_function_keys_.update(*v);
      }

      if (auto v = json_utility::find_array(json, "devices")) {
        for (const auto& device_json : *v) {
          devices_.emplace_back(device_json);
        }
      }
    }

    static nlohmann::json make_default_fn_function_keys_json(void) {
      auto json = nlohmann::json::array();

      json.push_back(nlohmann::json::object());
      json.back()["from"]["key_code"] = "f1";
      json.back()["to"]["consumer_key_code"] = "display_brightness_decrement";

      json.push_back(nlohmann::json::object());
      json.back()["from"]["key_code"] = "f2";
      json.back()["to"]["consumer_key_code"] = "display_brightness_increment";

      json.push_back(nlohmann::json::object());
      json.back()["from"]["key_code"] = "f3";
      json.back()["to"]["key_code"] = "mission_control";

      json.push_back(nlohmann::json::object());
      json.back()["from"]["key_code"] = "f4";
      json.back()["to"]["key_code"] = "launchpad";

      json.push_back(nlohmann::json::object());
      json.back()["from"]["key_code"] = "f5";
      json.back()["to"]["key_code"] = "illumination_decrement";

      json.push_back(nlohmann::json::object());
      json.back()["from"]["key_code"] = "f6";
      json.back()["to"]["key_code"] = "illumination_increment";

      json.push_back(nlohmann::json::object());
      json.back()["from"]["key_code"] = "f7";
      json.back()["to"]["consumer_key_code"] = "rewind";

      json.push_back(nlohmann::json::object());
      json.back()["from"]["key_code"] = "f8";
      json.back()["to"]["consumer_key_code"] = "play_or_pause";

      json.push_back(nlohmann::json::object());
      json.back()["from"]["key_code"] = "f9";
      json.back()["to"]["consumer_key_code"] = "fastforward";

      json.push_back(nlohmann::json::object());
      json.back()["from"]["key_code"] = "f10";
      json.back()["to"]["consumer_key_code"] = "mute";

      json.push_back(nlohmann::json::object());
      json.back()["from"]["key_code"] = "f11";
      json.back()["to"]["consumer_key_code"] = "volume_decrement";

      json.push_back(nlohmann::json::object());
      json.back()["from"]["key_code"] = "f12";
      json.back()["to"]["consumer_key_code"] = "volume_increment";

      return json;
    }

    nlohmann::json to_json(void) const {
      auto j = json_;
      j["name"] = name_;
      j["selected"] = selected_;
      j["simple_modifications"] = simple_modifications_;
      j["fn_function_keys"] = fn_function_keys_;
      j["complex_modifications"] = complex_modifications_;
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

    const simple_modifications& get_simple_modifications(void) const {
      return simple_modifications_;
    }
    simple_modifications& get_simple_modifications(void) {
      return const_cast<simple_modifications&>(static_cast<const profile&>(*this).get_simple_modifications());
    }

    const simple_modifications* find_simple_modifications(const device_identifiers& identifiers) const {
      for (const auto& d : devices_) {
        if (d.get_identifiers() == identifiers) {
          return &(d.get_simple_modifications());
        }
      }
      return nullptr;
    }
    simple_modifications* find_simple_modifications(const device_identifiers& identifiers) {
      add_device(identifiers);

      return const_cast<simple_modifications*>(static_cast<const profile&>(*this).find_simple_modifications(identifiers));
    }

    const simple_modifications& get_fn_function_keys(void) const {
      return fn_function_keys_;
    }
    simple_modifications& get_fn_function_keys(void) {
      return const_cast<simple_modifications&>(static_cast<const profile&>(*this).get_fn_function_keys());
    }

    const simple_modifications* find_fn_function_keys(const device_identifiers& identifiers) const {
      for (const auto& d : devices_) {
        if (d.get_identifiers() == identifiers) {
          return &(d.get_fn_function_keys());
        }
      }
      return nullptr;
    }
    simple_modifications* find_fn_function_keys(const device_identifiers& identifiers) {
      add_device(identifiers);

      return const_cast<simple_modifications*>(static_cast<const profile&>(*this).find_fn_function_keys(identifiers));
    }

    const complex_modifications& get_complex_modifications(void) const {
      return complex_modifications_;
    }
    void push_back_complex_modifications_rule(const profile::complex_modifications::rule& rule) {
      complex_modifications_.push_back_rule(rule);
    }
    void erase_complex_modifications_rule(size_t index) {
      complex_modifications_.erase_rule(index);
    }
    void swap_complex_modifications_rules(size_t index1, size_t index2) {
      complex_modifications_.swap_rules(index1, index2);
    }
    void set_complex_modifications_parameter(const std::string& name, int value) {
      complex_modifications_.set_parameter_value(name, value);
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

    bool get_device_ignore(const device_identifiers& identifiers) const {
      for (const auto& d : devices_) {
        if (d.get_identifiers() == identifiers) {
          return d.get_ignore();
        }
      }

      device d(nlohmann::json({
          {"identifiers", identifiers.to_json()},
      }));
      return d.get_ignore();
    }
    void set_device_ignore(const device_identifiers& identifiers,
                           bool ignore) {
      add_device(identifiers);

      for (auto&& device : devices_) {
        if (device.get_identifiers() == identifiers) {
          device.set_ignore(ignore);
          return;
        }
      }
    }

    bool get_device_manipulate_caps_lock_led(const device_identifiers& identifiers) const {
      for (const auto& d : devices_) {
        if (d.get_identifiers() == identifiers) {
          return d.get_manipulate_caps_lock_led();
        }
      }

      device d(nlohmann::json({
          {"identifiers", identifiers.to_json()},
      }));
      return d.get_manipulate_caps_lock_led();
    }
    void set_device_manipulate_caps_lock_led(const device_identifiers& identifiers,
                                             bool manipulate_caps_lock_led) {
      add_device(identifiers);

      for (auto&& device : devices_) {
        if (device.get_identifiers() == identifiers) {
          device.set_manipulate_caps_lock_led(manipulate_caps_lock_led);
          return;
        }
      }
    }

    bool get_device_disable_built_in_keyboard_if_exists(const device_identifiers& identifiers) const {
      for (const auto& d : devices_) {
        if (d.get_identifiers() == identifiers) {
          return d.get_disable_built_in_keyboard_if_exists();
        }
      }
      return false;
    }
    void set_device_disable_built_in_keyboard_if_exists(const device_identifiers& identifiers,
                                                        bool disable_built_in_keyboard_if_exists) {
      add_device(identifiers);

      for (auto&& device : devices_) {
        if (device.get_identifiers() == identifiers) {
          device.set_disable_built_in_keyboard_if_exists(disable_built_in_keyboard_if_exists);
          return;
        }
      }
    }

  private:
    void add_device(const device_identifiers& identifiers) {
      for (auto&& device : devices_) {
        if (device.get_identifiers() == identifiers) {
          return;
        }
      }

      auto json = nlohmann::json({
          {"identifiers", identifiers.to_json()},
      });
      devices_.emplace_back(json);
    }

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

            if (auto v = json_utility::find_object(json_, "global")) {
              global_configuration_ = global_configuration(*v);
            }

            if (auto v = json_utility::find_array(json_, "profiles")) {
              for (const auto& profile_json : *v) {
                profiles_.emplace_back(profile_json);
              }
            }

          } catch (std::exception& e) {
            logger::get_logger().error("parse error in {0}: {1}", file_path, e.what());
            json_ = nlohmann::json();
            loaded_ = false;
          }
        } else {
          logger::get_logger().error("Failed to open {0}", file_path);
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

  void save_to_file(const std::string& file_path) {
    json_utility::save_to_file(to_json(), file_path, 0700, 0600);
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

inline void to_json(nlohmann::json& json, const core_configuration::profile::complex_modifications::rule& rule) {
  json = rule.get_json();
}

inline void to_json(nlohmann::json& json, const core_configuration::profile::complex_modifications::parameters& parameters) {
  json = parameters.to_json();
}

inline void to_json(nlohmann::json& json, const core_configuration::profile::virtual_hid_keyboard& virtual_hid_keyboard) {
  json = virtual_hid_keyboard.to_json();
}

inline void to_json(nlohmann::json& json, const core_configuration::profile::device& device) {
  json = device.to_json();
}
} // namespace krbn
