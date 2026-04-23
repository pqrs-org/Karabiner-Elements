#pragma once

#include "types/core_service_daemon_state.hpp"
#include <nlohmann/json.hpp>

namespace krbn {
enum class settings_window_alert : uint8_t {
  none,
  doctor,
  settings,
  services_disabled,
  services_not_running,
  input_monitoring_permissions,
  accessibility,
  virtual_hid_device_service_client_not_connected,
  driver_version_mismatched,
  driver_not_activated,
  driver_not_connected,
};

NLOHMANN_JSON_SERIALIZE_ENUM(
    settings_window_alert,
    {
        {settings_window_alert::none, "none"},
        {settings_window_alert::doctor, "doctor"},
        {settings_window_alert::settings, "settings"},
        {settings_window_alert::services_disabled, "services_disabled"},
        {settings_window_alert::services_not_running, "services_not_running"},
        {settings_window_alert::input_monitoring_permissions, "input_monitoring_permissions"},
        {settings_window_alert::accessibility, "accessibility"},
        {settings_window_alert::virtual_hid_device_service_client_not_connected, "virtual_hid_device_service_client_not_connected"},
        {settings_window_alert::driver_version_mismatched, "driver_version_mismatched"},
        {settings_window_alert::driver_not_activated, "driver_not_activated"},
        {settings_window_alert::driver_not_connected, "driver_not_connected"},
    });

class settings_window_guidance_context final {
public:
  settings_window_guidance_context(void)
      : services_enabled_(true),
        core_daemons_enabled_(true),
        core_agents_enabled_(true),
        core_daemons_running_(true),
        core_agents_running_(true),
        services_waiting_seconds_(0) {
  }

  bool get_services_enabled(void) const {
    return services_enabled_;
  }

  void set_services_enabled(bool value) {
    services_enabled_ = value;
  }

  bool get_core_daemons_enabled(void) const {
    return core_daemons_enabled_;
  }

  void set_core_daemons_enabled(bool value) {
    core_daemons_enabled_ = value;
  }

  bool get_core_agents_enabled(void) const {
    return core_agents_enabled_;
  }

  void set_core_agents_enabled(bool value) {
    core_agents_enabled_ = value;
  }

  bool get_core_daemons_running(void) const {
    return core_daemons_running_;
  }

  void set_core_daemons_running(bool value) {
    core_daemons_running_ = value;
  }

  bool get_core_agents_running(void) const {
    return core_agents_running_;
  }

  void set_core_agents_running(bool value) {
    core_agents_running_ = value;
  }

  int get_services_waiting_seconds(void) const {
    return services_waiting_seconds_;
  }

  void set_services_waiting_seconds(int value) {
    services_waiting_seconds_ = value;
  }

private:
  bool services_enabled_;
  bool core_daemons_enabled_;
  bool core_agents_enabled_;
  bool core_daemons_running_;
  bool core_agents_running_;
  int services_waiting_seconds_;
};

inline void to_json(nlohmann::json& json, const settings_window_guidance_context& value) {
  json = nlohmann::json::object({
      {"services_enabled", value.get_services_enabled()},
      {"core_daemons_enabled", value.get_core_daemons_enabled()},
      {"core_agents_enabled", value.get_core_agents_enabled()},
      {"core_daemons_running", value.get_core_daemons_running()},
      {"core_agents_running", value.get_core_agents_running()},
      {"services_waiting_seconds", value.get_services_waiting_seconds()},
  });
}

inline void from_json(const nlohmann::json& json, settings_window_guidance_context& value) {
  value.set_services_enabled(json.at("services_enabled").get<bool>());
  value.set_core_daemons_enabled(json.at("core_daemons_enabled").get<bool>());
  value.set_core_agents_enabled(json.at("core_agents_enabled").get<bool>());
  value.set_core_daemons_running(json.at("core_daemons_running").get<bool>());
  value.set_core_agents_running(json.at("core_agents_running").get<bool>());
  value.set_services_waiting_seconds(json.at("services_waiting_seconds").get<int>());
}

class settings_window_guidance_state final {
public:
  settings_window_guidance_state(void)
      : current_alert_(settings_window_alert::none) {
  }

  settings_window_alert get_current_alert(void) const {
    return current_alert_;
  }

  void set_current_alert(settings_window_alert value) {
    current_alert_ = value;
  }

  const settings_window_guidance_context& get_guidance_context(void) const {
    return guidance_context_;
  }

  void set_guidance_context(const settings_window_guidance_context& value) {
    guidance_context_ = value;
  }

  const core_service_daemon_state& get_core_service_daemon_state(void) const {
    return core_service_deamon_state_;
  }

  void set_core_service_daemon_state(const core_service_daemon_state& value) {
    core_service_deamon_state_ = value;
  }

private:
  settings_window_alert current_alert_;
  settings_window_guidance_context guidance_context_;
  core_service_daemon_state core_service_deamon_state_;
};

inline void to_json(nlohmann::json& json, const settings_window_guidance_state& value) {
  json = nlohmann::json::object({
      {"current_alert", value.get_current_alert()},
      {"guidance_context", value.get_guidance_context()},
      {"core_service_daemon_state", value.get_core_service_daemon_state()},
  });
}

inline void from_json(const nlohmann::json& json, settings_window_guidance_state& value) {
  value.set_current_alert(json.at("current_alert").get<settings_window_alert>());
  value.set_guidance_context(json.at("guidance_context").get<settings_window_guidance_context>());
  value.set_core_service_daemon_state(json.value("core_service_daemon_state",
                                                 core_service_daemon_state()));
}
} // namespace krbn
