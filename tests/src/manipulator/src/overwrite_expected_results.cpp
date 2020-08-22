#include <catch2/catch.hpp>

#include "../../share/json_helper.hpp"
#include "../../share/manipulator_helper.hpp"
#include "test_runner.hpp"

TEST_CASE("actual examples") {
  auto helper = std::make_unique<krbn::unit_testing::manipulator_helper>();
  helper->run_tests(krbn::unit_testing::json_helper::load_jsonc("json/manipulator_manager/tests.json"),
                    true);

  helper = nullptr;
}

int main(int argc, char* argv[]) {
  return run_tests(argc, argv);
}
