#include <catch2/catch.hpp>

#include "../../share/manipulator_helper.hpp"
#include "manipulator/manipulators/post_event_to_virtual_devices/post_event_to_virtual_devices.hpp"
#include "test_runner.hpp"

TEST_CASE("actual examples") {
  auto helper = std::make_unique<krbn::unit_testing::manipulator_helper>();

  helper->run_tests(nlohmann::json::parse(std::ifstream("json/tests.json")), true);

  helper = nullptr;
}

int main(int argc, char* argv[]) {
  return run_tests(argc, argv);
}
