#include "../../share/json_helper.hpp"
#include "../../share/manipulator_helper.hpp"

int main(void) {
  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();

  auto helper = std::make_unique<krbn::unit_testing::manipulator_helper>();

  helper->run_tests(krbn::unit_testing::json_helper::load_jsonc("json/manipulator_manager/tests.json"),
                    true);

  helper = nullptr;

  return 0;
}
