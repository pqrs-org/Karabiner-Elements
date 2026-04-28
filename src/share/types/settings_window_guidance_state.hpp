#pragma once

#include "types/core_service_daemon_state.hpp"
#include <nlohmann/json.hpp>
#include <optional>

namespace krbn {
enum class settings_window_guidance_setup : uint8_t {
  none,
  services,
  accessibility,
  input_monitoring,
  driver_extension,
};

NLOHMANN_JSON_SERIALIZE_ENUM(
    settings_window_guidance_setup,
    {
        {settings_window_guidance_setup::none, "none"},
        {settings_window_guidance_setup::services, "services"},
        {settings_window_guidance_setup::accessibility, "accessibility"},
        {settings_window_guidance_setup::input_monitoring, "input_monitoring"},
        {settings_window_guidance_setup::driver_extension, "driver_extension"},
    });

enum class settings_window_guidance_alert : uint8_t {
  none,
  doctor,
  settings,
  services_not_running,
  virtual_hid_device_service_client_not_connected,
  driver_version_mismatched,
  driver_not_connected,
};

NLOHMANN_JSON_SERIALIZE_ENUM(
    settings_window_guidance_alert,
    {
        {settings_window_guidance_alert::none, "none"},
        {settings_window_guidance_alert::doctor, "doctor"},
        {settings_window_guidance_alert::settings, "settings"},
        {settings_window_guidance_alert::services_not_running, "services_not_running"},
        {settings_window_guidance_alert::virtual_hid_device_service_client_not_connected, "virtual_hid_device_service_client_not_connected"},
        {settings_window_guidance_alert::driver_version_mismatched, "driver_version_mismatched"},
        {settings_window_guidance_alert::driver_not_connected, "driver_not_connected"},
    });

class settings_window_guidance_context final {
public:
  settings_window_guidance_context() {
  }

  const std::optional<bool>& get_core_daemons_enabled() const {
    return core_daemons_enabled_;
  }

  void set_core_daemons_enabled(const std::optional<bool>& value) {
    core_daemons_enabled_ = value;
  }

  const std::optional<bool>& get_core_agents_enabled() const {
    return core_agents_enabled_;
  }

  void set_core_agents_enabled(const std::optional<bool>& value) {
    core_agents_enabled_ = value;
  }

  const std::optional<bool>& get_core_daemons_running() const {
    return core_daemons_running_;
  }

  void set_core_daemons_running(const std::optional<bool>& value) {
    core_daemons_running_ = value;
  }

  const std::optional<bool>& get_core_agents_running() const {
    return core_agents_running_;
  }

  void set_core_agents_running(const std::optional<bool>& value) {
    core_agents_running_ = value;
  }

  std::optional<bool> services_enabled() const {
    if (core_daemons_enabled_ == std::optional<bool>(true) &&
        core_agents_enabled_ == std::optional<bool>(true)) {
      return true;
    }

    if (core_daemons_enabled_ == std::optional<bool>(false) ||
        core_agents_enabled_ == std::optional<bool>(false)) {
      return false;
    }

    return std::nullopt;
  }

  std::optional<bool> services_running() const {
    if (core_daemons_running_ == std::optional<bool>(true) &&
        core_agents_running_ == std::optional<bool>(true)) {
      return true;
    }

    if (core_daemons_running_ == std::optional<bool>(false) ||
        core_agents_running_ == std::optional<bool>(false)) {
      return false;
    }

    return std::nullopt;
  }

private:
  std::optional<bool> core_daemons_enabled_;
  std::optional<bool> core_agents_enabled_;
  std::optional<bool> core_daemons_running_;
  std::optional<bool> core_agents_running_;
};

inline void to_json(nlohmann::json& json, const settings_window_guidance_context& value) {
  json = nlohmann::json::object({
      {"core_daemons_enabled", value.get_core_daemons_enabled()},
      {"core_agents_enabled", value.get_core_agents_enabled()},
      {"core_daemons_running", value.get_core_daemons_running()},
      {"core_agents_running", value.get_core_agents_running()},
      {"services_enabled", value.services_enabled()},
      {"services_running", value.services_running()},
  });
}

inline void from_json(const nlohmann::json& json, settings_window_guidance_context& value) {
  const auto get_optional_bool = [](const nlohmann::json& json, const char* key) -> std::optional<bool> {
    if (json.at(key).is_null()) {
      return std::nullopt;
    }
    return json.at(key).get<bool>();
  };

  value.set_core_daemons_enabled(get_optional_bool(json,
                                                   "core_daemons_enabled"));
  value.set_core_agents_enabled(get_optional_bool(json,
                                                  "core_agents_enabled"));
  value.set_core_daemons_running(get_optional_bool(json,
                                                   "core_daemons_running"));
  value.set_core_agents_running(get_optional_bool(json,
                                                  "core_agents_running"));
}

class settings_window_guidance_state final {
public:
  settings_window_guidance_state()
      : current_setup_(settings_window_guidance_setup::none),
        current_alert_(settings_window_guidance_alert::none) {
  }

  settings_window_guidance_setup get_current_setup() const {
    return current_setup_;
  }

  void set_current_setup(settings_window_guidance_setup value) {
    current_setup_ = value;
  }

  settings_window_guidance_alert get_current_alert() const {
    return current_alert_;
  }

  void set_current_alert(settings_window_guidance_alert value) {
    current_alert_ = value;
  }

  const settings_window_guidance_context& get_guidance_context() const {
    return guidance_context_;
  }

  void set_guidance_context(const settings_window_guidance_context& value) {
    guidance_context_ = value;
  }

  const core_service_daemon_state& get_core_service_daemon_state() const {
    return core_service_deamon_state_;
  }

  void set_core_service_daemon_state(const core_service_daemon_state& value) {
    core_service_deamon_state_ = value;
  }

private:
  settings_window_guidance_setup current_setup_;
  settings_window_guidance_alert current_alert_;
  settings_window_guidance_context guidance_context_;
  core_service_daemon_state core_service_deamon_state_;
};

inline void to_json(nlohmann::json& json, const settings_window_guidance_state& value) {
  json = nlohmann::json::object({
      {"current_setup", value.get_current_setup()},
      {"current_alert", value.get_current_alert()},
      {"guidance_context", value.get_guidance_context()},
      {"core_service_daemon_state", value.get_core_service_daemon_state()},
  });
}

inline void from_json(const nlohmann::json& json, settings_window_guidance_state& value) {
  value.set_current_setup(json.at("current_setup").get<settings_window_guidance_setup>());
  value.set_current_alert(json.at("current_alert").get<settings_window_guidance_alert>());
  value.set_guidance_context(json.at("guidance_context").get<settings_window_guidance_context>());
  value.set_core_service_daemon_state(json.value("core_service_daemon_state",
                                                 core_service_daemon_state()));
}
} // namespace krbn
