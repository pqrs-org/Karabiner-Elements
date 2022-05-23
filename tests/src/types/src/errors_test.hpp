#include "../../share/json_helper.hpp"
#include "types.hpp"
#include <boost/ut.hpp>

namespace {
void handle_json(const nlohmann::json& json) {
  using namespace boost::ut;

  auto c = json.at("class").get<std::string>();
  if (c == "device_identifiers") {
    json.at("input").get<krbn::device_identifiers>();
  } else if (c == "mouse_key") {
    json.at("input").get<krbn::mouse_key>();
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
          expect(e.at("error").get<std::string>() == ex.what());
        } catch (...) {
          expect(false);
        }
      }
    }
  };
}
