#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "modifier_flag_manager.hpp"
#include "thread_utility.hpp"

TEST_CASE("manipulator.modifier_flag_manager") {
  krbn::modifier_flag_manager modifier_flag_manager;

  krbn::modifier_flag_manager::active_modifier_flag left_shift_1(krbn::modifier_flag_manager::active_modifier_flag::type::increase,
                                                                 krbn::modifier_flag::left_shift,
                                                                 krbn::device_id(1));

  krbn::modifier_flag_manager::active_modifier_flag decrease_left_shift_1(krbn::modifier_flag_manager::active_modifier_flag::type::decrease,
                                                                          krbn::modifier_flag::left_shift,
                                                                          krbn::device_id(1));

  krbn::modifier_flag_manager::active_modifier_flag lock_left_shift_1(krbn::modifier_flag_manager::active_modifier_flag::type::increase_lock,
                                                                      krbn::modifier_flag::left_shift,
                                                                      krbn::device_id(1));

  krbn::modifier_flag_manager::active_modifier_flag left_shift_2(krbn::modifier_flag_manager::active_modifier_flag::type::increase,
                                                                 krbn::modifier_flag::left_shift,
                                                                 krbn::device_id(2));

  // ----------------------------------------
  // Push back

  modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
  REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == true);
  REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::right_shift) == false);

  // ----------------------------------------
  // Erase

  modifier_flag_manager.erase_active_modifier_flag(left_shift_1);
  REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == false);

  // ----------------------------------------
  // Push back twice

  modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
  modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
  REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == true);

  modifier_flag_manager.erase_active_modifier_flag(left_shift_1);
  REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == true);

  modifier_flag_manager.erase_active_modifier_flag(left_shift_1);
  REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == false);

  // ----------------------------------------
  // Erase first (erase is ignored)

  modifier_flag_manager.erase_active_modifier_flag(left_shift_1);
  REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == false);

  modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
  REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == true);

  modifier_flag_manager.erase_active_modifier_flag(left_shift_1);
  REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == false);

  // ----------------------------------------
  // type::decrease

  modifier_flag_manager.push_back_active_modifier_flag(decrease_left_shift_1);
  REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == false);

  modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
  REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == false);

  modifier_flag_manager.erase_active_modifier_flag(decrease_left_shift_1);
  REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == true);

  modifier_flag_manager.erase_active_modifier_flag(left_shift_1);
  REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == false);

  // ----------------------------------------
  // Multiple device id

  modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
  modifier_flag_manager.push_back_active_modifier_flag(left_shift_2);
  REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == true);

  modifier_flag_manager.erase_active_modifier_flag(left_shift_1);
  REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == true);

  modifier_flag_manager.erase_active_modifier_flag(left_shift_2);
  REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == false);

  // ----------------------------------------
  // Erase all

  modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
  modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
  modifier_flag_manager.push_back_active_modifier_flag(left_shift_2);
  REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == true);

  modifier_flag_manager.erase_all_active_modifier_flags(krbn::device_id(1));
  REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == true);

  modifier_flag_manager.erase_all_active_modifier_flags(krbn::device_id(2));
  REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == false);

  // ----------------------------------------
  // type::increase_lock

  modifier_flag_manager.push_back_active_modifier_flag(lock_left_shift_1);
  modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
  REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == true);

  modifier_flag_manager.erase_all_active_modifier_flags_except_lock(krbn::device_id(1));
  REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == true);

  modifier_flag_manager.erase_active_modifier_flag(lock_left_shift_1);
  REQUIRE(modifier_flag_manager.is_pressed(krbn::modifier_flag::left_shift) == false);
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
