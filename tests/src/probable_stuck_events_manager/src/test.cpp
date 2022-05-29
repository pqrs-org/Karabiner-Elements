#include "probable_stuck_events_manager.hpp"
#include <boost/ut.hpp>

int main(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "probable_stuck_events_manager"_test = [] {
    krbn::probable_stuck_events_manager m;

    // ----------------------------------------
    // key_down, key_up

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_a),
                    krbn::event_type::key_down,
                    krbn::absolute_time_point(1000),
                    krbn::device_state::grabbed) == false);
    expect(m.find_probable_stuck_event() == std::nullopt);

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_a),
                    krbn::event_type::key_up,
                    krbn::absolute_time_point(1010),
                    krbn::device_state::grabbed) == false);
    expect(m.find_probable_stuck_event() == std::nullopt);

    // ----------------------------------------
    // key_up, key_down, key_up, key_down, key_up

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_b),
                    krbn::event_type::key_up,
                    krbn::absolute_time_point(1100),
                    krbn::device_state::grabbed) == true);
    expect(m.find_probable_stuck_event() ==
           krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                        pqrs::hid::usage::keyboard_or_keypad::keyboard_b));

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_b),
                    krbn::event_type::key_down,
                    krbn::absolute_time_point(1110),
                    krbn::device_state::grabbed) == false);
    expect(m.find_probable_stuck_event() ==
           krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                        pqrs::hid::usage::keyboard_or_keypad::keyboard_b));

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_b),
                    krbn::event_type::key_up,
                    krbn::absolute_time_point(1120),
                    krbn::device_state::grabbed) == true);
    expect(m.find_probable_stuck_event() == std::nullopt);

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_b),
                    krbn::event_type::key_down,
                    krbn::absolute_time_point(1130),
                    krbn::device_state::grabbed) == false);
    expect(m.find_probable_stuck_event() == std::nullopt);

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_b),
                    krbn::event_type::key_up,
                    krbn::absolute_time_point(1140),
                    krbn::device_state::grabbed) == false);
    expect(m.find_probable_stuck_event() == std::nullopt);

    // ----------------------------------------
    // ignore old event

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_c),
                    krbn::event_type::key_up,
                    krbn::absolute_time_point(900),
                    krbn::device_state::grabbed) == false);
    expect(m.find_probable_stuck_event() == std::nullopt);

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_c),
                    krbn::event_type::key_up,
                    krbn::absolute_time_point(1200),
                    krbn::device_state::grabbed) == true);
    expect(m.find_probable_stuck_event() ==
           krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                        pqrs::hid::usage::keyboard_or_keypad::keyboard_c));

    // ----------------------------------------
    // clear

    m.clear();
    expect(m.find_probable_stuck_event() == std::nullopt);

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_a),
                    krbn::event_type::key_up,
                    krbn::absolute_time_point(500),
                    krbn::device_state::grabbed) == true);
    expect(m.find_probable_stuck_event() ==
           krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                        pqrs::hid::usage::keyboard_or_keypad::keyboard_a));
  };

  "probable_stuck_events_manager key_down is dropped"_test = [] {
    krbn::probable_stuck_events_manager m;

    // ----------------------------------------
    // key_down, key_up, key_up

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_a),
                    krbn::event_type::key_down,
                    krbn::absolute_time_point(1000),
                    krbn::device_state::grabbed) == false);
    expect(m.find_probable_stuck_event() == std::nullopt);

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_a),
                    krbn::event_type::key_up,
                    krbn::absolute_time_point(1010),
                    krbn::device_state::grabbed) == false);
    expect(m.find_probable_stuck_event() == std::nullopt);

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_a),
                    krbn::event_type::key_up,
                    krbn::absolute_time_point(1020),
                    krbn::device_state::grabbed) == false);
    expect(m.find_probable_stuck_event() == std::nullopt);
  };

  "probable_stuck_events_manager time_stamp rewind"_test = [] {
    //
    // key_down, key_up, key_up (normal time_stamp)
    //

    {
      krbn::probable_stuck_events_manager m;

      expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                   pqrs::hid::usage::keyboard_or_keypad::keyboard_a),
                      krbn::event_type::key_down,
                      krbn::absolute_time_point(1000),
                      krbn::device_state::grabbed) == false);
      expect(m.find_probable_stuck_event() == std::nullopt);

      expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                   pqrs::hid::usage::keyboard_or_keypad::keyboard_b),
                      krbn::event_type::key_up,
                      krbn::absolute_time_point(1010),
                      krbn::device_state::grabbed) == true);
      expect(m.find_probable_stuck_event() ==
             krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                          pqrs::hid::usage::keyboard_or_keypad::keyboard_b));

      expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                   pqrs::hid::usage::keyboard_or_keypad::keyboard_c),
                      krbn::event_type::key_up,
                      krbn::absolute_time_point(1020),
                      krbn::device_state::grabbed) == true);
      expect(m.find_probable_stuck_event() ==
             krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                          pqrs::hid::usage::keyboard_or_keypad::keyboard_b));
    }

    //
    // key_down, key_up, key_up (rewind time_stamp)
    //

    {
      krbn::probable_stuck_events_manager m;

      expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                   pqrs::hid::usage::keyboard_or_keypad::keyboard_a),
                      krbn::event_type::key_down,
                      krbn::absolute_time_point(1000),
                      krbn::device_state::grabbed) == false);
      expect(m.find_probable_stuck_event() == std::nullopt);

      expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                   pqrs::hid::usage::keyboard_or_keypad::keyboard_b),
                      krbn::event_type::key_up,
                      krbn::absolute_time_point(910),
                      krbn::device_state::grabbed) == false);
      expect(m.find_probable_stuck_event() == std::nullopt);

      expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                   pqrs::hid::usage::keyboard_or_keypad::keyboard_c),
                      krbn::event_type::key_up,
                      krbn::absolute_time_point(920),
                      krbn::device_state::grabbed) == false);
      expect(m.find_probable_stuck_event() == std::nullopt);
    }
  };

  "probable_stuck_events_manager key_up only"_test = [] {
    //
    // key_up, key_up, key_up
    //

    {
      krbn::probable_stuck_events_manager m;

      expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                   pqrs::hid::usage::keyboard_or_keypad::keyboard_a),
                      krbn::event_type::key_up,
                      krbn::absolute_time_point(1000),
                      krbn::device_state::grabbed) == true);
      expect(m.find_probable_stuck_event() ==
             krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                          pqrs::hid::usage::keyboard_or_keypad::keyboard_a));

      expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                   pqrs::hid::usage::keyboard_or_keypad::keyboard_a),
                      krbn::event_type::key_up,
                      krbn::absolute_time_point(1010),
                      krbn::device_state::grabbed) == true);
      expect(m.find_probable_stuck_event() == std::nullopt);

      expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                   pqrs::hid::usage::keyboard_or_keypad::keyboard_a),
                      krbn::event_type::key_up,
                      krbn::absolute_time_point(1020),
                      krbn::device_state::grabbed) == false);
      expect(m.find_probable_stuck_event() == std::nullopt);

      m.clear();

      expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                   pqrs::hid::usage::keyboard_or_keypad::keyboard_a),
                      krbn::event_type::key_up,
                      krbn::absolute_time_point(1030),
                      krbn::device_state::grabbed) == false);
      expect(m.find_probable_stuck_event() == std::nullopt);
    }
  };

  return 0;
}
