#include "../../share/json_helper.hpp"
#include "../../share/manipulator_helper.hpp"
#include "manipulator/manipulators/post_event_to_virtual_devices/post_event_to_virtual_devices.hpp"
#include "run_loop_thread_utility.hpp"
#include <boost/ut.hpp>

int main(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();
  auto scoped_run_loop_thread_manager = krbn::run_loop_thread_utility::initialize_shared_run_loop_thread();

  "actual examples"_test = [] {
    auto helper = std::make_unique<krbn::unit_testing::manipulator_helper>();

    helper->run_tests(krbn::unit_testing::json_helper::load_jsonc("json/tests.json"));

    helper = nullptr;
  };

  "mouse_key_handler.count_converter"_test = [] {
    {
      krbn::manipulator::manipulators::post_event_to_virtual_devices::mouse_key_handler::count_converter count_converter(64);
      expect(count_converter.update(32) == static_cast<uint8_t>(0));
      expect(count_converter.update(32) == static_cast<uint8_t>(1));
      expect(count_converter.update(32) == static_cast<uint8_t>(0));
      expect(count_converter.update(32) == static_cast<uint8_t>(1));

      expect(count_converter.update(16) == static_cast<uint8_t>(0));
      expect(count_converter.update(16) == static_cast<uint8_t>(0));
      expect(count_converter.update(16) == static_cast<uint8_t>(0));
      expect(count_converter.update(16) == static_cast<uint8_t>(1));

      expect(count_converter.update(-16) == static_cast<uint8_t>(0));
      expect(count_converter.update(-16) == static_cast<uint8_t>(0));
      expect(count_converter.update(-16) == static_cast<uint8_t>(0));
      expect(count_converter.update(-16) == static_cast<uint8_t>(-1));
    }
    {
      krbn::manipulator::manipulators::post_event_to_virtual_devices::mouse_key_handler::count_converter count_converter(64);
      expect(count_converter.update(128) == static_cast<uint8_t>(2));
      expect(count_converter.update(128) == static_cast<uint8_t>(2));
      expect(count_converter.update(-128) == static_cast<uint8_t>(-2));
      expect(count_converter.update(-128) == static_cast<uint8_t>(-2));
    }
  };

  return 0;
}
