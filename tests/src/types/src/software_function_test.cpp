#include <catch2/catch.hpp>

#include "types.hpp"

TEST_CASE("software_function") {
  //
  // cg_event_double_click
  //

  {
    auto json = nlohmann::json::object({
        {"button", 3},
    });

    auto value = json.get<krbn::software_function_details::cg_event_double_click>();
    REQUIRE(value.get_button() == 3);

    REQUIRE(nlohmann::json(value) == json);
  }

  //
  // set_mouse_cursor_position
  //

  // position_value
  {
    auto json = nlohmann::json(20);
    auto value = json.get<krbn::software_function_details::set_mouse_cursor_position::position_value>();
    REQUIRE(value.get_value() == 20);
    REQUIRE(value.get_type() == krbn::software_function_details::set_mouse_cursor_position::position_value::type::point);
    REQUIRE(value.point_value(1200) == 20);
  }
  {
    auto json = nlohmann::json("50%");
    auto value = json.get<krbn::software_function_details::set_mouse_cursor_position::position_value>();
    REQUIRE(value.get_value() == 50);
    REQUIRE(value.get_type() == krbn::software_function_details::set_mouse_cursor_position::position_value::type::percent);
    REQUIRE(value.point_value(1200) == 600);
  }
  {
    auto json = nlohmann::json("20");
    auto value = json.get<krbn::software_function_details::set_mouse_cursor_position::position_value>();
    REQUIRE(value.get_value() == 20);
    REQUIRE(value.get_type() == krbn::software_function_details::set_mouse_cursor_position::position_value::type::point);
    REQUIRE(value.point_value(1200) == 20);
  }

  {
    auto json = nlohmann::json::object({
        {"x", 100},
        {"y", "20%"},
        {"screen", 1},
    });

    auto value = json.get<krbn::software_function_details::set_mouse_cursor_position>();
    REQUIRE(value.get_x().get_value() == 100);
    REQUIRE(value.get_x().get_type() == krbn::software_function_details::set_mouse_cursor_position::position_value::type::point);
    REQUIRE(value.get_y().get_value() == 20);
    REQUIRE(value.get_y().get_type() == krbn::software_function_details::set_mouse_cursor_position::position_value::type::percent);
    REQUIRE(value.get_screen() == 1);

    REQUIRE(nlohmann::json(value) == json);
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
    REQUIRE(value.get_if<krbn::software_function_details::cg_event_double_click>() == nullptr);
    REQUIRE(value.get_if<krbn::software_function_details::set_mouse_cursor_position>()->get_x().get_value() == 100);

    REQUIRE(nlohmann::json(value) == json);
  }
}
