#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "system_preferences.hpp"
#include "thread_utility.hpp"

TEST_CASE("convert") {
  for (uint32_t i = 0; i < 50000; ++i) {
    auto f = system_preferences::convert_key_repeat_milliseconds_to_system_preferences_value(i);
    auto v = system_preferences::convert_key_repeat_system_preferences_value_to_milliseconds(f);

    REQUIRE(v == i);
  }
}

int main(int argc, char* const argv[]) {
  thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
