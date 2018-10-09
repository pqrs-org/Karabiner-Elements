#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "../share/manipulator_helper.hpp"
#include <boost/optional/optional_io.hpp>

TEST_CASE("initialize") {
  pqrs::dispatcher::extra::initialize_shared_dispatcher();
}

TEST_CASE("actual examples") {
  auto helper = std::make_unique<krbn::unit_testing::manipulator_helper>();
  helper->run_tests(nlohmann::json::parse(std::ifstream("json/manipulator_manager/tests.json")),
                    true);

  helper = nullptr;
}

TEST_CASE("terminate") {
  pqrs::dispatcher::extra::terminate_shared_dispatcher();
}
