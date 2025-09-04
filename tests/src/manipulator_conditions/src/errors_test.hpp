#include "../../share/json_helper.hpp"
#include "manipulator/manipulator_factory.hpp"
#include <boost/ut.hpp>

namespace {
using namespace boost::ut;
using namespace boost::ut::literals;

void handle_json(const nlohmann::json& json) {
  auto c = json.at("class").get<std::string>();
  if (c == "device") {
    krbn::manipulator::conditions::device(json.at("input"));
  } else if (c == "event_changed") {
    krbn::manipulator::conditions::event_changed(json.at("input"));
  } else if (c == "expression") {
    krbn::manipulator::conditions::expression(json.at("input"));
  } else if (c == "frontmost_application") {
    krbn::manipulator::conditions::frontmost_application(json.at("input"));
  } else if (c == "input_source") {
    krbn::manipulator::conditions::input_source(json.at("input"));
  } else if (c == "keyboard_type") {
    krbn::manipulator::conditions::keyboard_type(json.at("input"));
  } else if (c == "variable") {
    krbn::manipulator::conditions::variable(json.at("input"));
  } else {
    expect(false);
  }
}
} // namespace

void run_errors_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "errors"_test = [] {
    namespace basic = krbn::manipulator::manipulators::basic;

    auto json = krbn::unit_testing::json_helper::load_jsonc("json/errors.jsonc");
    for (const auto& j : json) {
      auto error_json = krbn::unit_testing::json_helper::load_jsonc("json/" + j.get<std::string>());
      for (const auto& e : error_json) {
        try {
          handle_json(e);
          expect(false);
        } catch (pqrs::json::unmarshal_error& ex) {
          expect(std::string_view(e.at("error").get<std::string>()) == ex.what());
        } catch (...) {
          expect(false);
        }
      }
    }
  };
}
