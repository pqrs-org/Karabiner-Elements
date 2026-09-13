#include "keyboard_suppression.hpp"
#include <boost/ut.hpp>

int main() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "consume_matched_entry"_test = [] {
    krbn::keyboard_suppression suppression;

    auto now = krbn::absolute_time_point(100);
    auto key = krbn::momentary_switch_event(
        pqrs::hid::usage_page::keyboard_or_keypad,
        pqrs::hid::usage::keyboard_or_keypad::keyboard_a);

    suppression.enqueue(key, krbn::event_type::key_down, now);

    expect(suppression.consume(key, krbn::event_type::key_down, now));
    expect(!suppression.consume(key, krbn::event_type::key_down, now));
  };

  "different_event_type_is_not_matched"_test = [] {
    krbn::keyboard_suppression suppression;

    auto now = krbn::absolute_time_point(100);
    auto key = krbn::momentary_switch_event(
        pqrs::hid::usage_page::keyboard_or_keypad,
        pqrs::hid::usage::keyboard_or_keypad::keyboard_a);

    suppression.enqueue(key, krbn::event_type::key_down, now);

    expect(!suppression.consume(key, krbn::event_type::key_up, now));
    expect(suppression.consume(key, krbn::event_type::key_down, now));
  };

  "delayed_caps_lock_activation_is_suppressed_once"_test = [] {
    krbn::keyboard_suppression suppression;
    auto now = krbn::absolute_time_point(100);
    auto key = krbn::momentary_switch_event(krbn::modifier_flag::caps_lock);

    suppression.enqueue(key, krbn::event_type::key_down, now);

    // Reproduce the observed delay that exceeded the former 50 ms default TTL.
    auto arrival = now + pqrs::osx::chrono::make_absolute_time_duration(std::chrono::milliseconds(87));
    expect(suppression.consume(key, krbn::event_type::key_down, arrival));
    expect(!suppression.consume(key, krbn::event_type::key_down, arrival));
  };

  "brief_caps_lock_press_without_cgevents_expires"_test = [] {
    krbn::keyboard_suppression suppression;
    auto now = krbn::absolute_time_point(100);
    auto key = krbn::momentary_switch_event(krbn::modifier_flag::caps_lock);
    auto released = now + pqrs::osx::chrono::make_absolute_time_duration(std::chrono::milliseconds(40));

    suppression.enqueue(key, krbn::event_type::key_down, now);
    suppression.enqueue(key, krbn::event_type::key_up, released);

    // No CGEvent arrives. The key_up must neither register another entry nor
    // extend the original key_down's lifetime.
    auto expired = now + pqrs::osx::chrono::make_absolute_time_duration(std::chrono::milliseconds(200));
    expect(!suppression.consume(key, krbn::event_type::key_down, expired));
    expect(!suppression.consume(key, krbn::event_type::key_up, expired));
  };

  "caps_lock_state_changes_match_either_direction_once_per_press"_test = [] {
    // Cover both state changes, arriving before or after the physical release.
    for (auto notification : {krbn::event_type::key_down, krbn::event_type::key_up}) {
      for (bool released_first : {false, true}) {
        krbn::keyboard_suppression suppression;
        auto now = krbn::absolute_time_point(100);
        auto key = krbn::momentary_switch_event(krbn::modifier_flag::caps_lock);
        auto later = now + pqrs::osx::chrono::make_absolute_time_duration(std::chrono::milliseconds(10));

        suppression.enqueue(key, krbn::event_type::key_down, now);
        if (released_first) {
          suppression.enqueue(key, krbn::event_type::key_up, now);
        }
        expect(suppression.consume(key, notification, later));
        if (!released_first) {
          suppression.enqueue(key, krbn::event_type::key_up, later);
        }
        expect(!suppression.consume(key, krbn::event_type::key_down, later));
        expect(!suppression.consume(key, krbn::event_type::key_up, later));
      }
    }
  };

  "caps_lock_does_not_match_single_or_other_keys"_test = [] {
    krbn::keyboard_suppression suppression;
    auto now = krbn::absolute_time_point(100);
    auto caps_lock = krbn::momentary_switch_event(krbn::modifier_flag::caps_lock);
    auto shift = krbn::momentary_switch_event(krbn::modifier_flag::left_shift);

    suppression.enqueue(caps_lock, krbn::event_type::key_up, now);
    suppression.enqueue(caps_lock, krbn::event_type::single, now);
    expect(!suppression.consume(caps_lock, krbn::event_type::key_up, now));
    expect(!suppression.consume(caps_lock, krbn::event_type::single, now));

    suppression.enqueue(caps_lock, krbn::event_type::key_down, now);
    expect(!suppression.consume(caps_lock, krbn::event_type::single, now));
    expect(!suppression.consume(shift, krbn::event_type::key_up, now));
    expect(suppression.consume(caps_lock, krbn::event_type::key_up, now));
  };

  "successive_caps_lock_presses_each_suppress_one_notification"_test = [] {
    krbn::keyboard_suppression suppression;
    auto now = krbn::absolute_time_point(100);
    auto key = krbn::momentary_switch_event(krbn::modifier_flag::caps_lock);

    for (auto notification : {krbn::event_type::key_down, krbn::event_type::key_up, krbn::event_type::key_down}) {
      suppression.enqueue(key, krbn::event_type::key_down, now);
      expect(suppression.consume(key, notification, now));
      suppression.enqueue(key, krbn::event_type::key_up, now);
      expect(!suppression.consume(key, notification, now));
      now += pqrs::osx::chrono::make_absolute_time_duration(std::chrono::milliseconds(20));
    }
  };

  "expired_entry_is_not_matched"_test = [] {
    krbn::keyboard_suppression suppression(std::chrono::milliseconds(50), 1024);

    auto now = krbn::absolute_time_point(100);
    auto key = krbn::momentary_switch_event(
        pqrs::hid::usage_page::keyboard_or_keypad,
        pqrs::hid::usage::keyboard_or_keypad::keyboard_a);

    suppression.enqueue(key, krbn::event_type::key_down, now);

    auto expired = now + pqrs::osx::chrono::make_absolute_time_duration(std::chrono::milliseconds(60));
    expect(!suppression.consume(key, krbn::event_type::key_down, expired));
  };

  "max_size_drops_oldest_entries"_test = [] {
    krbn::keyboard_suppression suppression(std::chrono::milliseconds(1000), 1);

    auto now = krbn::absolute_time_point(100);
    auto key_a = krbn::momentary_switch_event(
        pqrs::hid::usage_page::keyboard_or_keypad,
        pqrs::hid::usage::keyboard_or_keypad::keyboard_a);
    auto key_b = krbn::momentary_switch_event(
        pqrs::hid::usage_page::keyboard_or_keypad,
        pqrs::hid::usage::keyboard_or_keypad::keyboard_b);

    suppression.enqueue(key_a, krbn::event_type::key_down, now);
    suppression.enqueue(key_b, krbn::event_type::key_down, now);

    expect(!suppression.consume(key_a, krbn::event_type::key_down, now));
    expect(suppression.consume(key_b, krbn::event_type::key_down, now));
  };

  "invalid_or_pointing_event_is_ignored"_test = [] {
    krbn::keyboard_suppression suppression;

    auto now = krbn::absolute_time_point(100);
    auto invalid = krbn::momentary_switch_event();
    auto pointing = krbn::momentary_switch_event(
        pqrs::hid::usage_page::button,
        pqrs::hid::usage::button::button_1);

    suppression.enqueue(invalid, krbn::event_type::key_down, now);
    suppression.enqueue(pointing, krbn::event_type::key_down, now);

    expect(!suppression.consume(invalid, krbn::event_type::key_down, now));
    expect(!suppression.consume(pointing, krbn::event_type::key_down, now));
  };

  return 0;
}
