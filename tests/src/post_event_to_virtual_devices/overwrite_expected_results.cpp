#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "../share/manipulator_helper.hpp"
#include "dispatcher_utility.hpp"
#include "manipulator/details/post_event_to_virtual_devices.hpp"
#include <boost/optional/optional_io.hpp>

TEST_CASE("initialize") {
  krbn::dispatcher_utility::initialize_dispatchers();
}

TEST_CASE("actual examples") {
  auto helper = std::make_unique<krbn::unit_testing::manipulator_helper>();

  helper->run_tests(nlohmann::json::parse(std::ifstream("json/tests.json")), true);

  helper = nullptr;
}

TEST_CASE("terminate") {
  krbn::dispatcher_utility::terminate_dispatchers();
}
