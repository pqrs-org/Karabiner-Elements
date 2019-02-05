#include <catch2/catch.hpp>

#include "pointing_button_manager.hpp"

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
    pqrs::karabiner_virtual_hid_device::hid_report::buttons expected;

    pointing_button_manager.push_back_active_pointing_button(button1_1);
    expected.insert(1);
    REQUIRE(pointing_button_manager.make_hid_report_buttons() == expected);

    pointing_button_manager.push_back_active_pointing_button(button2_1);
    expected.insert(2);
    REQUIRE(pointing_button_manager.make_hid_report_buttons() == expected);

    pointing_button_manager.push_back_active_pointing_button(button3_1);
    expected.insert(3);
    REQUIRE(pointing_button_manager.make_hid_report_buttons() == expected);

    pointing_button_manager.push_back_active_pointing_button(button20_1);
    expected.insert(20);
    REQUIRE(pointing_button_manager.make_hid_report_buttons() == expected);
  }

  // decrease first
  {
    krbn::pointing_button_manager pointing_button_manager;
    pqrs::karabiner_virtual_hid_device::hid_report::buttons expected;

    pointing_button_manager.push_back_active_pointing_button(button1_decrease_1);
    REQUIRE(pointing_button_manager.make_hid_report_buttons() == expected);

    pointing_button_manager.push_back_active_pointing_button(button1_1);
    expected.insert(1);
    REQUIRE(pointing_button_manager.make_hid_report_buttons() == expected);
  }

  // push twice
  {
    krbn::pointing_button_manager pointing_button_manager;
    pqrs::karabiner_virtual_hid_device::hid_report::buttons expected;

    pointing_button_manager.push_back_active_pointing_button(button1_1);
    expected.insert(1);
    REQUIRE(pointing_button_manager.make_hid_report_buttons() == expected);

    pointing_button_manager.push_back_active_pointing_button(button1_decrease_1);
    expected.erase(1);
    REQUIRE(pointing_button_manager.make_hid_report_buttons() == expected);

    pointing_button_manager.push_back_active_pointing_button(button1_1);
    pointing_button_manager.push_back_active_pointing_button(button1_1);
    pointing_button_manager.push_back_active_pointing_button(button1_decrease_1);
    REQUIRE(pointing_button_manager.make_hid_report_buttons() == expected);
  }

  // multiple devices
  {
    krbn::pointing_button_manager pointing_button_manager;
    pqrs::karabiner_virtual_hid_device::hid_report::buttons expected;

    pointing_button_manager.push_back_active_pointing_button(button1_1);
    pointing_button_manager.push_back_active_pointing_button(button1_2);
    expected.insert(1);
    REQUIRE(pointing_button_manager.make_hid_report_buttons() == expected);

    pointing_button_manager.push_back_active_pointing_button(button1_decrease_1);
    expected.erase(1);
    REQUIRE(pointing_button_manager.make_hid_report_buttons() == expected);
  }
}
