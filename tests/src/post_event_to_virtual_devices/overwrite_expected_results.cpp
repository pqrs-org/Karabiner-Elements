#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "../share/manipulator_helper.hpp"
#include "manipulator/details/post_event_to_virtual_devices.hpp"
#include "thread_utility.hpp"
#include <boost/optional/optional_io.hpp>

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("actual examples") {
  auto time_source = std::make_shared<pqrs::dispatcher::pseudo_time_source>();
  auto dispatcher = std::make_shared<pqrs::dispatcher::dispatcher>(time_source);
  auto helper = std::make_unique<krbn::unit_testing::manipulator_helper>(time_source,
                                                                         dispatcher);

  helper->run_tests(nlohmann::json::parse(std::ifstream("json/tests.json")), true);

  helper = nullptr;

  dispatcher->terminate();
  dispatcher = nullptr;
}
