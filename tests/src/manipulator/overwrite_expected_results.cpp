#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "../share/manipulator_helper.hpp"
#include "thread_utility.hpp"
#include <boost/optional/optional_io.hpp>

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("actual examples") {
  krbn::unit_testing::manipulator_helper::run_tests(nlohmann::json::parse(std::ifstream("json/manipulator_manager/tests.json")),
                                                    true);
}
