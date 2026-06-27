#pragma once

#include "../configuration_json_helper.hpp"
#include <pqrs/json.hpp>

namespace krbn::core_configuration::details {
class global_configuration final {
public:
  global_configuration(const global_configuration&) = delete;

  global_configuration(const nlohmann::json& json,
                       error_handling error_handling)
      : json_(json) {
    helper_values_.push_back_value<bool>("check_for_updates",
                                         check_for_updates_,
                                         true);

    helper_values_.push_back_value<bool>("show_in_menu_bar",
                                         show_in_menu_bar_,
                                         true);

    helper_values_.push_back_value<bool>("show_profile_name_in_menu_bar",
                                         show_profile_name_in_menu_bar_,
                                         false);

    helper_values_.push_back_value<bool>("show_additional_menu_items",
                                         show_additional_menu_items_,
                                         false);

    helper_values_.push_back_value<bool>("enable_notification_window",
                                         enable_notification_window_,
                                         true);

    helper_values_.push_back_value<bool>("ask_for_confirmation_before_quitting",
                                         ask_for_confirmation_before_quitting_,
                                         true);

    helper_values_.push_back_value<bool>("unsafe_ui",
                                         unsafe_ui_,
                                         false);

    helper_values_.push_back_value<bool>("filter_useless_events_from_specific_devices",
                                         filter_useless_events_from_specific_devices_,
                                         true);

    helper_values_.push_back_value<bool>("reorder_same_timestamp_input_events_to_prioritize_modifiers",
                                         reorder_same_timestamp_input_events_to_prioritize_modifiers_,
                                         true);

    helper_values_.push_back_value<bool>("enable_cgeventtap_fallback",
                                         enable_cgeventtap_fallback_,
                                         false);

    pqrs::json::requires_object(json, "json");

    if (!json_.contains("check_for_updates") &&
        json_.contains("check_for_updates_on_startup")) {
      json_["check_for_updates"] = json_["check_for_updates_on_startup"];
    }
    json_.erase("check_for_updates_on_startup");

    helper_values_.update_value(json_, error_handling);
  }

  nlohmann::json to_json() const {
    auto j = json_;

    helper_values_.update_json(j);

    return j;
  }

  [[nodiscard]] const bool& get_check_for_updates() const {
    return check_for_updates_;
  }
  void set_check_for_updates(bool value) {
    check_for_updates_ = value;
  }

  [[nodiscard]] const bool& get_show_in_menu_bar() const {
    return show_in_menu_bar_;
  }
  void set_show_in_menu_bar(bool value) {
    show_in_menu_bar_ = value;
  }

  [[nodiscard]] const bool& get_show_profile_name_in_menu_bar() const {
    return show_profile_name_in_menu_bar_;
  }
  void set_show_profile_name_in_menu_bar(bool value) {
    show_profile_name_in_menu_bar_ = value;
  }

  [[nodiscard]] const bool& get_show_additional_menu_items() const {
    return show_additional_menu_items_;
  }
  void set_show_additional_menu_items(bool value) {
    show_additional_menu_items_ = value;
  }

  [[nodiscard]] const bool& get_enable_notification_window() const {
    return enable_notification_window_;
  }
  void set_enable_notification_window(bool value) {
    enable_notification_window_ = value;
  }

  [[nodiscard]] const bool& get_ask_for_confirmation_before_quitting() const {
    return ask_for_confirmation_before_quitting_;
  }
  void set_ask_for_confirmation_before_quitting(bool value) {
    ask_for_confirmation_before_quitting_ = value;
  }

  [[nodiscard]] const bool& get_unsafe_ui() const {
    return unsafe_ui_;
  }
  void set_unsafe_ui(bool value) {
    unsafe_ui_ = value;
  }

  [[nodiscard]] const bool& get_filter_useless_events_from_specific_devices() const {
    return filter_useless_events_from_specific_devices_;
  }
  void set_filter_useless_events_from_specific_devices(bool value) {
    filter_useless_events_from_specific_devices_ = value;
  }

  [[nodiscard]] const bool& get_reorder_same_timestamp_input_events_to_prioritize_modifiers() const {
    return reorder_same_timestamp_input_events_to_prioritize_modifiers_;
  }
  void set_reorder_same_timestamp_input_events_to_prioritize_modifiers(bool value) {
    reorder_same_timestamp_input_events_to_prioritize_modifiers_ = value;
  }

  [[nodiscard]] const bool& get_enable_cgeventtap_fallback() const {
    return enable_cgeventtap_fallback_;
  }
  void set_enable_cgeventtap_fallback(bool value) {
    enable_cgeventtap_fallback_ = value;
  }

private:
  nlohmann::json json_;
  bool check_for_updates_;
  bool show_in_menu_bar_;
  bool show_profile_name_in_menu_bar_;
  bool show_additional_menu_items_;
  bool enable_notification_window_;
  bool ask_for_confirmation_before_quitting_;
  bool unsafe_ui_;
  bool filter_useless_events_from_specific_devices_;
  bool reorder_same_timestamp_input_events_to_prioritize_modifiers_;
  bool enable_cgeventtap_fallback_;
  configuration_json_helper::helper_values helper_values_;
};

inline void to_json(nlohmann::json& json, const global_configuration& global_configuration) {
  json = global_configuration.to_json();
}
} // namespace krbn::core_configuration::details
