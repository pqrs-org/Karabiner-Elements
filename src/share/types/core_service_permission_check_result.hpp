#pragma once

#include <nlohmann/json.hpp>

namespace krbn {
class core_service_permission_check_result final {
public:
  core_service_permission_check_result()
      : input_monitoring_granted_(false),
        accessibility_process_trusted_(false) {
  }

  bool get_input_monitoring_granted() const {
    return input_monitoring_granted_;
  }

  void set_input_monitoring_granted(bool value) {
    input_monitoring_granted_ = value;
  }

  bool get_accessibility_process_trusted() const {
    return accessibility_process_trusted_;
  }

  void set_accessibility_process_trusted(bool value) {
    accessibility_process_trusted_ = value;
  }

  bool required_permissions_granted() const {
    return input_monitoring_granted_ &&
           accessibility_process_trusted_;
  }

  bool operator==(const core_service_permission_check_result&) const = default;

private:
  bool input_monitoring_granted_;
  bool accessibility_process_trusted_;
};

inline void to_json(nlohmann::json& json, const core_service_permission_check_result& value) {
  json = nlohmann::json::object({
      {"input_monitoring_granted", value.get_input_monitoring_granted()},
      {"accessibility_process_trusted", value.get_accessibility_process_trusted()},
  });
}

inline void from_json(const nlohmann::json& json, core_service_permission_check_result& value) {
  value.set_input_monitoring_granted(json.at("input_monitoring_granted").get<bool>());
  value.set_accessibility_process_trusted(json.at("accessibility_process_trusted").get<bool>());
}
} // namespace krbn
