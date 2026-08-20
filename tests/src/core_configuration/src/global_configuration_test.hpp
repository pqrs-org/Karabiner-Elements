#include "core_configuration/core_configuration.hpp"
#include <boost/ut.hpp>

void run_global_configuration_test() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "global_configuration"_test = [] {
    // empty json
    {
      auto json = nlohmann::json::object();
      krbn::core_configuration::details::global_configuration global_configuration(json,
                                                                                   krbn::core_configuration::error_handling::strict);
      expect(global_configuration.get_check_for_updates() == true);
      expect(global_configuration.get_show_in_menu_bar() == true);
      expect(global_configuration.get_show_profile_name_in_menu_bar() == false);
      expect(global_configuration.get_show_additional_menu_items() == false);
      expect(global_configuration.get_enable_notification_window() == true);
      expect(global_configuration.get_notification_window_position() == "bottom_right");
      expect(global_configuration.get_notification_window_respect_screen_visible_frame() == true);
      expect(global_configuration.get_notification_window_show_icon() == true);
      expect(global_configuration.get_notification_window_font_size() == 13);
      expect(global_configuration.get_notification_window_colors().get_light().get_background_color() == "system");
      expect(global_configuration.get_notification_window_colors().get_light().get_text_color() == "system");
      expect(global_configuration.get_notification_window_colors().get_dark().get_background_color() == "system");
      expect(global_configuration.get_notification_window_colors().get_dark().get_text_color() == "system");
      expect(global_configuration.get_unsafe_ui() == false);
      expect(global_configuration.get_filter_useless_events_from_specific_devices() == true);
      expect(global_configuration.get_reorder_same_timestamp_input_events_to_prioritize_modifiers() == true);
      expect(global_configuration.get_enable_cgeventtap_fallback() == false);
      expect(global_configuration.get_delay_milliseconds_before_sleep_shortcut() == 500);
    }

    // load values from json
    {
      nlohmann::json json{
          {"check_for_updates", false},
          {"show_in_menu_bar", false},
          {"show_profile_name_in_menu_bar", true},
          {"show_additional_menu_items", true},
          {"enable_notification_window", false},
          {"notification_window_position", "top_left"},
          {"notification_window_respect_screen_visible_frame", false},
          {"notification_window_show_icon", false},
          {"notification_window_font_size", 24},
          {"notification_window_colors",
           {
               {"light",
                {
                    {"background_color", "#112233ff"},
                    {"text_color", "#44556677"},
                }},
               {"dark",
                {
                    {"background_color", "#89abcdef"},
                    {"text_color", "#ABCDEFFF"},
                }},
           }},
          {"unsafe_ui", true},
          {"filter_useless_events_from_specific_devices", false},
          {"reorder_same_timestamp_input_events_to_prioritize_modifiers", false},
          {"enable_cgeventtap_fallback", true},
          {"delay_milliseconds_before_sleep_shortcut", 250},
      };
      krbn::core_configuration::details::global_configuration global_configuration(json,
                                                                                   krbn::core_configuration::error_handling::strict);
      expect(global_configuration.get_check_for_updates() == false);
      expect(global_configuration.get_show_in_menu_bar() == false);
      expect(global_configuration.get_show_profile_name_in_menu_bar() == true);
      expect(global_configuration.get_show_additional_menu_items() == true);
      expect(global_configuration.get_enable_notification_window() == false);
      expect(global_configuration.get_notification_window_position() == "top_left");
      expect(global_configuration.get_notification_window_respect_screen_visible_frame() == false);
      expect(global_configuration.get_notification_window_show_icon() == false);
      expect(global_configuration.get_notification_window_font_size() == 24);
      expect(global_configuration.get_notification_window_colors().get_light().get_background_color() == "#112233ff");
      expect(global_configuration.get_notification_window_colors().get_light().get_text_color() == "#44556677");
      expect(global_configuration.get_notification_window_colors().get_dark().get_background_color() == "#89abcdef");
      expect(global_configuration.get_notification_window_colors().get_dark().get_text_color() == "#abcdefff");
      expect(global_configuration.to_json()["notification_window_colors"]["dark"]["text_color"] == "#abcdefff");
      expect(global_configuration.get_unsafe_ui() == true);
      expect(global_configuration.get_filter_useless_events_from_specific_devices() == false);
      expect(global_configuration.get_reorder_same_timestamp_input_events_to_prioritize_modifiers() == false);
      expect(global_configuration.get_enable_cgeventtap_fallback() == true);
      expect(global_configuration.get_delay_milliseconds_before_sleep_shortcut() == 250);

      //
      // Set default values
      //

      global_configuration.set_check_for_updates(true);
      global_configuration.set_show_in_menu_bar(true);
      global_configuration.set_show_profile_name_in_menu_bar(false);
      global_configuration.set_show_additional_menu_items(false);
      global_configuration.set_enable_notification_window(true);
      global_configuration.set_notification_window_position("bottom_right");
      global_configuration.set_notification_window_respect_screen_visible_frame(true);
      global_configuration.set_notification_window_show_icon(true);
      global_configuration.set_notification_window_font_size(13);
      global_configuration.get_notification_window_colors().get_light().set_background_color("system");
      global_configuration.get_notification_window_colors().get_light().set_text_color("system");
      global_configuration.get_notification_window_colors().get_dark().set_background_color("system");
      global_configuration.get_notification_window_colors().get_dark().set_text_color("system");
      global_configuration.set_unsafe_ui(false);
      global_configuration.set_filter_useless_events_from_specific_devices(true);
      global_configuration.set_reorder_same_timestamp_input_events_to_prioritize_modifiers(true);
      global_configuration.set_enable_cgeventtap_fallback(false);
      global_configuration.set_delay_milliseconds_before_sleep_shortcut(500);
      nlohmann::json j(global_configuration);
      expect(j.empty());
    }

    // invalid values in json
    {
      nlohmann::json json{
          {"check_for_updates", nlohmann::json::array()},
          {"show_in_menu_bar", 0},
          {"show_profile_name_in_menu_bar", nlohmann::json::object()},
          {"show_additional_menu_items", nlohmann::json::object()},
          {"enable_notification_window", nlohmann::json::object()},
          {"notification_window_position", nlohmann::json::object()},
          {"notification_window_respect_screen_visible_frame", nlohmann::json::object()},
          {"notification_window_show_icon", nlohmann::json::object()},
          {"notification_window_font_size", nlohmann::json::object()},
          {"notification_window_colors", nlohmann::json::object({{"light", nlohmann::json::object({{"background_color", nlohmann::json::array()}})}})},
          {"unsafe_ui", nlohmann::json::object()},
          {"filter_useless_events_from_specific_devices", nlohmann::json::object()},
          {"reorder_same_timestamp_input_events_to_prioritize_modifiers", nlohmann::json::object()},
          {"enable_cgeventtap_fallback", nlohmann::json::object()},
          {"delay_milliseconds_before_sleep_shortcut", nlohmann::json::object()},
      };
      krbn::core_configuration::details::global_configuration global_configuration(json,
                                                                                   krbn::core_configuration::error_handling::loose);
      expect(global_configuration.get_check_for_updates() == true);
      expect(global_configuration.get_show_in_menu_bar() == true);
      expect(global_configuration.get_show_profile_name_in_menu_bar() == false);
      expect(global_configuration.get_show_additional_menu_items() == false);
      expect(global_configuration.get_enable_notification_window() == true);
      expect(global_configuration.get_notification_window_position() == "bottom_right");
      expect(global_configuration.get_notification_window_respect_screen_visible_frame() == true);
      expect(global_configuration.get_notification_window_show_icon() == true);
      expect(global_configuration.get_notification_window_font_size() == 13);
      expect(global_configuration.get_notification_window_colors().get_light().get_background_color() == "system");
      expect(global_configuration.get_unsafe_ui() == false);
      expect(global_configuration.get_filter_useless_events_from_specific_devices() == true);
      expect(global_configuration.get_reorder_same_timestamp_input_events_to_prioritize_modifiers() == true);
      expect(global_configuration.get_enable_cgeventtap_fallback() == false);
      expect(global_configuration.get_delay_milliseconds_before_sleep_shortcut() == 500);
    }

    // invalid notification_window_position in json
    {
      nlohmann::json json{
          {"notification_window_position", "unknown"},
      };
      krbn::core_configuration::details::global_configuration global_configuration(json,
                                                                                   krbn::core_configuration::error_handling::strict);
      expect(global_configuration.get_notification_window_position() == "bottom_right");
    }

    // clamp notification_window_font_size
    {
      krbn::core_configuration::details::global_configuration global_configuration(
          nlohmann::json({{"notification_window_font_size", 7}}),
          krbn::core_configuration::error_handling::strict);
      expect(global_configuration.get_notification_window_font_size() == 8);

      global_configuration.set_notification_window_font_size(65);
      expect(global_configuration.get_notification_window_font_size() == 64);
    }

    // clamp delay_milliseconds_before_sleep_shortcut
    {
      krbn::core_configuration::details::global_configuration global_configuration(
          nlohmann::json({{"delay_milliseconds_before_sleep_shortcut", -1}}),
          krbn::core_configuration::error_handling::strict);
      expect(global_configuration.get_delay_milliseconds_before_sleep_shortcut() == 0);

      global_configuration.set_delay_milliseconds_before_sleep_shortcut(10001);
      expect(global_configuration.get_delay_milliseconds_before_sleep_shortcut() == 10000);
    }

    // invalid notification window colors in json
    {
      nlohmann::json json{
          {"notification_window_colors",
           {
               {"light",
                {
                    {"background_color", "red"},
                    {"text_color", "#123456"},
                }},
               {"dark",
                {
                    {"background_color", "#123456789"},
                    {"text_color", "#gggggg"},
                }},
           }},
      };
      krbn::core_configuration::details::global_configuration global_configuration(json,
                                                                                   krbn::core_configuration::error_handling::strict);
      expect(global_configuration.get_notification_window_colors().get_light().get_background_color() == "system");
      expect(global_configuration.get_notification_window_colors().get_light().get_text_color() == "system");
      expect(global_configuration.get_notification_window_colors().get_dark().get_background_color() == "system");
      expect(global_configuration.get_notification_window_colors().get_dark().get_text_color() == "system");
    }

    // migrate from check_for_updates_on_startup
    {
      nlohmann::json json{
          {"check_for_updates_on_startup", false},
      };
      krbn::core_configuration::details::global_configuration global_configuration(json,
                                                                                   krbn::core_configuration::error_handling::strict);
      expect(global_configuration.get_check_for_updates() == false);

      nlohmann::json expected({
          {"check_for_updates", false},
      });
      expect(global_configuration.to_json() == expected);
    }

    // remove ask_for_confirmation_before_quitting
    {
      nlohmann::json json{
          {"ask_for_confirmation_before_quitting", false},
      };
      krbn::core_configuration::details::global_configuration global_configuration(json,
                                                                                   krbn::core_configuration::error_handling::strict);
      expect(global_configuration.to_json().empty());
    }
  };
}
