#include "../../../src/core/grabber/include/grabber/device_grabber_details/probable_stuck_events_manager.hpp"
#include <boost/ut.hpp>

int main(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "probable_stuck_events_manager"_test = [] {
    krbn::probable_stuck_events_manager m;

    //
    // key_down, key_up
    //

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_a),
                    krbn::event_type::key_down,
                    krbn::device_state::seized) == false);
    expect(m.find_probable_stuck_event() == std::nullopt);

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_a),
                    krbn::event_type::key_up,
                    krbn::device_state::seized) == false);
    expect(m.find_probable_stuck_event() == std::nullopt);

    //
    // key_up, key_down, key_up, key_down, key_up
    //

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_b),
                    krbn::event_type::key_up,
                    krbn::device_state::seized) == true);
    expect(m.find_probable_stuck_event() ==
           krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                        pqrs::hid::usage::keyboard_or_keypad::keyboard_b));

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_b),
                    krbn::event_type::key_down,
                    krbn::device_state::seized) == false);
    expect(m.find_probable_stuck_event() ==
           krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                        pqrs::hid::usage::keyboard_or_keypad::keyboard_b));

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_b),
                    krbn::event_type::key_up,
                    krbn::device_state::seized) == true);
    expect(m.find_probable_stuck_event() == std::nullopt);

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_b),
                    krbn::event_type::key_down,
                    krbn::device_state::seized) == false);
    expect(m.find_probable_stuck_event() == std::nullopt);

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_b),
                    krbn::event_type::key_up,
                    krbn::device_state::seized) == false);
    expect(m.find_probable_stuck_event() == std::nullopt);

    //
    // clear
    //

    m.clear();
    expect(m.find_probable_stuck_event() == std::nullopt);

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_a),
                    krbn::event_type::key_up,
                    krbn::device_state::seized) == true);
    expect(m.find_probable_stuck_event() ==
           krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                        pqrs::hid::usage::keyboard_or_keypad::keyboard_a));
  };

  "probable_stuck_events_manager multiple stuck keys"_test = [] {
    krbn::probable_stuck_events_manager m;

    //
    // keyboard_b key_up
    //

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_b),
                    krbn::event_type::key_up,
                    krbn::device_state::seized) == true);
    expect(m.find_probable_stuck_event() == krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                         pqrs::hid::usage::keyboard_or_keypad::keyboard_b));

    //
    // keyboard_a key_up
    //

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_a),
                    krbn::event_type::key_up,
                    krbn::device_state::seized) == true);
    expect(m.find_probable_stuck_event() == krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                         pqrs::hid::usage::keyboard_or_keypad::keyboard_a));

    //
    // keyboard_b key_down, key_up
    //

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_b),
                    krbn::event_type::key_down,
                    krbn::device_state::seized) == false);
    expect(m.find_probable_stuck_event() == krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                         pqrs::hid::usage::keyboard_or_keypad::keyboard_a));

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_b),
                    krbn::event_type::key_up,
                    krbn::device_state::seized) == false);
    expect(m.find_probable_stuck_event() == krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                         pqrs::hid::usage::keyboard_or_keypad::keyboard_a));

    //
    // keyboard_a key_down, key_up
    //

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_a),
                    krbn::event_type::key_down,
                    krbn::device_state::seized) == false);
    expect(m.find_probable_stuck_event() == krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                         pqrs::hid::usage::keyboard_or_keypad::keyboard_a));

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_a),
                    krbn::event_type::key_up,
                    krbn::device_state::seized) == true);
    expect(m.find_probable_stuck_event() == std::nullopt);
  };

  "probable_stuck_events_manager key_down is dropped"_test = [] {
    krbn::probable_stuck_events_manager m;

    //
    // key_down, key_up, key_up
    //

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_a),
                    krbn::event_type::key_down,
                    krbn::device_state::seized) == false);
    expect(m.find_probable_stuck_event() == std::nullopt);

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_a),
                    krbn::event_type::key_up,
                    krbn::device_state::seized) == false);
    expect(m.find_probable_stuck_event() == std::nullopt);

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_a),
                    krbn::event_type::key_up,
                    krbn::device_state::seized) == false);
    expect(m.find_probable_stuck_event() == std::nullopt);
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
                      krbn::device_state::seized) == true);
      expect(m.find_probable_stuck_event() ==
             krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                          pqrs::hid::usage::keyboard_or_keypad::keyboard_a));

      expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                   pqrs::hid::usage::keyboard_or_keypad::keyboard_a),
                      krbn::event_type::key_up,
                      krbn::device_state::seized) == true);
      // Added to exceptional_key_up_events_
      expect(m.find_probable_stuck_event() == std::nullopt);

      expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                   pqrs::hid::usage::keyboard_or_keypad::keyboard_a),
                      krbn::event_type::key_up,
                      krbn::device_state::seized) == false);
      expect(m.find_probable_stuck_event() == std::nullopt);

      m.clear();

      expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                   pqrs::hid::usage::keyboard_or_keypad::keyboard_a),
                      krbn::event_type::key_up,
                      krbn::device_state::seized) == false);
      expect(m.find_probable_stuck_event() == std::nullopt);
    }
  };

  "probable_stuck_events_manager observed device"_test = [] {
    krbn::probable_stuck_events_manager m;

    //
    // keyboard_b key_up
    //

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_b),
                    krbn::event_type::key_up,
                    krbn::device_state::seized) == true);
    expect(m.find_probable_stuck_event() == krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                         pqrs::hid::usage::keyboard_or_keypad::keyboard_b));

    //
    // keyboard_a key_up
    //

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_a),
                    krbn::event_type::key_up,
                    krbn::device_state::seized) == true);
    expect(m.find_probable_stuck_event() == krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                         pqrs::hid::usage::keyboard_or_keypad::keyboard_a));

    //
    // keyboard_b key_down, key_up
    //

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_c),
                    krbn::event_type::key_down,
                    krbn::device_state::observed) == true);
    expect(m.find_probable_stuck_event() == krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                         pqrs::hid::usage::keyboard_or_keypad::keyboard_c));

    expect(m.update(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                 pqrs::hid::usage::keyboard_or_keypad::keyboard_c),
                    krbn::event_type::key_up,
                    krbn::device_state::observed) == true);
    expect(m.find_probable_stuck_event() == std::nullopt);
  };

  return 0;
}
