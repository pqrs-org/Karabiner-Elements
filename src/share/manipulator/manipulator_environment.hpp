#pragma once

#include "core_configuration/core_configuration.hpp"
#include "device_properties_manager.hpp"
#include "json_writer.hpp"
#include "logger.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <pqrs/filesystem.hpp>
#include <pqrs/osx/frontmost_application_monitor.hpp>
#include <pqrs/osx/frontmost_application_monitor/extra/nlohmann_json.hpp>
#include <pqrs/osx/input_source.hpp>
#include <pqrs/osx/input_source/extra/nlohmann_json.hpp>
#include <pqrs/osx/system_preferences.hpp>
#include <pqrs/osx/system_preferences/extra/nlohmann_json.hpp>
#include <string>

namespace krbn {
namespace manipulator {
class manipulator_environment final {
public:
  manipulator_environment(const manipulator_environment&) = delete;

  manipulator_environment(void) {
  }

  nlohmann::json to_json(void) const {
    nlohmann::json input_source_json;
    if (auto& v = input_source_properties_.get_first_language()) {
      input_source_json["language"] = *v;
    }
    if (auto& v = input_source_properties_.get_input_source_id()) {
      input_source_json["input_source_id"] = *v;
    }
    if (auto& v = input_source_properties_.get_input_mode_id()) {
      input_source_json["input_mode_id"] = *v;
    }

    return nlohmann::json({
        {"frontmost_application", frontmost_application_},
        {"input_source", input_source_json},
        {"variables", variables_},
        {"system_preferences_properties", system_preferences_properties_},
        {"virtual_hid_devices_state", virtual_hid_devices_state_},
        {"virtual_hid_keyboard_keyboard_type", virtual_hid_keyboard_keyboard_type_},
    });
  }

  void enable_json_output(const std::string& output_json_file_path) {
    output_json_file_path_ = output_json_file_path;
  }

  void disable_json_output(void) {
    output_json_file_path_.clear();
  }

  const device_properties_manager& get_device_properties_manager(void) const {
    return device_properties_manager_;
  }

  std::shared_ptr<device_properties> find_device_properties(device_id device_id) const {
    return device_properties_manager_.find(device_id);
  }

  void insert_device_properties(device_id device_id,
                                const device_properties& device_properties) {
    device_properties_manager_.insert(device_id, device_properties);
  }

  void erase_device_properties(device_id device_id) {
    device_properties_manager_.erase(device_id);
  }

  const pqrs::osx::frontmost_application_monitor::application& get_frontmost_application(void) const {
    return frontmost_application_;
  }

  void set_frontmost_application(const pqrs::osx::frontmost_application_monitor::application& value) {
    frontmost_application_ = value;
    async_save_to_file();
  }

  const pqrs::osx::input_source::properties& get_input_source_properties(void) const {
    return input_source_properties_;
  }

  void set_input_source_properties(const pqrs::osx::input_source::properties& value) {
    input_source_properties_ = value;
    async_save_to_file();
  }

  manipulator_environment_variable get_variable(const std::string& name) const {
    auto it = variables_.find(name);
    if (it != std::end(variables_)) {
      return it->second;
    }
    return manipulator_environment_variable();
  }

  void set_variable(const std::string& name, const manipulator_environment_variable& value) {
    // logger::get_logger()->info("set_variable {0} {1}", name, value);
    variables_[name] = value;
    async_save_to_file();
  }

  std::weak_ptr<const core_configuration::core_configuration> get_core_configuration(void) const {
    return core_configuration_;
  }

  void set_core_configuration(std::weak_ptr<const core_configuration::core_configuration> core_configuration) {
    core_configuration_ = core_configuration;
    update_virtual_hid_keyboard_keyboard_type();
    async_save_to_file();
  }

  const pqrs::osx::system_preferences::properties& get_system_preferences_properties(void) const {
    return system_preferences_properties_;
  }

  void set_system_preferences_properties(const pqrs::osx::system_preferences::properties& value) {
    system_preferences_properties_ = value;
    update_virtual_hid_keyboard_keyboard_type();
    async_save_to_file();
  }

  void set_virtual_hid_devices_state(const virtual_hid_devices_state& value) {
    virtual_hid_devices_state_ = value;
    async_save_to_file();
  }

  const std::string& get_virtual_hid_keyboard_keyboard_type(void) const {
    return virtual_hid_keyboard_keyboard_type_;
  }

private:
  void async_save_to_file(void) const {
    if (!output_json_file_path_.empty()) {
      json_writer::async_save_to_file(to_json(), output_json_file_path_, 0755, 0644);
    }
  }

  void update_virtual_hid_keyboard_keyboard_type(void) {
    pqrs::hid::country_code::value_t country_code(0);
    if (auto c = core_configuration_.lock()) {
      country_code = c->get_selected_profile().get_virtual_hid_keyboard().get_country_code();
    }

    pqrs::osx::system_preferences::keyboard_type_key key(
        hid::vendor_id::karabiner_virtual_hid_device,
        hid::product_id::karabiner_virtual_hid_keyboard,
        country_code);
    auto& keyboard_types = system_preferences_properties_.get_keyboard_types();
    auto it = keyboard_types.find(key);
    if (it != std::end(keyboard_types)) {
      virtual_hid_keyboard_keyboard_type_ = pqrs::osx::iokit_keyboard_type::make_string(it->second);
    } else {
      virtual_hid_keyboard_keyboard_type_.clear();
    }
  }

  std::string output_json_file_path_;
  device_properties_manager device_properties_manager_;
  pqrs::osx::frontmost_application_monitor::application frontmost_application_;
  pqrs::osx::input_source::properties input_source_properties_;
  std::unordered_map<std::string, manipulator_environment_variable> variables_;
  std::weak_ptr<const core_configuration::core_configuration> core_configuration_;
  pqrs::osx::system_preferences::properties system_preferences_properties_;
  std::string virtual_hid_keyboard_keyboard_type_; // cache value
  virtual_hid_devices_state virtual_hid_devices_state_;
};
} // namespace manipulator
} // namespace krbn
