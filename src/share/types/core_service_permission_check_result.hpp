#pragma once

#include <nlohmann/json.hpp>

namespace krbn {
class core_service_permission_check_result final {
public:
  core_service_permission_check_result()
      : iohid_listen_event_allowed_(false),
        accessibility_process_trusted_(false) {
  }

  bool get_iohid_listen_event_allowed() const {
    return iohid_listen_event_allowed_;
  }

  void set_iohid_listen_event_allowed(bool value) {
    iohid_listen_event_allowed_ = value;
  }

  bool get_accessibility_process_trusted() const {
    return accessibility_process_trusted_;
  }

  void set_accessibility_process_trusted(bool value) {
    accessibility_process_trusted_ = value;
  }

  bool required_permissions_granted() const {
    return iohid_listen_event_allowed_ &&
           accessibility_process_trusted_;
  }

  bool operator==(const core_service_permission_check_result&) const = default;

private:
  bool iohid_listen_event_allowed_;
  bool accessibility_process_trusted_;
};

inline void to_json(nlohmann::json& json, const core_service_permission_check_result& value) {
  json = nlohmann::json::object({
      {"iohid_listen_event_allowed", value.get_iohid_listen_event_allowed()},
      {"accessibility_process_trusted", value.get_accessibility_process_trusted()},
  });
}

inline void from_json(const nlohmann::json& json, core_service_permission_check_result& value) {
  value.set_iohid_listen_event_allowed(json.at("iohid_listen_event_allowed").get<bool>());
  value.set_accessibility_process_trusted(json.at("accessibility_process_trusted").get<bool>());
}
} // namespace krbn
