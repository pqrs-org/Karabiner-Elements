#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "pointing_button_manager.hpp"
#include "thread_utility.hpp"

TEST_CASE("manipulator.pointing_button_manager") {
  krbn::pointing_button_manager pointing_button_manager;

  krbn::pointing_button_manager::active_pointing_button button1_1(krbn::pointing_button_manager::active_pointing_button::type::increase,
                                                                  krbn::pointing_button::button1,
                                                                  krbn::device_id(1));

  krbn::pointing_button_manager::active_pointing_button button2_1(krbn::pointing_button_manager::active_pointing_button::type::increase,
                                                                  krbn::pointing_button::button2,
                                                                  krbn::device_id(1));

  krbn::pointing_button_manager::active_pointing_button button3_1(krbn::pointing_button_manager::active_pointing_button::type::increase,
                                                                  krbn::pointing_button::button3,
                                                                  krbn::device_id(1));

  krbn::pointing_button_manager::active_pointing_button button20_1(krbn::pointing_button_manager::active_pointing_button::type::increase,
                                                                   krbn::pointing_button::button20,
                                                                   krbn::device_id(1));

  pointing_button_manager.push_back_active_pointing_button(button1_1);
  REQUIRE(pointing_button_manager.get_hid_report_bits() == 0x1);

  pointing_button_manager.push_back_active_pointing_button(button2_1);
  REQUIRE(pointing_button_manager.get_hid_report_bits() == 0x3);

  pointing_button_manager.push_back_active_pointing_button(button3_1);
  REQUIRE(pointing_button_manager.get_hid_report_bits() == 0x7);

  pointing_button_manager.push_back_active_pointing_button(button20_1);
  REQUIRE(pointing_button_manager.get_hid_report_bits() == 0x80007);
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
