#include "../../share/json_helper.hpp"
#include "manipulator/types.hpp"
#include <boost/ut.hpp>
#include <pqrs/json.hpp>

int main(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "simple_modifications.json"_test = [] {
    auto json = krbn::unit_testing::json_helper::load_jsonc("../../../src/apps/SettingsWindow/Resources/simple_modifications.json");
    auto label_set = std::unordered_set<std::string>();

    for (const auto& entry : json) {
      if (auto data = pqrs::json::find_array(entry, "data")) {
        //
        // Check `data` is valid
        //

        for (const auto& d : **data) {
          krbn::manipulator::to_event_definition e(d);
        }

        //
        // Check `from` data array size == 1
        //

        auto not_from = false;
        if (auto nf = pqrs::json::find<bool>(entry, "not_from")) {
          not_from = *nf;
        }
        if (!not_from) {
          expect((**data).size() == 1);
        }

        //
        // Check `label` uniqueness
        //

        if (auto label = pqrs::json::find<std::string>(entry, "label")) {
          std::cout << *label << std::endl;
          if (label_set.contains(*label) == true) {
            std::cout << std::endl
                      << *label << " is duplicated." << std::endl
                      << std::endl;
            expect(false);
          }

          label_set.insert(*label);
        }
      }
    }

    expect(label_set.size() > 0);
  };

  return 0;
}
