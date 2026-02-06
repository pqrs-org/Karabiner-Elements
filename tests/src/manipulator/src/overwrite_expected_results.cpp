#include "../../share/json_helper.hpp"
#include "../../share/manipulator_helper.hpp"
#include "run_loop_thread_utility.hpp"

int main(void) {
  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();
  auto scoped_run_loop_thread_manager = krbn::run_loop_thread_utility::initialize_scoped_run_loop_thread_manager(
      pqrs::cf::run_loop_thread::failure_policy::abort);

  auto helper = std::make_unique<krbn::unit_testing::manipulator_helper>();

  helper->run_tests(krbn::unit_testing::json_helper::load_jsonc("json/manipulator_manager/tests.json"),
                    true);

  helper = nullptr;

  return 0;
}
