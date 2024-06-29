#include "complex_modifications_assets_manager.hpp"
#include <boost/ut.hpp>
#include <iostream>

void run_complex_modifications_assets_manager_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "reload"_test = [] {
    {
      krbn::complex_modifications_assets_manager complex_modifications_assets_manager;
      complex_modifications_assets_manager.reload("json/complex_modifications",
                                                  krbn::core_configuration::error_handling::strict,
                                                  false);

      auto& files = complex_modifications_assets_manager.get_files();
      expect(files.size() == 5);
      expect(files[0]->get_title() == "CCC");
      {
        auto& rules = files[0]->get_rules();
        expect(rules.size() == 1);
        expect(rules[0]->get_description() == "CCC1");
      }

      expect(files[1]->get_title() == "EEE");
      {
        auto& rules = files[1]->get_rules();
        expect(rules.size() == 2);
        expect(rules[0]->get_description() == "EEE1");
        expect(rules[1]->get_description() == "EEE2");
      }

      expect(files[2]->get_title() == "FFF");
      expect(files[3]->get_title() == "SSS");
      expect(files[4]->get_title() == "symlink");
    }

    {
      krbn::complex_modifications_assets_manager complex_modifications_assets_manager;
      complex_modifications_assets_manager.reload("not_found",
                                                  krbn::core_configuration::error_handling::strict,
                                                  false);

      auto& files = complex_modifications_assets_manager.get_files();
      expect(files.size() == 0);
    }
  };
}
