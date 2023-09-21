#include "../../share/json_helper.hpp"
#include "manipulator/manipulators/mouse_basic/mouse_basic.hpp"
#include "manipulator/types.hpp"
#include <boost/ut.hpp>

namespace {
using namespace boost::ut;
using namespace boost::ut::literals;

void handle_json(const nlohmann::json& json) {
  auto c = json.at("class").get<std::string>();
  if (c == "mouse_basic") {
    krbn::core_configuration::details::complex_modifications_parameters parameters;
    krbn::manipulator::manipulators::mouse_basic::mouse_basic(json.at("input"), parameters);
  } else {
    expect(false);
  }
}
} // namespace

void run_errors_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "errors"_test = [] {
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
