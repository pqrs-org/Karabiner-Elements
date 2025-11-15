#include "types.hpp"
#include <boost/ut.hpp>

void run_software_function_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;
  using namespace std::literals;

  "software_function"_test = [] {
    //
    // cg_event_double_click
    //

    {
      auto json = nlohmann::json::object({
          {"button", 3},
      });

      auto value = json.get<krbn::software_function_details::cg_event_double_click>();
      expect(value.get_button() == 3);

      expect(nlohmann::json(value) == json);
    }

    //
    // iokit_power_management_sleep_system
    //

    {
      auto json = nlohmann::json::object({
          {"delay_milliseconds", 100},
      });

      auto value = json.get<krbn::software_function_details::iokit_power_management_sleep_system>();
      expect(value.get_delay_milliseconds() == std::chrono::milliseconds(100));

      expect(nlohmann::json(value) == json);
    }

    //
    // open_application
    //

    {
      auto json = nlohmann::json::object({
          {"bundle_identifier", "com.apple.Terminal"},
      });

      auto value = json.get<krbn::software_function_details::open_application>();
      expect("com.apple.Terminal" == value.get_bundle_identifier());

      expect(nlohmann::json(value) == json);
    }

    {
      auto json = nlohmann::json::object({
          {"file_path", "/System/Applications/Utilities/Terminal.app/Contents/MacOS/Terminal"},
      });

      auto value = json.get<krbn::software_function_details::open_application>();
      expect("/System/Applications/Utilities/Terminal.app/Contents/MacOS/Terminal" == value.get_file_path());

      expect(nlohmann::json(value) == json);
    }

    {
      auto json = R"(

{
  "frontmost_application_history_index": 1,
  "frontmost_application_history_exclusion_bundle_identifiers": [
    "^com\\.apple\\.Terminal$",
    "^com\\.apple\\.Safari$"
  ],
  "frontmost_application_history_exclusion_file_paths": [
    "^/System/Applications/App Store\\.app/Contents/MacOS/App Store"
  ]
}

)"_json;
      auto value = json.get<krbn::software_function_details::open_application>();
      expect(1 == value.get_frontmost_application_history_index());
      auto bundle_identifiers = value.get_frontmost_application_history_exclusion_bundle_identifiers();
      expect(2 == bundle_identifiers.size());
      expect("^com\\.apple\\.Terminal$"sv == bundle_identifiers[0].get_string());
      expect("^com\\.apple\\.Safari$"sv == bundle_identifiers[1].get_string());
      auto file_paths = value.get_frontmost_application_history_exclusion_file_paths();
      expect(1 == file_paths.size());
      expect("^/System/Applications/App Store\\.app/Contents/MacOS/App Store"sv == file_paths[0].get_string());

      expect(nlohmann::json(value) == json);
    }

    //
    // set_mouse_cursor_position
    //

    // position_value
    {
      auto json = nlohmann::json(20);
      auto value = json.get<krbn::software_function_details::set_mouse_cursor_position::position_value>();
      expect(value.get_value() == 20);
      expect(value.get_type() == krbn::software_function_details::set_mouse_cursor_position::position_value::type::point);
      expect(value.point_value(1200) == 20);
    }
    {
      auto json = nlohmann::json("50%");
      auto value = json.get<krbn::software_function_details::set_mouse_cursor_position::position_value>();
      expect(value.get_value() == 50);
      expect(value.get_type() == krbn::software_function_details::set_mouse_cursor_position::position_value::type::percent);
      expect(value.point_value(1200) == 600);
    }
    {
      auto json = nlohmann::json("20");
      auto value = json.get<krbn::software_function_details::set_mouse_cursor_position::position_value>();
      expect(value.get_value() == 20);
      expect(value.get_type() == krbn::software_function_details::set_mouse_cursor_position::position_value::type::point);
      expect(value.point_value(1200) == 20);
    }

    {
      auto json = nlohmann::json::object({
          {"x", 100},
          {"y", "20%"},
          {"screen", 1},
      });

      auto value = json.get<krbn::software_function_details::set_mouse_cursor_position>();
      expect(value.get_x().get_value() == 100);
      expect(value.get_x().get_type() == krbn::software_function_details::set_mouse_cursor_position::position_value::type::point);
      expect(value.get_y().get_value() == 20);
      expect(value.get_y().get_type() == krbn::software_function_details::set_mouse_cursor_position::position_value::type::percent);
      expect(value.get_screen() == 1);

      expect(nlohmann::json(value) == json);
    }

    //
    // software_function
    //

    {
      auto json = nlohmann::json::object({
          {"set_mouse_cursor_position",
           nlohmann::json::object({
               {"x", 100},
               {"y", 200},
               {"screen", 1},
           })},
      });

      auto value = json.get<krbn::software_function>();
      expect(value.get_if<krbn::software_function_details::cg_event_double_click>() == nullptr);
      expect(value.get_if<krbn::software_function_details::set_mouse_cursor_position>()->get_x().get_value() == 100);

      expect(nlohmann::json(value) == json);
    }
  };
}
