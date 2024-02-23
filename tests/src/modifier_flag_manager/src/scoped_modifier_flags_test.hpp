#pragma once

#include "modifier_flag_manager.hpp"
#include <boost/ut.hpp>

void run_scoped_modifier_flags_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "modifier_flag_manager::scoped_modifier_flags"_test = [] {
    using namespace krbn;
    typedef modifier_flag_manager::active_modifier_flag active_modifier_flag;
    typedef modifier_flag_manager::scoped_modifier_flags scoped_modifier_flags;

    {
      modifier_flag_manager modifier_flag_manager;

      modifier_flag_manager.push_back_active_modifier_flag(active_modifier_flag(active_modifier_flag::type::increase, modifier_flag::left_command, device_id(1)));
      modifier_flag_manager.push_back_active_modifier_flag(active_modifier_flag(active_modifier_flag::type::increase_lock, modifier_flag::left_command, device_id(1)));
      modifier_flag_manager.push_back_active_modifier_flag(active_modifier_flag(active_modifier_flag::type::increase_sticky, modifier_flag::left_command, device_id(1)));
      modifier_flag_manager.push_back_active_modifier_flag(active_modifier_flag(active_modifier_flag::type::decrease, modifier_flag::left_option, device_id(1)));
      modifier_flag_manager.push_back_active_modifier_flag(active_modifier_flag(active_modifier_flag::type::decrease_lock, modifier_flag::left_option, device_id(1)));
      modifier_flag_manager.push_back_active_modifier_flag(active_modifier_flag(active_modifier_flag::type::decrease_sticky, modifier_flag::left_option, device_id(1)));
      modifier_flag_manager.push_back_active_modifier_flag(active_modifier_flag(active_modifier_flag::type::increase_led_lock, modifier_flag::caps_lock, device_id(1)));

      auto active_modifier_flags = modifier_flag_manager.get_active_modifier_flags();

      expect(std::unordered_set<modifier_flag>({
                 modifier_flag::caps_lock,
                 modifier_flag::left_command,
             }) == modifier_flag_manager.make_modifier_flags());

      {
        scoped_modifier_flags scoped_modifier_flags(modifier_flag_manager, std::unordered_set<modifier_flag>{
                                                                               modifier_flag::left_shift,
                                                                           });

        expect(std::unordered_set<modifier_flag>({
                   modifier_flag::left_shift,
               }) == modifier_flag_manager.make_modifier_flags());

        std::cout << std::endl
                  << scoped_modifier_flags.get_scoped_active_modifier_flags()
                  << std::endl;

        expect(std::vector<active_modifier_flag>({
                   active_modifier_flag(active_modifier_flag::type::decrease_led_lock, modifier_flag::caps_lock, device_id(0)),
                   active_modifier_flag(active_modifier_flag::type::increase, modifier_flag::left_shift, device_id(0)),
                   active_modifier_flag(active_modifier_flag::type::decrease, modifier_flag::left_command, device_id(0)),
                   active_modifier_flag(active_modifier_flag::type::decrease_lock, modifier_flag::left_command, device_id(0)),
                   active_modifier_flag(active_modifier_flag::type::decrease_sticky, modifier_flag::left_command, device_id(0)),
               }) == scoped_modifier_flags.get_scoped_active_modifier_flags());
      }

      expect(active_modifier_flags == modifier_flag_manager.get_active_modifier_flags());

      {
        scoped_modifier_flags scoped_modifier_flags(modifier_flag_manager, std::unordered_set<modifier_flag>{
                                                                               modifier_flag::left_option,
                                                                           });

        expect(std::unordered_set<modifier_flag>({
                   modifier_flag::left_option,
               }) == modifier_flag_manager.make_modifier_flags());

        std::cout << std::endl
                  << scoped_modifier_flags.get_scoped_active_modifier_flags()
                  << std::endl;

        expect(std::vector<active_modifier_flag>({
                   active_modifier_flag(active_modifier_flag::type::decrease_led_lock, modifier_flag::caps_lock, device_id(0)),
                   active_modifier_flag(active_modifier_flag::type::increase, modifier_flag::left_option, device_id(0)),
                   active_modifier_flag(active_modifier_flag::type::increase, modifier_flag::left_option, device_id(0)),
                   active_modifier_flag(active_modifier_flag::type::increase, modifier_flag::left_option, device_id(0)),
                   active_modifier_flag(active_modifier_flag::type::increase, modifier_flag::left_option, device_id(0)),
                   active_modifier_flag(active_modifier_flag::type::decrease, modifier_flag::left_command, device_id(0)),
                   active_modifier_flag(active_modifier_flag::type::decrease_lock, modifier_flag::left_command, device_id(0)),
                   active_modifier_flag(active_modifier_flag::type::decrease_sticky, modifier_flag::left_command, device_id(0)),
               }) == scoped_modifier_flags.get_scoped_active_modifier_flags());
      }

      expect(active_modifier_flags == modifier_flag_manager.get_active_modifier_flags());

      {
        scoped_modifier_flags scoped_modifier_flags(modifier_flag_manager, std::unordered_set<modifier_flag>{
                                                                               modifier_flag::left_command,
                                                                           });

        expect(std::unordered_set<modifier_flag>({
                   modifier_flag::left_command,
               }) == modifier_flag_manager.make_modifier_flags());

        std::cout << std::endl
                  << scoped_modifier_flags.get_scoped_active_modifier_flags()
                  << std::endl;

        expect(std::vector<active_modifier_flag>({
                   active_modifier_flag(active_modifier_flag::type::decrease_led_lock, modifier_flag::caps_lock, device_id(0)),
               }) == scoped_modifier_flags.get_scoped_active_modifier_flags());
      }

      expect(active_modifier_flags == modifier_flag_manager.get_active_modifier_flags());
    }

    //
    // Tests for type::decrease_led_lock
    //

    {
      modifier_flag_manager modifier_flag_manager;

      modifier_flag_manager.push_back_active_modifier_flag(active_modifier_flag(active_modifier_flag::type::decrease_led_lock, modifier_flag::caps_lock, device_id(1)));

      auto active_modifier_flags = modifier_flag_manager.get_active_modifier_flags();

      expect(std::unordered_set<modifier_flag>({}) == modifier_flag_manager.make_modifier_flags());

      {
        scoped_modifier_flags scoped_modifier_flags(modifier_flag_manager, std::unordered_set<modifier_flag>{});

        expect(std::unordered_set<modifier_flag>({}) == modifier_flag_manager.make_modifier_flags());

        std::cout << std::endl
                  << scoped_modifier_flags.get_scoped_active_modifier_flags()
                  << std::endl;

        expect(std::vector<active_modifier_flag>({}) == scoped_modifier_flags.get_scoped_active_modifier_flags());
      }

      expect(active_modifier_flags == modifier_flag_manager.get_active_modifier_flags());

      {
        scoped_modifier_flags scoped_modifier_flags(modifier_flag_manager, std::unordered_set<modifier_flag>{modifier_flag::caps_lock});

        expect(std::unordered_set<modifier_flag>({
                   modifier_flag::caps_lock,
               }) == modifier_flag_manager.make_modifier_flags());

        std::cout << std::endl
                  << scoped_modifier_flags.get_scoped_active_modifier_flags()
                  << std::endl;

        expect(std::vector<active_modifier_flag>({
                   active_modifier_flag(active_modifier_flag::type::increase, modifier_flag::caps_lock, device_id(0)),
               }) == scoped_modifier_flags.get_scoped_active_modifier_flags());
      }

      expect(active_modifier_flags == modifier_flag_manager.get_active_modifier_flags());
    }

    //
    // Increase caps_lock by type::increase
    //

    {
      modifier_flag_manager modifier_flag_manager;

      auto active_modifier_flags = modifier_flag_manager.get_active_modifier_flags();

      expect(std::unordered_set<modifier_flag>({}) == modifier_flag_manager.make_modifier_flags());

      {
        scoped_modifier_flags scoped_modifier_flags(modifier_flag_manager, std::unordered_set<modifier_flag>{modifier_flag::caps_lock});

        expect(std::unordered_set<modifier_flag>({
                   modifier_flag::caps_lock,
               }) == modifier_flag_manager.make_modifier_flags());

        std::cout << std::endl
                  << scoped_modifier_flags.get_scoped_active_modifier_flags()
                  << std::endl;

        expect(std::vector<active_modifier_flag>({
                   active_modifier_flag(active_modifier_flag::type::increase, modifier_flag::caps_lock, device_id(0)),
               }) == scoped_modifier_flags.get_scoped_active_modifier_flags());
      }

      expect(active_modifier_flags == modifier_flag_manager.get_active_modifier_flags());
    }
  };
}
