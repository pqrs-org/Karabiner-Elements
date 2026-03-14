#include "keyboard_suppression.hpp"
#include <boost/ut.hpp>

int main(void) {
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
