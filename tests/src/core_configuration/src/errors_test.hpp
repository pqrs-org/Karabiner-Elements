#include "../../share/json_helper.hpp"
#include "manipulator/manipulator_factory.hpp"
#include <boost/ut.hpp>

namespace {
using namespace boost::ut;
using namespace boost::ut::literals;

void handle_json(const nlohmann::json& json) {
  auto c = json.at("class").get<std::string>();
  if (c == "complex_modifications") {
    krbn::core_configuration::details::complex_modifications(json.at("input"));
  } else if (c == "devices") {
    krbn::core_configuration::details::device(json.at("input"));
  } else if (c == "parameters") {
    json.at("input").get<krbn::core_configuration::details::parameters>();
  } else if (c == "profile") {
    krbn::core_configuration::details::profile(json.at("input"));
  } else if (c == "simple_modifications") {
    krbn::core_configuration::details::simple_modifications m;
    m.update(json.at("input"));
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
        if (!e.at("error").is_null()) {
          try {
            handle_json(e);
            expect(false) << "The expected exception is not thrown";
          } catch (pqrs::json::unmarshal_error& ex) {
            expect(std::string_view(e.at("error").get<std::string>()) == ex.what());
          } catch (...) {
            expect(false) << "An unexpected exception is thrown";
          }
        }
      }
    }
  };
}
