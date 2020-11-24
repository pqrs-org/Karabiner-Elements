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

    manager.insert(krbn::momentary_switch_event(krbn::key_code::keyboard_a));
    REQUIRE(!manager.empty());

    manager.erase(krbn::momentary_switch_event(krbn::key_code::keyboard_a));
    REQUIRE(manager.empty());
  }

  // Duplicated key_code

  {
    krbn::pressed_keys_manager manager;

    manager.insert(krbn::momentary_switch_event(krbn::key_code::keyboard_a));
    manager.insert(krbn::momentary_switch_event(krbn::key_code::keyboard_a));
    REQUIRE(!manager.empty());

    manager.erase(krbn::momentary_switch_event(krbn::key_code::keyboard_a));
    REQUIRE(manager.empty());
  }

  // consumer_key_code

  {
    krbn::pressed_keys_manager manager;

    manager.insert(krbn::momentary_switch_event(krbn::consumer_key_code::mute));
    REQUIRE(!manager.empty());

    manager.erase(krbn::momentary_switch_event(krbn::consumer_key_code::mute));
    REQUIRE(manager.empty());
  }

  // pointing_button

  {
    krbn::pressed_keys_manager manager;

    manager.insert(krbn::momentary_switch_event(krbn::pointing_button::button1));
    REQUIRE(!manager.empty());

    manager.erase(krbn::momentary_switch_event(krbn::pointing_button::button1));
    REQUIRE(manager.empty());
  }

  // combination

  {
    krbn::pressed_keys_manager manager;

    manager.insert(krbn::momentary_switch_event(krbn::key_code::keyboard_a));
    REQUIRE(!manager.empty());

    manager.insert(krbn::momentary_switch_event(krbn::key_code::keyboard_a));
    REQUIRE(!manager.empty());

    manager.insert(krbn::momentary_switch_event(krbn::consumer_key_code::mute));
    REQUIRE(!manager.empty());

    manager.insert(krbn::momentary_switch_event(krbn::pointing_button::button1));
    REQUIRE(!manager.empty());

    manager.erase(krbn::momentary_switch_event(krbn::key_code::keyboard_a));
    REQUIRE(!manager.empty());

    manager.erase(krbn::momentary_switch_event(krbn::consumer_key_code::mute));
    REQUIRE(!manager.empty());

    manager.erase(krbn::momentary_switch_event(krbn::pointing_button::button10));
    REQUIRE(!manager.empty());

    manager.erase(krbn::momentary_switch_event(krbn::pointing_button::button1));
    REQUIRE(manager.empty());
  }
}
