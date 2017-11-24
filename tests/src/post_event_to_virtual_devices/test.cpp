#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "../share/manipulator_helper.hpp"
#include "manipulator/details/post_event_to_virtual_devices.hpp"
#include "thread_utility.hpp"
#include <boost/optional/optional_io.hpp>

TEST_CASE("actual examples") {
  krbn::unit_testing::manipulator_helper::run_tests(nlohmann::json::parse(std::ifstream("json/tests.json")));
}

TEST_CASE("mouse_key_handler.count_converter") {
  {
    krbn::manipulator::details::post_event_to_virtual_devices::mouse_key_handler::count_converter count_converter(64);
    REQUIRE(count_converter.update(32) == static_cast<uint8_t>(0));
    REQUIRE(count_converter.update(32) == static_cast<uint8_t>(1));
    REQUIRE(count_converter.update(32) == static_cast<uint8_t>(0));
    REQUIRE(count_converter.update(32) == static_cast<uint8_t>(1));

    REQUIRE(count_converter.update(16) == static_cast<uint8_t>(0));
    REQUIRE(count_converter.update(16) == static_cast<uint8_t>(0));
    REQUIRE(count_converter.update(16) == static_cast<uint8_t>(0));
    REQUIRE(count_converter.update(16) == static_cast<uint8_t>(1));

    REQUIRE(count_converter.update(-16) == static_cast<uint8_t>(0));
    REQUIRE(count_converter.update(-16) == static_cast<uint8_t>(0));
    REQUIRE(count_converter.update(-16) == static_cast<uint8_t>(0));
    REQUIRE(count_converter.update(-16) == static_cast<uint8_t>(-1));
  }
  {
    krbn::manipulator::details::post_event_to_virtual_devices::mouse_key_handler::count_converter count_converter(64);
    REQUIRE(count_converter.update(128) == static_cast<uint8_t>(2));
    REQUIRE(count_converter.update(128) == static_cast<uint8_t>(2));
    REQUIRE(count_converter.update(-128) == static_cast<uint8_t>(-2));
    REQUIRE(count_converter.update(-128) == static_cast<uint8_t>(-2));
  }
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
