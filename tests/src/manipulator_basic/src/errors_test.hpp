#include "../../share/json_helper.hpp"
#include "manipulator/manipulators/basic/basic.hpp"
#include "manipulator/types.hpp"
#include <boost/ut.hpp>

namespace {
using namespace boost::ut;
using namespace boost::ut::literals;

void handle_json(const nlohmann::json& json) {
  auto c = json.at("class").get<std::string>();
  if (c == "basic") {
    krbn::core_configuration::details::complex_modifications_parameters parameters;
    krbn::manipulator::manipulators::basic::basic(json.at("input"), parameters);
  } else if (c == "from_event_definition") {
    json.at("input").get<krbn::manipulator::manipulators::basic::from_event_definition>();
  } else if (c == "simultaneous_options::key_order") {
    json.at("input").get<krbn::manipulator::manipulators::basic::simultaneous_options::key_order>();
  } else if (c == "simultaneous_options::key_up_when") {
    json.at("input").get<krbn::manipulator::manipulators::basic::simultaneous_options::key_up_when>();
  } else if (c == "simultaneous_options") {
    json.at("input").get<krbn::manipulator::manipulators::basic::simultaneous_options>();
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
          expect(e.at("error").get<std::string>() == ex.what());
        } catch (...) {
          expect(false);
        }
      }
    }
  };
}
