#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "../share/manipulator_helper.hpp"
#include "manipulator/details/post_event_to_virtual_devices.hpp"
#include "thread_utility.hpp"
#include <boost/optional/optional_io.hpp>

TEST_CASE("actual examples") {
  krbn::unit_testing::manipulator_helper::run_tests(nlohmann::json::parse(std::ifstream("json/tests.json")));
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
