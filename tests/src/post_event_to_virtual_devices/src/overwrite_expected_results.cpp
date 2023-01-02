#include "../../share/json_helper.hpp"
#include "../../share/manipulator_helper.hpp"
#include "manipulator/manipulators/post_event_to_virtual_devices/post_event_to_virtual_devices.hpp"
#include "run_loop_thread_utility.hpp"

int main(void) {
  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();
  auto scoped_run_loop_thread_manager = krbn::run_loop_thread_utility::initialize_shared_run_loop_thread();

  auto helper = std::make_unique<krbn::unit_testing::manipulator_helper>();

  helper->run_tests(krbn::unit_testing::json_helper::load_jsonc("json/tests.json"), true);

  helper = nullptr;

  return 0;
}
