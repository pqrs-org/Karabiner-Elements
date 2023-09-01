#include "types.hpp"
#include <boost/ut.hpp>
#include <set>

void run_momentary_switch_event_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;
  using namespace std::literals;

  "momentary_switch_event"_test = [] {
    //
    // usage_page::keyboard_or_keypad
    //

    {
      krbn::momentary_switch_event e(pqrs::hid::usage_page::keyboard_or_keypad,
                                     pqrs::hid::usage::keyboard_or_keypad::keyboard_a);
      expect(e.make_modifier_flag() == std::nullopt);
      expect(e.modifier_flag() == false);
      expect(e.caps_lock() == false);
      expect(e.pointing_button() == false);
      expect(nlohmann::json(e).get<krbn::momentary_switch_event>() == e);
    }
    {
      krbn::momentary_switch_event e(pqrs::hid::usage_page::keyboard_or_keypad,
                                     pqrs::hid::usage::keyboard_or_keypad::keyboard_left_shift);
      expect(e.make_modifier_flag() == krbn::modifier_flag::left_shift);
      expect(e.modifier_flag() == true);
      expect(e.caps_lock() == false);
      expect(e.pointing_button() == false);
      expect(nlohmann::json(e).get<krbn::momentary_switch_event>() == e);
    }
    {
      krbn::momentary_switch_event e(pqrs::hid::usage_page::keyboard_or_keypad,
                                     pqrs::hid::usage::keyboard_or_keypad::keyboard_caps_lock);
      // Note: make_modifier_flag returns nullopt for caps_lock.
      expect(e.make_modifier_flag() == std::nullopt);
      expect(e.modifier_flag() == false);
      expect(e.caps_lock() == true);
      expect(e.pointing_button() == false);
      expect(nlohmann::json(e).get<krbn::momentary_switch_event>() == e);
    }
    {
      // Not target usage
      expect(!krbn::momentary_switch_event::target(pqrs::hid::usage_page::keyboard_or_keypad,
                                                   pqrs::hid::usage::keyboard_or_keypad::error_undefined));
    }

    //
    // usage_page::consumer
    //

    {
      krbn::momentary_switch_event e(pqrs::hid::usage_page::consumer,
                                     pqrs::hid::usage::consumer::mute);
      expect(e.make_modifier_flag() == std::nullopt);
      expect(e.modifier_flag() == false);
      expect(e.caps_lock() == false);
      expect(e.pointing_button() == false);
      expect(nlohmann::json(e).get<krbn::momentary_switch_event>() == e);
    }
    {
      // Not target usage
      expect(!krbn::momentary_switch_event::target(pqrs::hid::usage_page::consumer,
                                                   pqrs::hid::usage::consumer::consumer_control));
    }

    //
    // usage_page::apple_vendor_keyboard
    //

    {
      krbn::momentary_switch_event e(pqrs::hid::usage_page::apple_vendor_keyboard,
                                     pqrs::hid::usage::apple_vendor_keyboard::expose_all);
      expect(e.make_modifier_flag() == std::nullopt);
      expect(e.modifier_flag() == false);
      expect(e.caps_lock() == false);
      expect(e.pointing_button() == false);
      expect(nlohmann::json(e).get<krbn::momentary_switch_event>() == e);
    }
    {
      krbn::momentary_switch_event e(pqrs::hid::usage_page::apple_vendor_keyboard,
                                     pqrs::hid::usage::apple_vendor_keyboard::function);
      expect(e.make_modifier_flag() == krbn::modifier_flag::fn);
      expect(e.modifier_flag() == true);
      expect(e.caps_lock() == false);
      expect(e.pointing_button() == false);
      expect(nlohmann::json(e).get<krbn::momentary_switch_event>() == e);
    }
    {
      // Not target usage
      expect(!krbn::momentary_switch_event::target(pqrs::hid::usage_page::apple_vendor_keyboard,
                                                   pqrs::hid::usage::apple_vendor_keyboard::caps_lock_delay_enable));
    }

    //
    // usage_page::apple_vendor_top_case
    //

    {
      krbn::momentary_switch_event e(pqrs::hid::usage_page::apple_vendor_top_case,
                                     pqrs::hid::usage::apple_vendor_top_case::brightness_down);
      expect(e.make_modifier_flag() == std::nullopt);
      expect(e.modifier_flag() == false);
      expect(e.caps_lock() == false);
      expect(e.pointing_button() == false);
      expect(nlohmann::json(e).get<krbn::momentary_switch_event>() == e);
    }
    {
      krbn::momentary_switch_event e(pqrs::hid::usage_page::apple_vendor_top_case,
                                     pqrs::hid::usage::apple_vendor_top_case::keyboard_fn);
      expect(e.make_modifier_flag() == krbn::modifier_flag::fn);
      expect(e.modifier_flag() == true);
      expect(e.caps_lock() == false);
      expect(e.pointing_button() == false);
      expect(nlohmann::json(e).get<krbn::momentary_switch_event>() == e);
    }
    {
      // Not target usage
      expect(!krbn::momentary_switch_event::target(pqrs::hid::usage_page::apple_vendor_top_case,
                                                   pqrs::hid::usage::apple_vendor_top_case::clamshell_latched));
    }

    //
    // usage_page::button
    //

    {
      krbn::momentary_switch_event e(pqrs::hid::usage_page::button,
                                     pqrs::hid::usage::button::button_1);
      expect(e.make_modifier_flag() == std::nullopt);
      expect(e.modifier_flag() == false);
      expect(e.caps_lock() == false);
      expect(e.pointing_button() == true);
      expect(nlohmann::json(e).get<krbn::momentary_switch_event>() == e);
    }

    //
    // dpad
    //

    {
      krbn::momentary_switch_event e(pqrs::hid::usage_page::generic_desktop,
                                     pqrs::hid::usage::generic_desktop::dpad_up);
      expect(e.make_modifier_flag() == std::nullopt);
      expect(e.modifier_flag() == false);
      expect(e.caps_lock() == false);
      expect(e.pointing_button() == false);
      expect(nlohmann::json(e).get<krbn::momentary_switch_event>() == e);
    }

    //
    // map
    //

    {
      std::set<krbn::momentary_switch_event> map;
      map.insert(krbn::momentary_switch_event(pqrs::hid::usage_page::consumer,
                                              pqrs::hid::usage::consumer::mute));
      map.insert(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                              pqrs::hid::usage::keyboard_or_keypad::keyboard_b));
      map.insert(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                              pqrs::hid::usage::keyboard_or_keypad::keyboard_a));
      map.insert(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                              pqrs::hid::usage::keyboard_or_keypad::keyboard_c));
      map.insert(krbn::momentary_switch_event(pqrs::hid::usage_page::button,
                                              pqrs::hid::usage::button::button_2));
      map.insert(krbn::momentary_switch_event(pqrs::hid::usage_page::button,
                                              pqrs::hid::usage::button::button_1));
      map.insert(krbn::momentary_switch_event(pqrs::hid::usage_page::generic_desktop,
                                              pqrs::hid::usage::generic_desktop::dpad_up));

      int i = 0;
      for (const auto& m : map) {
        std::cout << nlohmann::json(m).dump() << std::endl;

        switch (i++) {
          case 0:
            expect(m == krbn::momentary_switch_event(pqrs::hid::usage_page::generic_desktop,
                                                     pqrs::hid::usage::generic_desktop::dpad_up));
            break;
          case 1:
            expect(m == krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                     pqrs::hid::usage::keyboard_or_keypad::keyboard_a));
            break;
          case 2:
            expect(m == krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                     pqrs::hid::usage::keyboard_or_keypad::keyboard_b));
            break;
          case 3:
            expect(m == krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                     pqrs::hid::usage::keyboard_or_keypad::keyboard_c));
            break;
          case 4:
            expect(m == krbn::momentary_switch_event(pqrs::hid::usage_page::button,
                                                     pqrs::hid::usage::button::button_1));
            break;
          case 5:
            expect(m == krbn::momentary_switch_event(pqrs::hid::usage_page::button,
                                                     pqrs::hid::usage::button::button_2));
            break;
          case 6:
            expect(m == krbn::momentary_switch_event(pqrs::hid::usage_page::consumer,
                                                     pqrs::hid::usage::consumer::mute));
            break;
        }
      }
    }
  };

  "momentary_switch_event json"_test = [] {
    //
    // key_code
    //

    {
      std::string expected("{\"key_code\":\"escape\"}");
      krbn::momentary_switch_event e(pqrs::hid::usage_page::keyboard_or_keypad,
                                     pqrs::hid::usage::keyboard_or_keypad::keyboard_escape);
      nlohmann::json actual = e;
      expect(actual.dump() == expected);
      expect(actual.get<krbn::momentary_switch_event>() == e);
    }
    {
      // Alias
      std::string expected("{\"key_code\":\"left_option\"}");
      krbn::momentary_switch_event e(pqrs::hid::usage_page::keyboard_or_keypad,
                                     pqrs::hid::usage::keyboard_or_keypad::keyboard_left_alt);
      nlohmann::json actual = e;
      expect(actual.dump() == expected);
      expect(actual.get<krbn::momentary_switch_event>() == e);
    }
    {
      // Other usage_page
      krbn::momentary_switch_event expected(pqrs::hid::usage_page::apple_vendor_keyboard,
                                            pqrs::hid::usage::apple_vendor_keyboard::expose_all);
      auto json = nlohmann::json::object({{"key_code", "mission_control"}});
      expect(json.get<krbn::momentary_switch_event>() == expected);
    }

    //
    // consumer_key_code
    //

    {
      std::string expected("{\"consumer_key_code\":\"mute\"}");
      krbn::momentary_switch_event e(pqrs::hid::usage_page::consumer,
                                     pqrs::hid::usage::consumer::mute);
      nlohmann::json actual = e;
      expect(actual.dump() == expected);
      expect(actual.get<krbn::momentary_switch_event>() == e);
    }

    //
    // apple_vendor_keyboard_key_code
    //

    {
      std::string expected("{\"apple_vendor_keyboard_key_code\":\"spotlight\"}");
      krbn::momentary_switch_event e(pqrs::hid::usage_page::apple_vendor_keyboard,
                                     pqrs::hid::usage::apple_vendor_keyboard::spotlight);
      nlohmann::json actual = e;
      expect(actual.dump() == expected);
      expect(actual.get<krbn::momentary_switch_event>() == e);
    }

    //
    // apple_vendor_top_case_key_code
    //

    {
      std::string expected("{\"apple_vendor_top_case_key_code\":\"brightness_down\"}");
      krbn::momentary_switch_event e(pqrs::hid::usage_page::apple_vendor_top_case,
                                     pqrs::hid::usage::apple_vendor_top_case::brightness_down);
      nlohmann::json actual = e;
      expect(actual.dump() == expected);
      expect(actual.get<krbn::momentary_switch_event>() == e);
    }

    //
    // pointing_button
    //

    {
      std::string expected("{\"pointing_button\":\"button1\"}");
      krbn::momentary_switch_event e(pqrs::hid::usage_page::button,
                                     pqrs::hid::usage::button::button_1);
      nlohmann::json actual = e;
      expect(actual.dump() == expected);
      expect(actual.get<krbn::momentary_switch_event>() == e);
    }

    //
    // dpad
    //

    {
      std::string expected("{\"dpad\":\"up\"}");
      krbn::momentary_switch_event e(pqrs::hid::usage_page::generic_desktop,
                                     pqrs::hid::usage::generic_desktop::dpad_up);
      nlohmann::json actual = e;
      expect(actual.dump() == expected);
      expect(actual.get<krbn::momentary_switch_event>() == e);
    }

    //
    // Unsupported
    //

    {
      krbn::momentary_switch_event e(pqrs::hid::usage_page::generic_desktop,
                                     pqrs::hid::usage::generic_desktop::hat_switch);
      nlohmann::json actual = e;
      expect("\"unsupported\""sv == actual.dump());
    }

    //
    // Number
    //

    {
      krbn::momentary_switch_event expected(pqrs::hid::usage_page::keyboard_or_keypad,
                                            pqrs::hid::usage::value_t(1234));
      auto json = nlohmann::json::object({{"key_code", 1234}});
      expect(json.get<krbn::momentary_switch_event>() == expected);
    }

    //
    // Not target
    //

    {

      krbn::momentary_switch_event e(pqrs::hid::usage_page::consumer,
                                     pqrs::hid::usage::consumer::ac_pan);
      nlohmann::json actual = e;
      expect("\"unsupported\""sv == actual.dump());
    }

    //
    // Errors
    //

    try {
      nlohmann::json json;
      [[maybe_unused]] auto event = krbn::momentary_switch_event(json);
      expect(false);
    } catch (pqrs::json::unmarshal_error& ex) {
      expect(std::string("json must be object, but is `null`") == ex.what());
    } catch (...) {
      expect(false);
    }

    try {
      auto json = nlohmann::json::object({{"key_code", "unknown_value"}});
      [[maybe_unused]] auto event = krbn::momentary_switch_event(json);
      expect(false);
    } catch (pqrs::json::unmarshal_error& ex) {
      expect(std::string("unknown key_code: `\"unknown_value\"`") == ex.what());
    } catch (...) {
      expect(false);
    }
  };

  "momentary_switch_event(modifier_flag)"_test = [] {
    expect(!krbn::momentary_switch_event(krbn::modifier_flag::zero).valid());

    expect(krbn::momentary_switch_event(krbn::modifier_flag::caps_lock).get_usage_pair() ==
           pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_caps_lock));

    expect(krbn::momentary_switch_event(krbn::modifier_flag::left_control).get_usage_pair() ==
           pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_left_control));

    expect(krbn::momentary_switch_event(krbn::modifier_flag::left_shift).get_usage_pair() ==
           pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_left_shift));

    expect(krbn::momentary_switch_event(krbn::modifier_flag::left_option).get_usage_pair() ==
           pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_left_alt));

    expect(krbn::momentary_switch_event(krbn::modifier_flag::left_command).get_usage_pair() ==
           pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_left_gui));

    expect(krbn::momentary_switch_event(krbn::modifier_flag::right_control).get_usage_pair() ==
           pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_right_control));

    expect(krbn::momentary_switch_event(krbn::modifier_flag::right_shift).get_usage_pair() ==
           pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_right_shift));

    expect(krbn::momentary_switch_event(krbn::modifier_flag::right_option).get_usage_pair() ==
           pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_right_alt));

    expect(krbn::momentary_switch_event(krbn::modifier_flag::right_command).get_usage_pair() ==
           pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_right_gui));

    expect(krbn::momentary_switch_event(krbn::modifier_flag::fn).get_usage_pair() ==
           pqrs::hid::usage_pair(pqrs::hid::usage_page::apple_vendor_top_case,
                                 pqrs::hid::usage::apple_vendor_top_case::keyboard_fn));

    expect(!krbn::momentary_switch_event(krbn::modifier_flag::end_).valid());
  };

  "momentary_switch_event::make_modifier_flag"_test = [] {
    expect(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                        pqrs::hid::usage::keyboard_or_keypad::keyboard_a)
               .make_modifier_flag() == std::nullopt);

    expect(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                        pqrs::hid::usage::keyboard_or_keypad::keyboard_caps_lock)
               .make_modifier_flag() == std::nullopt);

    expect(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                        pqrs::hid::usage::keyboard_or_keypad::keyboard_left_control)
               .make_modifier_flag() == krbn::modifier_flag::left_control);

    expect(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                        pqrs::hid::usage::keyboard_or_keypad::keyboard_left_shift)
               .make_modifier_flag() == krbn::modifier_flag::left_shift);

    expect(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                        pqrs::hid::usage::keyboard_or_keypad::keyboard_left_alt)
               .make_modifier_flag() == krbn::modifier_flag::left_option);

    expect(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                        pqrs::hid::usage::keyboard_or_keypad::keyboard_left_gui)
               .make_modifier_flag() == krbn::modifier_flag::left_command);

    expect(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                        pqrs::hid::usage::keyboard_or_keypad::keyboard_right_control)
               .make_modifier_flag() == krbn::modifier_flag::right_control);

    expect(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                        pqrs::hid::usage::keyboard_or_keypad::keyboard_right_shift)
               .make_modifier_flag() == krbn::modifier_flag::right_shift);

    expect(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                        pqrs::hid::usage::keyboard_or_keypad::keyboard_right_alt)
               .make_modifier_flag() == krbn::modifier_flag::right_option);

    expect(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                        pqrs::hid::usage::keyboard_or_keypad::keyboard_right_gui)
               .make_modifier_flag() == krbn::modifier_flag::right_command);

    expect(krbn::momentary_switch_event(pqrs::hid::usage_page::apple_vendor_keyboard,
                                        pqrs::hid::usage::apple_vendor_keyboard::function)
               .make_modifier_flag() == krbn::modifier_flag::fn);

    expect(krbn::momentary_switch_event(pqrs::hid::usage_page::apple_vendor_top_case,
                                        pqrs::hid::usage::apple_vendor_top_case::keyboard_fn)
               .make_modifier_flag() == krbn::modifier_flag::fn);
  };
}
