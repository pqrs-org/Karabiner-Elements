#include "../../share/json_helper.hpp"
#include "manipulator/manipulators/basic/from_event_definition.hpp"
#include "manipulator/types.hpp"
#include <boost/ut.hpp>
#include <pqrs/json.hpp>

int main() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "simple_modifications.json"_test = [] {
    auto json = krbn::unit_testing::json_helper::load_jsonc("../../../src/apps/SettingsWindow/Resources/simple_modifications.json");

    for (const auto& entry : json) {
      if (auto data = pqrs::json::find_array(entry, "data")) {
        auto not_from = false;
        if (auto nf = pqrs::json::find<bool>(entry, "not_from")) {
          not_from = *nf;
        }

        auto not_to = false;
        if (auto nt = pqrs::json::find<bool>(entry, "not_to")) {
          not_to = *nt;
        }

        //
        // Check `data` is valid as `to`
        //

        if (!not_to) {
          for (const auto& d : **data) {
            krbn::manipulator::to_event_definition e(d);
          }
        }

        //
        // Check `data` is valid as `from`
        //

        if (!not_from) {
          expect((**data).size() == 1);

          auto from = (**data)[0];
          from["modifiers"]["optional"] = nlohmann::json::array({"any"});
          krbn::manipulator::manipulators::basic::from_event_definition e(from);
        }
      }
    }
  };

  return 0;
}
