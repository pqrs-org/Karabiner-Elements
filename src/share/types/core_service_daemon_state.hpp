#pragma once

#include "core_service_permission_check_result.hpp"
#include <nlohmann/json.hpp>
#include <optional>
#include <pqrs/json.hpp>
#include <string>

namespace krbn {
class core_service_daemon_state final {
public:
  const std::optional<core_service_permission_check_result>& get_current_process_permission_check_result() const {
    return current_process_permission_check_result_;
  }

  void set_current_process_permission_check_result(const std::optional<core_service_permission_check_result>& value) {
    current_process_permission_check_result_ = value;
  }

  const std::optional<core_service_permission_check_result>& get_bundle_permission_check_result() const {
    return bundle_permission_check_result_;
  }

  void set_bundle_permission_check_result(const std::optional<core_service_permission_check_result>& value) {
    bundle_permission_check_result_ = value;
  }

  const std::optional<bool>& get_virtual_hid_device_service_client_connected() const {
    return virtual_hid_device_service_client_connected_;
  }

  void set_virtual_hid_device_service_client_connected(const std::optional<bool>& value) {
    virtual_hid_device_service_client_connected_ = value;
  }

  const std::optional<bool>& get_driver_activated() const {
    return driver_activated_;
  }

  void set_driver_activated(const std::optional<bool>& value) {
    driver_activated_ = value;
  }

  const std::optional<bool>& get_driver_connected() const {
    return driver_connected_;
  }

  void set_driver_connected(const std::optional<bool>& value) {
    driver_connected_ = value;
  }

  const std::optional<bool>& get_driver_version_mismatched() const {
    return driver_version_mismatched_;
  }

  void set_driver_version_mismatched(const std::optional<bool>& value) {
    driver_version_mismatched_ = value;
  }

  const std::optional<bool>& get_virtual_hid_keyboard_ready() const {
    return virtual_hid_keyboard_ready_;
  }

  void set_virtual_hid_keyboard_ready(const std::optional<bool>& value) {
    virtual_hid_keyboard_ready_ = value;
  }

  const std::optional<bool>& get_virtual_hid_keyboard_type_not_set() const {
    return virtual_hid_keyboard_type_not_set_;
  }

  void set_virtual_hid_keyboard_type_not_set(const std::optional<bool>& value) {
    virtual_hid_keyboard_type_not_set_ = value;
  }

  const std::string& get_karabiner_json_parse_error_message() const {
    return karabiner_json_parse_error_message_;
  }

  void set_karabiner_json_parse_error_message(const std::string& value) {
    karabiner_json_parse_error_message_ = value;
  }

  bool operator==(const core_service_daemon_state&) const = default;

private:
  std::optional<core_service_permission_check_result> current_process_permission_check_result_;
  // If permissions such as Accessibility are granted to Karabiner-Core-Service from System Settings after the process has started,
  // the permission state of the current process and that of the application bundle itself may no longer match.
  // Since the Settings window alert logic should be based on the application bundle state,
  // the permissions of the current process and the application bundle are stored separately.
  std::optional<core_service_permission_check_result> bundle_permission_check_result_;
  std::optional<bool> virtual_hid_device_service_client_connected_;
  std::optional<bool> driver_activated_;
  std::optional<bool> driver_connected_;
  std::optional<bool> driver_version_mismatched_;
  std::optional<bool> virtual_hid_keyboard_ready_;
  std::optional<bool> virtual_hid_keyboard_type_not_set_;
  std::string karabiner_json_parse_error_message_;
};

inline void to_json(nlohmann::json& json, const core_service_daemon_state& value) {
  json = nlohmann::json::object();

  if (auto permission_check_result = value.get_current_process_permission_check_result()) {
    json["current_process_permission_check_result"] = *permission_check_result;
  }

  if (auto permission_check_result = value.get_bundle_permission_check_result()) {
    json["bundle_permission_check_result"] = *permission_check_result;
  }

  if (auto v = value.get_virtual_hid_device_service_client_connected()) {
    json["virtual_hid_device_service_client_connected"] = *v;
  }

  if (auto v = value.get_driver_activated()) {
    json["driver_activated"] = *v;
  }

  if (auto v = value.get_driver_connected()) {
    json["driver_connected"] = *v;
  }

  if (auto v = value.get_driver_version_mismatched()) {
    json["driver_version_mismatched"] = *v;
  }

  if (auto v = value.get_virtual_hid_keyboard_ready()) {
    json["virtual_hid_keyboard_ready"] = *v;
  }

  if (auto v = value.get_virtual_hid_keyboard_type_not_set()) {
    json["virtual_hid_keyboard_type_not_set"] = *v;
  }

  json["karabiner_json_parse_error_message"] = value.get_karabiner_json_parse_error_message();
}

inline void from_json(const nlohmann::json& json, core_service_daemon_state& value) {
  value.set_current_process_permission_check_result(pqrs::json::find<core_service_permission_check_result>(json, "current_process_permission_check_result"));
  value.set_bundle_permission_check_result(pqrs::json::find<core_service_permission_check_result>(json, "bundle_permission_check_result"));
  value.set_virtual_hid_device_service_client_connected(pqrs::json::find<bool>(json, "virtual_hid_device_service_client_connected"));
  value.set_driver_activated(pqrs::json::find<bool>(json, "driver_activated"));
  value.set_driver_connected(pqrs::json::find<bool>(json, "driver_connected"));
  value.set_driver_version_mismatched(pqrs::json::find<bool>(json, "driver_version_mismatched"));
  value.set_virtual_hid_keyboard_ready(pqrs::json::find<bool>(json, "virtual_hid_keyboard_ready"));
  value.set_virtual_hid_keyboard_type_not_set(pqrs::json::find<bool>(json, "virtual_hid_keyboard_type_not_set"));
  value.set_karabiner_json_parse_error_message(json.value("karabiner_json_parse_error_message",
                                                          std::string()));
}
} // namespace krbn
