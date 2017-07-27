#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "complex_modifications_assets_manager.hpp"
#include "thread_utility.hpp"
#include <iostream>

TEST_CASE("reload") {
  {
    krbn::complex_modifications_assets_manager complex_modifications_assets_manager;
    complex_modifications_assets_manager.reload("assets/complex_modifications", false);

    auto& files = complex_modifications_assets_manager.get_files();
    REQUIRE(files.size() == 5);
    REQUIRE(files[0].get_title() == "CCC");
    {
      auto& rules = files[0].get_rules();
      REQUIRE(rules.size() == 1);
      REQUIRE(rules[0].get_description() == "CCC1");
    }

    REQUIRE(files[1].get_title() == "EEE");
    {
      auto& rules = files[1].get_rules();
      REQUIRE(rules.size() == 2);
      REQUIRE(rules[0].get_description() == "EEE1");
      REQUIRE(rules[1].get_description() == "EEE2");
    }

    REQUIRE(files[2].get_title() == "FFF");
    REQUIRE(files[3].get_title() == "SSS");
    REQUIRE(files[4].get_title() == "symlink");
  }

  {
    krbn::complex_modifications_assets_manager complex_modifications_assets_manager;
    complex_modifications_assets_manager.reload("not_found", false);

    auto& files = complex_modifications_assets_manager.get_files();
    REQUIRE(files.size() == 0);
  }
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
