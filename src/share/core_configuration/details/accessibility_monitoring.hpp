#pragma once

#include "../configuration_json_helper.hpp"
#include <nlohmann/json.hpp>

namespace krbn::core_configuration::details {
class accessibility_monitoring final {
public:
  accessibility_monitoring(const accessibility_monitoring&) = delete;

  accessibility_monitoring(const nlohmann::json& json,
                           error_handling error_handling)
      : json_(json) {
    helper_values_.push_back_value<bool>("frontmost_application",
                                         frontmost_application_,
                                         true);
    helper_values_.push_back_value<bool>("focused_ui_element",
                                         focused_ui_element_,
                                         true);
    helper_values_.push_back_value<bool>("focused_window",
                                         focused_window_,
                                         true);

    pqrs::json::requires_object(json, "json");
    helper_values_.update_value(json_, error_handling);
  }

  [[nodiscard]] nlohmann::json to_json() const {
    auto json = json_;
    helper_values_.update_json(json);
    return json;
  }

  [[nodiscard]] bool get_frontmost_application() const { return frontmost_application_; }
  void set_frontmost_application(bool value) { frontmost_application_ = value; }

  [[nodiscard]] bool get_focused_ui_element() const { return focused_ui_element_; }
  void set_focused_ui_element(bool value) { focused_ui_element_ = value; }

  [[nodiscard]] bool get_focused_window() const { return focused_window_; }
  void set_focused_window(bool value) { focused_window_ = value; }

private:
  nlohmann::json json_;
  bool frontmost_application_;
  bool focused_ui_element_;
  bool focused_window_;
  configuration_json_helper::helper_values helper_values_;
};

inline void to_json(nlohmann::json& json, const accessibility_monitoring& value) {
  json = value.to_json();
}
} // namespace krbn::core_configuration::details
