#include <catch2/catch.hpp>

#include "modifier_flag_manager.hpp"

namespace {
krbn::modifier_flag_manager::active_modifier_flag left_shift_1(krbn::modifier_flag_manager::active_modifier_flag::type::increase,
                                                               krbn::modifier_flag::left_shift,
                                                               krbn::device_id(1));

krbn::modifier_flag_manager::active_modifier_flag decrease_left_shift_1(krbn::modifier_flag_manager::active_modifier_flag::type::decrease,
                                                                        krbn::modifier_flag::left_shift,
                                                                        krbn::device_id(1));

krbn::modifier_flag_manager::active_modifier_flag lock_left_shift_1(krbn::modifier_flag_manager::active_modifier_flag::type::increase_lock,
                                                                    krbn::modifier_flag::left_shift,
                                                                    krbn::device_id(1));

krbn::modifier_flag_manager::active_modifier_flag decrease_lock_left_shift_1(krbn::modifier_flag_manager::active_modifier_flag::type::decrease_lock,
                                                                             krbn::modifier_flag::left_shift,
                                                                             krbn::device_id(1));

krbn::modifier_flag_manager::active_modifier_flag left_shift_2(krbn::modifier_flag_manager::active_modifier_flag::type::increase,
                                                               krbn::modifier_flag::left_shift,
                                                               krbn::device_id(2));

krbn::modifier_flag_manager::active_modifier_flag decrease_left_shift_2(krbn::modifier_flag_manager::active_modifier_flag::type::decrease,
                                                                        krbn::modifier_flag::left_shift,
                                                                        krbn::device_id(2));

krbn::modifier_flag_manager::active_modifier_flag right_shift_1(krbn::modifier_flag_manager::active_modifier_flag::type::increase,
                                                                krbn::modifier_flag::right_shift,
                                                                krbn::device_id(1));

krbn::modifier_flag_manager::active_modifier_flag right_command_1(krbn::modifier_flag_manager::active_modifier_flag::type::increase,
                                                                  krbn::modifier_flag::right_command,
                                                                  krbn::device_id(1));
} // namespace

TEST_CASE("modifier_flag_manager") {
  // ----------------------------------------
  // Push back
  {
    krbn::modifier_flag_manager modifier_flag_manager;

    modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
    REQUIRE(modifier_flag_manager.get_active_modifier_flags().size() == 1);
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == true);
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::right_shift) == false);
  }

  // ----------------------------------------
  // Erase
  {
    krbn::modifier_flag_manager modifier_flag_manager;

    modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
    modifier_flag_manager.push_back_active_modifier_flag(decrease_left_shift_1);
    REQUIRE(modifier_flag_manager.get_active_modifier_flags().size() == 0);
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == false);
  }

  // ----------------------------------------
  // Push back twice
  {
    krbn::modifier_flag_manager modifier_flag_manager;

    modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
    modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
    REQUIRE(modifier_flag_manager.get_active_modifier_flags().size() == 2);
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == true);

    modifier_flag_manager.push_back_active_modifier_flag(decrease_left_shift_1);
    REQUIRE(modifier_flag_manager.get_active_modifier_flags().size() == 1);
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == true);

    modifier_flag_manager.push_back_active_modifier_flag(decrease_left_shift_1);
    REQUIRE(modifier_flag_manager.get_active_modifier_flags().size() == 0);
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == false);
  }

  // ----------------------------------------
  // Erase first
  {
    krbn::modifier_flag_manager modifier_flag_manager;

    modifier_flag_manager.push_back_active_modifier_flag(decrease_left_shift_1);
    REQUIRE(modifier_flag_manager.get_active_modifier_flags().size() == 1);
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == false);

    modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
    REQUIRE(modifier_flag_manager.get_active_modifier_flags().size() == 0);
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == false);

    modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
    REQUIRE(modifier_flag_manager.get_active_modifier_flags().size() == 1);
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == true);
  }

  // ----------------------------------------
  // Multiple device id
  {
    krbn::modifier_flag_manager modifier_flag_manager;

    modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
    modifier_flag_manager.push_back_active_modifier_flag(left_shift_2);
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == true);

    modifier_flag_manager.push_back_active_modifier_flag(decrease_left_shift_1);
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == true);

    modifier_flag_manager.push_back_active_modifier_flag(decrease_left_shift_2);
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == false);
  }

  // ----------------------------------------
  // Erase all
  {
    krbn::modifier_flag_manager modifier_flag_manager;

    modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
    modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
    modifier_flag_manager.push_back_active_modifier_flag(left_shift_2);
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == true);

    modifier_flag_manager.erase_all_active_modifier_flags(krbn::device_id(1));
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == true);

    modifier_flag_manager.erase_all_active_modifier_flags(krbn::device_id(2));
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == false);
  }

  // ----------------------------------------
  // type::increase_lock
  {
    krbn::modifier_flag_manager modifier_flag_manager;

    modifier_flag_manager.push_back_active_modifier_flag(lock_left_shift_1);
    modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == true);

    modifier_flag_manager.erase_all_active_modifier_flags_except_lock(krbn::device_id(1));
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == true);

    modifier_flag_manager.push_back_active_modifier_flag(decrease_lock_left_shift_1);
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == false);
  }
}

TEST_CASE("modifier_flag_manager::make_hid_report_modifiers") {
  {
    namespace hid_report = pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report;

    krbn::modifier_flag_manager modifier_flag_manager;
    hid_report::modifiers expected;

    REQUIRE(modifier_flag_manager.make_hid_report_modifiers() == expected);

    modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
    modifier_flag_manager.push_back_active_modifier_flag(right_shift_1);
    modifier_flag_manager.push_back_active_modifier_flag(right_command_1);
    expected.insert(hid_report::modifier::left_shift);
    expected.insert(hid_report::modifier::right_shift);
    expected.insert(hid_report::modifier::right_command);
    REQUIRE(modifier_flag_manager.make_hid_report_modifiers() == expected);
  }
}
