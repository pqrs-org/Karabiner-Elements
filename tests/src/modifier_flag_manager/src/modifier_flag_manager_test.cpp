#include <catch2/catch.hpp>

#include "modifier_flag_manager.hpp"

namespace {
krbn::modifier_flag_manager::active_modifier_flag left_shift_1(krbn::modifier_flag_manager::active_modifier_flag::type::increase,
                                                               krbn::modifier_flag::left_shift,
                                                               krbn::device_id(1));

krbn::modifier_flag_manager::active_modifier_flag decrease_left_shift_1(krbn::modifier_flag_manager::active_modifier_flag::type::decrease,
                                                                        krbn::modifier_flag::left_shift,
                                                                        krbn::device_id(1));

krbn::modifier_flag_manager::active_modifier_flag lock_left_shift(krbn::modifier_flag_manager::active_modifier_flag::type::increase_lock,
                                                                  krbn::modifier_flag::left_shift,
                                                                  krbn::device_id(1));

krbn::modifier_flag_manager::active_modifier_flag decrease_lock_left_shift(krbn::modifier_flag_manager::active_modifier_flag::type::decrease_lock,
                                                                           krbn::modifier_flag::left_shift,
                                                                           krbn::device_id(1));

krbn::modifier_flag_manager::active_modifier_flag led_lock_caps_lock(krbn::modifier_flag_manager::active_modifier_flag::type::increase_led_lock,
                                                                     krbn::modifier_flag::caps_lock,
                                                                     krbn::device_id(1));

krbn::modifier_flag_manager::active_modifier_flag decrease_led_lock_caps_lock(krbn::modifier_flag_manager::active_modifier_flag::type::decrease_led_lock,
                                                                              krbn::modifier_flag::caps_lock,
                                                                              krbn::device_id(1));

krbn::modifier_flag_manager::active_modifier_flag sticky_left_shift(krbn::modifier_flag_manager::active_modifier_flag::type::increase_sticky,
                                                                    krbn::modifier_flag::left_shift,
                                                                    krbn::device_id(1));

krbn::modifier_flag_manager::active_modifier_flag decrease_sticky_left_shift(krbn::modifier_flag_manager::active_modifier_flag::type::decrease_sticky,
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

    modifier_flag_manager.push_back_active_modifier_flag(lock_left_shift);
    modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == true);

    modifier_flag_manager.erase_all_active_modifier_flags_except_lock(krbn::device_id(1));
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == true);

    modifier_flag_manager.push_back_active_modifier_flag(decrease_lock_left_shift);
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == false);
  }

  //
  // type::decrease_lock
  //

  {
    krbn::modifier_flag_manager modifier_flag_manager;

    modifier_flag_manager.push_back_active_modifier_flag(decrease_lock_left_shift);
    modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
    // lock_left_shift(-1), left_shift(+1)
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == false);

    modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
    // lock_left_shift(-1), left_shift(+2)
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == true);

    modifier_flag_manager.push_back_active_modifier_flag(decrease_left_shift_1);
    // lock_left_shift(-1), left_shift(+1)
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == false);

    modifier_flag_manager.push_back_active_modifier_flag(decrease_lock_left_shift);
    // lock_left_shift(-1), left_shift(+1)
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == false);

    modifier_flag_manager.push_back_active_modifier_flag(lock_left_shift);
    // lock_left_shift(0), left_shift(+1)
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == true);

    modifier_flag_manager.push_back_active_modifier_flag(decrease_left_shift_1);
    // lock_left_shift(0), left_shift(0)
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == false);
  }

  //
  // lock twice
  //

  {
    krbn::modifier_flag_manager modifier_flag_manager;

    modifier_flag_manager.push_back_active_modifier_flag(lock_left_shift);
    modifier_flag_manager.push_back_active_modifier_flag(lock_left_shift);
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == true);

    modifier_flag_manager.push_back_active_modifier_flag(decrease_lock_left_shift);
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == false);
  }

  //
  // led_lock
  //

  {
    krbn::modifier_flag_manager modifier_flag_manager;

    modifier_flag_manager.push_back_active_modifier_flag(led_lock_caps_lock);
    // led_lock_caps_lock(1)
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::caps_lock) == true);

    modifier_flag_manager.push_back_active_modifier_flag(led_lock_caps_lock);
    // led_lock_caps_lock(1)
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::caps_lock) == true);

    modifier_flag_manager.push_back_active_modifier_flag(decrease_led_lock_caps_lock);
    // led_lock_caps_lock(0)
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::caps_lock) == false);

    modifier_flag_manager.push_back_active_modifier_flag(decrease_led_lock_caps_lock);
    // led_lock_caps_lock(0)
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::caps_lock) == false);

    modifier_flag_manager.push_back_active_modifier_flag(led_lock_caps_lock);
    // led_lock_caps_lock(1)
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::caps_lock) == true);
  }

  //
  // sticky
  //

  {
    krbn::modifier_flag_manager modifier_flag_manager;

    modifier_flag_manager.push_back_active_modifier_flag(sticky_left_shift);
    // sticky_left_shift(1)
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == true);

    modifier_flag_manager.push_back_active_modifier_flag(sticky_left_shift);
    // sticky_left_shift(2)
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == true);

    modifier_flag_manager.push_back_active_modifier_flag(decrease_sticky_left_shift);
    // sticky_left_shift(1)
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == true);

    modifier_flag_manager.push_back_active_modifier_flag(decrease_sticky_left_shift);
    // sticky_left_shift(0)
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == false);

    modifier_flag_manager.push_back_active_modifier_flag(decrease_sticky_left_shift);
    // sticky_left_shift(-1)
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == false);

    modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
    // sticky_left_shift(-1), left_shift(+1)
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == false);

    modifier_flag_manager.erase_all_sticky_modifier_flags();
    // sticky_left_shift(0), left_shift(+1)
    REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == true);
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
