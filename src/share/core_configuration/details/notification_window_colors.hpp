#pragma once

#include "../configuration_json_helper.hpp"
#include <algorithm>
#include <cctype>
#include <pqrs/json.hpp>
#include <ranges>
#include <string>

namespace krbn::core_configuration::details {
class notification_window_color_theme final {
public:
  notification_window_color_theme(const nlohmann::json& json,
                                  error_handling error_handling)
      : json_(json) {
    helper_values_.push_back_value<std::string>("background_color",
                                                background_color_,
                                                "system");
    helper_values_.push_back_value<std::string>("text_color",
                                                text_color_,
                                                "system");

    pqrs::json::requires_object(json, "json");
    helper_values_.update_value(json_, error_handling);

    set_background_color(background_color_);
    set_text_color(text_color_);
  }

  [[nodiscard]] nlohmann::json to_json() const {
    auto json = json_;
    helper_values_.update_json(json);
    return json;
  }

  [[nodiscard]] const std::string& get_background_color() const {
    return background_color_;
  }

  void set_background_color(const std::string& value) {
    background_color_ = normalize_color(value);
  }

  [[nodiscard]] const std::string& get_text_color() const {
    return text_color_;
  }

  void set_text_color(const std::string& value) {
    text_color_ = normalize_color(value);
  }

private:
  [[nodiscard]] static std::string normalize_color(const std::string& value) {
    if (!valid_color(value) || value == "system") {
      return "system";
    }

    auto result = value;
    std::ranges::transform(result | std::views::drop(1),
                           result.begin() + 1,
                           [](auto c) {
                             return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                           });
    return result;
  }

  [[nodiscard]] static bool valid_color(const std::string& value) {
    if (value == "system") {
      return true;
    }

    if (value.size() != 9 || value[0] != '#') {
      return false;
    }

    return std::ranges::all_of(value | std::views::drop(1), [](auto c) {
      return std::isxdigit(static_cast<unsigned char>(c));
    });
  }

  nlohmann::json json_;
  std::string background_color_;
  std::string text_color_;
  configuration_json_helper::helper_values helper_values_;
};

inline void to_json(nlohmann::json& json,
                    const notification_window_color_theme& value) {
  json = value.to_json();
}

class notification_window_colors final {
public:
  notification_window_colors(const nlohmann::json& json,
                             error_handling error_handling)
      : json_(json),
        light_(std::make_shared<notification_window_color_theme>(nlohmann::json::object(), error_handling)),
        dark_(std::make_shared<notification_window_color_theme>(nlohmann::json::object(), error_handling)) {
    helper_values_.push_back_object<notification_window_color_theme>("light", light_);
    helper_values_.push_back_object<notification_window_color_theme>("dark", dark_);

    pqrs::json::requires_object(json, "json");
    helper_values_.update_value(json_, error_handling);
  }

  [[nodiscard]] nlohmann::json to_json() const {
    auto json = json_;
    helper_values_.update_json(json);
    return json;
  }

  [[nodiscard]] notification_window_color_theme& get_light() const {
    return *light_;
  }

  [[nodiscard]] notification_window_color_theme& get_dark() const {
    return *dark_;
  }

private:
  nlohmann::json json_;
  pqrs::not_null_shared_ptr_t<notification_window_color_theme> light_;
  pqrs::not_null_shared_ptr_t<notification_window_color_theme> dark_;
  configuration_json_helper::helper_values helper_values_;
};

inline void to_json(nlohmann::json& json,
                    const notification_window_colors& value) {
  json = value.to_json();
}
} // namespace krbn::core_configuration::details
