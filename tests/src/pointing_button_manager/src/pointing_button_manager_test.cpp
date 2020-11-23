#include <catch2/catch.hpp>

#include "pointing_button_manager.hpp"

namespace {
krbn::pointing_button_manager::active_pointing_button button1_1(
    krbn::pointing_button_manager::active_pointing_button::type::increase,
    pqrs::hid::usage_pair(pqrs::hid::usage_page::button, pqrs::hid::usage::button::button_1),
    krbn::device_id(1));

krbn::pointing_button_manager::active_pointing_button button1_decrease_1(
    krbn::pointing_button_manager::active_pointing_button::type::decrease,
    pqrs::hid::usage_pair(pqrs::hid::usage_page::button, pqrs::hid::usage::button::button_1),
    krbn::device_id(1));

krbn::pointing_button_manager::active_pointing_button button1_2(
    krbn::pointing_button_manager::active_pointing_button::type::increase,
    pqrs::hid::usage_pair(pqrs::hid::usage_page::button, pqrs::hid::usage::button::button_1),
    krbn::device_id(2));

krbn::pointing_button_manager::active_pointing_button button2_1(
    krbn::pointing_button_manager::active_pointing_button::type::increase,
    pqrs::hid::usage_pair(pqrs::hid::usage_page::button, pqrs::hid::usage::button::button_2),
    krbn::device_id(1));

krbn::pointing_button_manager::active_pointing_button button3_1(
    krbn::pointing_button_manager::active_pointing_button::type::increase,
    pqrs::hid::usage_pair(pqrs::hid::usage_page::button, pqrs::hid::usage::button::button_3),
    krbn::device_id(1));

krbn::pointing_button_manager::active_pointing_button button20_1(
    krbn::pointing_button_manager::active_pointing_button::type::increase,
    pqrs::hid::usage_pair(pqrs::hid::usage_page::button, pqrs::hid::usage::button::button_20),
    krbn::device_id(1));

krbn::pointing_button_manager::active_pointing_button button32_1(
    krbn::pointing_button_manager::active_pointing_button::type::increase,
    pqrs::hid::usage_pair(pqrs::hid::usage_page::button, pqrs::hid::usage::button::button_32),
    krbn::device_id(1));

// button0 == pqrs::hid::usage::undefined
krbn::pointing_button_manager::active_pointing_button button0_1(
    krbn::pointing_button_manager::active_pointing_button::type::increase,
    pqrs::hid::usage_pair(pqrs::hid::usage_page::button, pqrs::hid::usage::value_t(0)),
    krbn::device_id(1));
} // namespace

TEST_CASE("pointing_button_manager") {
  namespace hid_report = pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report;

  {
    krbn::pointing_button_manager pointing_button_manager;
    hid_report::buttons expected;

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

    pointing_button_manager.push_back_active_pointing_button(button32_1);
    expected.insert(32);
    REQUIRE(pointing_button_manager.make_hid_report_buttons() == expected);
  } // namespace pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report;

  // decrease first
  {
    krbn::pointing_button_manager pointing_button_manager;
    hid_report::buttons expected;

    pointing_button_manager.push_back_active_pointing_button(button1_decrease_1);
    REQUIRE(pointing_button_manager.make_hid_report_buttons() == expected);

    pointing_button_manager.push_back_active_pointing_button(button1_1);
    expected.insert(1);
    REQUIRE(pointing_button_manager.make_hid_report_buttons() == expected);
  }

  // push twice
  {
    krbn::pointing_button_manager pointing_button_manager;
    hid_report::buttons expected;

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
    hid_report::buttons expected;

    pointing_button_manager.push_back_active_pointing_button(button1_1);
    pointing_button_manager.push_back_active_pointing_button(button1_2);
    expected.insert(1);
    REQUIRE(pointing_button_manager.make_hid_report_buttons() == expected);

    pointing_button_manager.push_back_active_pointing_button(button1_decrease_1);
    expected.erase(1);
    REQUIRE(pointing_button_manager.make_hid_report_buttons() == expected);
  }

  // button0 is ignored
  {
    krbn::pointing_button_manager pointing_button_manager;

    pointing_button_manager.push_back_active_pointing_button(button0_1);
    REQUIRE(pointing_button_manager.is_pressed(button0_1.get_usage_pair()) == false);
  }
}
