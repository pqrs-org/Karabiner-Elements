#include <catch2/catch.hpp>

#include "pressed_keys_manager.hpp"

TEST_CASE("pressed_keys_manager") {
  // empty

  {
    krbn::pressed_keys_manager manager;

    REQUIRE(manager.empty());
  }

  // key_code

  {
    krbn::pressed_keys_manager manager;

    manager.insert(krbn::momentary_switch_event(pqrs::hid::usage_pair(
        pqrs::hid::usage_page::keyboard_or_keypad,
        pqrs::hid::usage::keyboard_or_keypad::keyboard_a)));
    REQUIRE(!manager.empty());

    manager.erase(krbn::momentary_switch_event(pqrs::hid::usage_pair(
        pqrs::hid::usage_page::keyboard_or_keypad,
        pqrs::hid::usage::keyboard_or_keypad::keyboard_a)));
    REQUIRE(manager.empty());
  }

  // Duplicated key_code

  {
    krbn::pressed_keys_manager manager;

    manager.insert(krbn::momentary_switch_event(pqrs::hid::usage_pair(
        pqrs::hid::usage_page::keyboard_or_keypad,
        pqrs::hid::usage::keyboard_or_keypad::keyboard_a)));
    manager.insert(krbn::momentary_switch_event(pqrs::hid::usage_pair(
        pqrs::hid::usage_page::keyboard_or_keypad,
        pqrs::hid::usage::keyboard_or_keypad::keyboard_a)));
    REQUIRE(!manager.empty());

    manager.erase(krbn::momentary_switch_event(pqrs::hid::usage_pair(
        pqrs::hid::usage_page::keyboard_or_keypad,
        pqrs::hid::usage::keyboard_or_keypad::keyboard_a)));
    REQUIRE(manager.empty());
  }

  // consumer_key_code

  {
    krbn::pressed_keys_manager manager;

    manager.insert(krbn::momentary_switch_event(pqrs::hid::usage_pair(
        pqrs::hid::usage_page::consumer,
        pqrs::hid::usage::consumer::mute)));
    REQUIRE(!manager.empty());

    manager.erase(krbn::momentary_switch_event(pqrs::hid::usage_pair(
        pqrs::hid::usage_page::consumer,
        pqrs::hid::usage::consumer::mute)));
    REQUIRE(manager.empty());
  }

  // pointing_button

  {
    krbn::pressed_keys_manager manager;

    manager.insert(krbn::momentary_switch_event(pqrs::hid::usage_pair(
        pqrs::hid::usage_page::button,
        pqrs::hid::usage::button::button_1)));
    REQUIRE(!manager.empty());

    manager.erase(krbn::momentary_switch_event(pqrs::hid::usage_pair(
        pqrs::hid::usage_page::button,
        pqrs::hid::usage::button::button_1)));
    REQUIRE(manager.empty());
  }

  // combination

  {
    krbn::pressed_keys_manager manager;

    manager.insert(krbn::momentary_switch_event(pqrs::hid::usage_pair(
        pqrs::hid::usage_page::keyboard_or_keypad,
        pqrs::hid::usage::keyboard_or_keypad::keyboard_a)));
    REQUIRE(!manager.empty());

    manager.insert(krbn::momentary_switch_event(pqrs::hid::usage_pair(
        pqrs::hid::usage_page::keyboard_or_keypad,
        pqrs::hid::usage::keyboard_or_keypad::keyboard_a)));
    REQUIRE(!manager.empty());

    manager.insert(krbn::momentary_switch_event(pqrs::hid::usage_pair(
        pqrs::hid::usage_page::consumer,
        pqrs::hid::usage::consumer::mute)));
    REQUIRE(!manager.empty());

    manager.insert(krbn::momentary_switch_event(pqrs::hid::usage_pair(
        pqrs::hid::usage_page::button,
        pqrs::hid::usage::button::button_1)));
    REQUIRE(!manager.empty());

    manager.erase(krbn::momentary_switch_event(pqrs::hid::usage_pair(
        pqrs::hid::usage_page::keyboard_or_keypad,
        pqrs::hid::usage::keyboard_or_keypad::keyboard_a)));
    REQUIRE(!manager.empty());

    manager.erase(krbn::momentary_switch_event(pqrs::hid::usage_pair(
        pqrs::hid::usage_page::consumer,
        pqrs::hid::usage::consumer::mute)));
    REQUIRE(!manager.empty());

    manager.erase(krbn::momentary_switch_event(pqrs::hid::usage_pair(
        pqrs::hid::usage_page::button,
        pqrs::hid::usage::button::button_10)));
    REQUIRE(!manager.empty());

    manager.erase(krbn::momentary_switch_event(pqrs::hid::usage_pair(
        pqrs::hid::usage_page::button,
        pqrs::hid::usage::button::button_1)));
    REQUIRE(manager.empty());
  }
}
