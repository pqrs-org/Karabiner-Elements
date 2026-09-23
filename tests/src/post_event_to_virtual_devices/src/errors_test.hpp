#include "../../share/json_helper.hpp"
#include "manipulator/manipulators/mouse_motion_and_wheel_to_key/mouse_motion_and_wheel_to_key.hpp"
#include <boost/ut.hpp>

void run_errors_test() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "configuration errors"_test = [] {
    for (const auto& file : krbn::unit_testing::json_helper::load_jsonc("json/errors.jsonc")) {
      for (const auto& e : krbn::unit_testing::json_helper::load_jsonc("json/" + file.get<std::string>())) {
        try {
          if (e.at("class") == "mouse_motion_and_wheel_to_key") {
            auto parameters = std::make_shared<krbn::core_configuration::details::complex_modifications_parameters>();
            krbn::manipulator::manipulators::mouse_motion_and_wheel_to_key::mouse_motion_and_wheel_to_key(e.at("input"), parameters);
          }
          expect(false) << e;
        } catch (const pqrs::json::unmarshal_error& ex) {
          expect(e.at("error").get<std::string>() == ex.what()) << e << ex.what();
        } catch (...) {
          expect(false) << e;
        }
      }
    }
  };
}
