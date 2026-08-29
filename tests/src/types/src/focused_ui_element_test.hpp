#include "test.hpp"
#include "types.hpp"
#include <boost/ut.hpp>
#include <optional>

void run_focused_ui_element_test() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "focused_ui_element"_test = [] {
    {
      auto json = nlohmann::json::object();
      auto value = json.get<krbn::focused_ui_element>();

      expect(value.get_window_title() == std::nullopt);
      expect(nlohmann::json(value) == json);
    }

    auto json = nlohmann::json::object({
        {"role", "AXTextField"},
        {"subrole", "AXSearchField"},
        {"title", "Search"},
        {"window_title", "Example Window"},
        {"window_position_x", 100.0},
        {"window_position_y", 200.0},
        {"window_size_width", 800.0},
        {"window_size_height", 600.0},
    });

    auto value = json.get<krbn::focused_ui_element>();
    expect(value.get_role() == "AXTextField");
    expect(value.get_subrole() == "AXSearchField");
    expect(value.get_title() == "Search");
    expect(value.get_window_title() == "Example Window");
    expect(value.get_window_position_x() == 100.0);
    expect(value.get_window_position_y() == 200.0);
    expect(value.get_window_size_width() == 800.0);
    expect(value.get_window_size_height() == 600.0);

    expect(nlohmann::json(value) == json);

    json_unmarshal_error_test<krbn::focused_ui_element>(
        nlohmann::json::object({
            {"window_title", nullptr},
        }),
        "`window_title` must be string, but is `null`");
  };
}
