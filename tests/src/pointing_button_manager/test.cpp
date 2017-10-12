#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "pointing_button_manager.hpp"
#include "thread_utility.hpp"

namespace {
krbn::pointing_button_manager::active_pointing_button button1_1(krbn::pointing_button_manager::active_pointing_button::type::increase,
                                                                krbn::pointing_button::button1,
                                                                krbn::device_id(1));

krbn::pointing_button_manager::active_pointing_button button1_decrease_1(krbn::pointing_button_manager::active_pointing_button::type::decrease,
                                                                         krbn::pointing_button::button1,
                                                                         krbn::device_id(1));

krbn::pointing_button_manager::active_pointing_button button1_2(krbn::pointing_button_manager::active_pointing_button::type::increase,
                                                                krbn::pointing_button::button1,
                                                                krbn::device_id(2));

krbn::pointing_button_manager::active_pointing_button button2_1(krbn::pointing_button_manager::active_pointing_button::type::increase,
                                                                krbn::pointing_button::button2,
                                                                krbn::device_id(1));

krbn::pointing_button_manager::active_pointing_button button3_1(krbn::pointing_button_manager::active_pointing_button::type::increase,
                                                                krbn::pointing_button::button3,
                                                                krbn::device_id(1));

krbn::pointing_button_manager::active_pointing_button button20_1(krbn::pointing_button_manager::active_pointing_button::type::increase,
                                                                 krbn::pointing_button::button20,
                                                                 krbn::device_id(1));
} // namespace

TEST_CASE("pointing_button_manager") {
  {
    krbn::pointing_button_manager pointing_button_manager;

    pointing_button_manager.push_back_active_pointing_button(button1_1);
    REQUIRE(pointing_button_manager.get_hid_report_bits() == 0x1);

    {
      auto report = pointing_button_manager.make_pointing_input_report();
      REQUIRE(report.buttons[0] == 0x1);
      REQUIRE(report.buttons[1] == 0x0);
      REQUIRE(report.buttons[2] == 0x0);
      REQUIRE(report.buttons[3] == 0x0);
    }

    pointing_button_manager.push_back_active_pointing_button(button2_1);
    REQUIRE(pointing_button_manager.get_hid_report_bits() == 0x3);

    {
      auto report = pointing_button_manager.make_pointing_input_report();
      REQUIRE(report.buttons[0] == 0x3);
      REQUIRE(report.buttons[1] == 0x0);
      REQUIRE(report.buttons[2] == 0x0);
      REQUIRE(report.buttons[3] == 0x0);
    }

    pointing_button_manager.push_back_active_pointing_button(button3_1);
    REQUIRE(pointing_button_manager.get_hid_report_bits() == 0x7);

    {
      auto report = pointing_button_manager.make_pointing_input_report();
      REQUIRE(report.buttons[0] == 0x7);
      REQUIRE(report.buttons[1] == 0x0);
      REQUIRE(report.buttons[2] == 0x0);
      REQUIRE(report.buttons[3] == 0x0);
    }

    pointing_button_manager.push_back_active_pointing_button(button20_1);
    REQUIRE(pointing_button_manager.get_hid_report_bits() == 0x80007);

    {
      auto report = pointing_button_manager.make_pointing_input_report();
      REQUIRE(report.buttons[0] == 0x7);
      REQUIRE(report.buttons[1] == 0x0);
      REQUIRE(report.buttons[2] == 0x8);
      REQUIRE(report.buttons[3] == 0x0);
    }
  }

  // decrease first
  {
    krbn::pointing_button_manager pointing_button_manager;

    pointing_button_manager.push_back_active_pointing_button(button1_decrease_1);
    REQUIRE(pointing_button_manager.get_hid_report_bits() == 0x0);

    pointing_button_manager.push_back_active_pointing_button(button1_1);
    REQUIRE(pointing_button_manager.get_hid_report_bits() == 0x1);
  }

  // push twice
  {
    krbn::pointing_button_manager pointing_button_manager;

    pointing_button_manager.push_back_active_pointing_button(button1_1);
    REQUIRE(pointing_button_manager.get_hid_report_bits() == 0x1);

    pointing_button_manager.push_back_active_pointing_button(button1_decrease_1);
    REQUIRE(pointing_button_manager.get_hid_report_bits() == 0x0);

    pointing_button_manager.push_back_active_pointing_button(button1_1);
    pointing_button_manager.push_back_active_pointing_button(button1_1);
    pointing_button_manager.push_back_active_pointing_button(button1_decrease_1);
    REQUIRE(pointing_button_manager.get_hid_report_bits() == 0x0);
  }

  // multiple devices
  {
    krbn::pointing_button_manager pointing_button_manager;

    pointing_button_manager.push_back_active_pointing_button(button1_1);
    pointing_button_manager.push_back_active_pointing_button(button1_2);
    REQUIRE(pointing_button_manager.get_hid_report_bits() == 0x1);

    pointing_button_manager.push_back_active_pointing_button(button1_decrease_1);
    REQUIRE(pointing_button_manager.get_hid_report_bits() == 0x0);
  }
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
