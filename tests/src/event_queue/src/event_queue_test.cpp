#include <catch2/catch.hpp>

#include "test.hpp"

namespace {
krbn::event_queue::event a_event(krbn::key_code::keyboard_a);
krbn::event_queue::event b_event(krbn::key_code::keyboard_b);
krbn::event_queue::event caps_lock_event(krbn::key_code::keyboard_caps_lock);
krbn::event_queue::event escape_event(krbn::key_code::keyboard_escape);
krbn::event_queue::event left_control_event(krbn::key_code::keyboard_left_control);
krbn::event_queue::event left_shift_event(krbn::key_code::keyboard_left_shift);
krbn::event_queue::event right_control_event(krbn::key_code::keyboard_right_control);
krbn::event_queue::event right_shift_event(krbn::key_code::keyboard_right_shift);
krbn::event_queue::event spacebar_event(krbn::key_code::keyboard_spacebar);
krbn::event_queue::event tab_event(krbn::key_code::keyboard_tab);

krbn::event_queue::event mute_event(krbn::consumer_key_code::mute);

krbn::event_queue::event button2_event(krbn::momentary_switch_event(pqrs::hid::usage_page::button,
                                                                    pqrs::hid::usage::button::button_2));

krbn::event_queue::event caps_lock_state_changed_1_event(krbn::event_queue::event::type::caps_lock_state_changed, 1);
krbn::event_queue::event caps_lock_state_changed_0_event(krbn::event_queue::event::type::caps_lock_state_changed, 0);

auto device_keys_and_pointing_buttons_are_released_event = krbn::event_queue::event::make_device_keys_and_pointing_buttons_are_released_event();
} // namespace

TEST_CASE("entry") {
  krbn::event_queue::entry entry1(krbn::device_id(1),
                                  krbn::event_queue::event_time_stamp(krbn::absolute_time_point(100),
                                                                      krbn::absolute_time_duration(10)),
                                  krbn::event_queue::event(krbn::key_code::keyboard_a),
                                  krbn::event_type::key_down,
                                  krbn::event_queue::event(krbn::key_code::keyboard_a),
                                  krbn::event_queue::state::original,
                                  false);
  auto entry2 = entry1;
  REQUIRE(entry1 == entry2);

  entry2.set_lazy(true);
  REQUIRE(entry1 != entry2);
}

TEST_CASE("json") {
  {
    nlohmann::json expected;
    expected["type"] = "key_code";
    expected["key_code"] = "a";
    auto json = a_event.to_json();
    REQUIRE(json == expected);
    auto event_from_json = krbn::event_queue::event::make_from_json(json);
    REQUIRE(json == event_from_json.to_json());
  }
  {
    nlohmann::json expected;
    expected["type"] = "consumer_key_code";
    expected["consumer_key_code"] = "mute";
    auto json = mute_event.to_json();
    REQUIRE(json == expected);
    auto event_from_json = krbn::event_queue::event::make_from_json(json);
    REQUIRE(json == event_from_json.to_json());
  }
  {
    auto expected = nlohmann::json::object({
        {"type", "momentary_switch_event"},
        {"momentary_switch_event", nlohmann::json::object({
                                       {"pointing_button", "button2"},
                                   })},
    });
    auto json = button2_event.to_json();
    REQUIRE(json == expected);
    auto event_from_json = krbn::event_queue::event::make_from_json(json);
    REQUIRE(json == event_from_json.to_json());
  }
  {
    nlohmann::json expected;
    expected["type"] = "pointing_motion";
    expected["pointing_motion"] = nlohmann::json::object();
    expected["pointing_motion"]["x"] = 1;
    expected["pointing_motion"]["y"] = 2;
    expected["pointing_motion"]["vertical_wheel"] = 3;
    expected["pointing_motion"]["horizontal_wheel"] = 4;

    krbn::pointing_motion pointing_motion;
    pointing_motion.set_x(1);
    pointing_motion.set_y(2);
    pointing_motion.set_vertical_wheel(3);
    pointing_motion.set_horizontal_wheel(4);

    auto e = krbn::event_queue::event(pointing_motion);

    auto json = e.to_json();
    REQUIRE(json == expected);
    auto event_from_json = krbn::event_queue::event::make_from_json(json);
    REQUIRE(json == event_from_json.to_json());
  }
  {
    nlohmann::json expected;
    expected["type"] = "caps_lock_state_changed";
    expected["caps_lock_state_changed"] = 1;
    auto json = caps_lock_state_changed_1_event.to_json();
    REQUIRE(json == expected);
    auto event_from_json = krbn::event_queue::event::make_from_json(json);
    REQUIRE(json == event_from_json.to_json());
  }
  {
    nlohmann::json expected;
    expected["type"] = "shell_command";
    expected["shell_command"] = "open https://pqrs.org";
    auto json = krbn::event_queue::event::make_shell_command_event("open https://pqrs.org").to_json();
    REQUIRE(json == expected);
    auto event_from_json = krbn::event_queue::event::make_from_json(json);
    REQUIRE(json == event_from_json.to_json());
  }
  {
    nlohmann::json expected;
    expected["type"] = "select_input_source";
    expected["input_source_specifiers"] = nlohmann::json::array();
    expected["input_source_specifiers"].push_back(nlohmann::json::object());
    expected["input_source_specifiers"].back()["language"] = "^en$";
    std::vector<pqrs::osx::input_source_selector::specifier> specifiers;
    pqrs::osx::input_source_selector::specifier s;
    s.set_language("^en$");
    specifiers.push_back(s);
    auto e = krbn::event_queue::event::make_select_input_source_event(specifiers);
    auto json = e.to_json();
    REQUIRE(json == expected);
    auto event_from_json = krbn::event_queue::event::make_from_json(json);
    REQUIRE(json == event_from_json.to_json());
  }
  {
    nlohmann::json expected;
    expected["type"] = "set_variable";
    expected["set_variable"] = nlohmann::json::array({"example1", 100});
    auto json = krbn::event_queue::event::make_set_variable_event(std::make_pair("example1", 100)).to_json();
    REQUIRE(json == expected);
    auto event_from_json = krbn::event_queue::event::make_from_json(json);
    REQUIRE(json == event_from_json.to_json());
  }
  {
    nlohmann::json expected;
    expected["type"] = "frontmost_application_changed";
    expected["frontmost_application"]["bundle_identifier"] = "com.apple.Terminal";
    expected["frontmost_application"]["file_path"] = "/Applications/Utilities/Terminal.app/Contents/MacOS/Terminal";
    pqrs::osx::frontmost_application_monitor::application application;
    application.set_bundle_identifier("com.apple.Terminal");
    application.set_file_path("/Applications/Utilities/Terminal.app/Contents/MacOS/Terminal");
    auto e = krbn::event_queue::event::make_frontmost_application_changed_event(application);
    auto json = e.to_json();
    REQUIRE(json == expected);
    auto event_from_json = krbn::event_queue::event::make_from_json(json);
    REQUIRE(json == event_from_json.to_json());
  }
  {
    nlohmann::json expected;
    expected["type"] = "input_source_changed";
    expected["input_source_properties"]["first_language"] = "en";
    expected["input_source_properties"]["input_source_id"] = "com.apple.keylayout.US";

    pqrs::osx::input_source::properties properties;
    properties.set_first_language("en");
    properties.set_input_source_id("com.apple.keylayout.US");
    auto e = krbn::event_queue::event::make_input_source_changed_event(properties);
    auto json = e.to_json();
    REQUIRE(json == expected);
    auto event_from_json = krbn::event_queue::event::make_from_json(json);
    REQUIRE(json == event_from_json.to_json());
  }
  {
    nlohmann::json expected;
    expected["type"] = "system_preferences_properties_changed";
    expected["system_preferences_properties"] = nlohmann::json::object({
        {"use_fkeys_as_standard_function_keys", false},
        {"scroll_direction_is_natural", true},
        {"keyboard_types", nlohmann::json::array()},
    });

    pqrs::osx::system_preferences::properties properties;
    properties.set_use_fkeys_as_standard_function_keys(false);
    properties.set_scroll_direction_is_natural(true);
    auto e = krbn::event_queue::event::make_system_preferences_properties_changed_event(properties);
    auto json = e.to_json();
    REQUIRE(json == expected);
    auto event_from_json = krbn::event_queue::event::make_from_json(json);
    REQUIRE(json == event_from_json.to_json());
  }
  {
    nlohmann::json expected;
    expected["type"] = "virtual_hid_keyboard_configuration_changed";
    expected["virtual_hid_keyboard_configuration"] = nlohmann::json::object({
        {"country_code", 123},
        {"mouse_key_xy_scale", 150},
    });

    krbn::core_configuration::details::virtual_hid_keyboard virtual_hid_keyboard;
    virtual_hid_keyboard.set_country_code(pqrs::hid::country_code::value_t(123));
    virtual_hid_keyboard.set_mouse_key_xy_scale(150);

    auto e = krbn::event_queue::event::make_virtual_hid_keyboard_configuration_changed_event(virtual_hid_keyboard);
    auto json = e.to_json();
    REQUIRE(json == expected);
    auto event_from_json = krbn::event_queue::event::make_from_json(json);
    REQUIRE(json == event_from_json.to_json());
  }
  {
    nlohmann::json expected;
    expected["type"] = "device_keys_and_pointing_buttons_are_released";
    auto json = device_keys_and_pointing_buttons_are_released_event.to_json();
    REQUIRE(json == expected);
    auto event_from_json = krbn::event_queue::event::make_from_json(json);
    REQUIRE(json == event_from_json.to_json());
  }
}

TEST_CASE("get_key_code") {
  REQUIRE(spacebar_event.get_key_code() == krbn::key_code::keyboard_spacebar);
  REQUIRE(button2_event.get_key_code() == std::nullopt);
  REQUIRE(caps_lock_state_changed_1_event.get_key_code() == std::nullopt);
  REQUIRE(caps_lock_state_changed_0_event.get_key_code() == std::nullopt);
  REQUIRE(device_keys_and_pointing_buttons_are_released_event.get_key_code() == std::nullopt);
}

TEST_CASE("get_consumer_key_code") {
  REQUIRE(spacebar_event.get_consumer_key_code() == std::nullopt);
  REQUIRE(mute_event.get_consumer_key_code() == krbn::consumer_key_code::mute);
}

TEST_CASE("get_frontmost_application_bundle_identifier") {
  REQUIRE(a_event.get_frontmost_application() == std::nullopt);

  {
    std::string bundle_identifier = "org.pqrs.example";
    std::string file_path = "/opt/bin/examle";
    pqrs::osx::frontmost_application_monitor::application application;
    application.set_bundle_identifier(bundle_identifier);
    application.set_file_path(file_path);
    auto e = krbn::event_queue::event::make_frontmost_application_changed_event(application);
    REQUIRE(e.get_frontmost_application() != std::nullopt);
    REQUIRE(e.get_frontmost_application()->get_bundle_identifier() == bundle_identifier);
    REQUIRE(e.get_frontmost_application()->get_file_path() == file_path);
  }
}

TEST_CASE("emplace_back_entry") {
  // Normal order
  {
    krbn::event_queue::queue event_queue;

    ENQUEUE_EVENT(event_queue, 1, 100, a_event, key_down, a_event);

    REQUIRE(event_queue.get_modifier_flag_manager().is_pressed(krbn::modifier_flag::left_shift) == false);

    ENQUEUE_EVENT(event_queue, 1, 200, left_shift_event, key_down, left_shift_event);

    REQUIRE(event_queue.get_modifier_flag_manager().is_pressed(krbn::modifier_flag::left_shift) == true);

    ENQUEUE_EVENT(event_queue, 1, 300, button2_event, key_down, button2_event);

    REQUIRE(event_queue.get_pointing_button_manager().is_pressed(
                pqrs::hid::usage_pair(
                    pqrs::hid::usage_page::button,
                    pqrs::hid::usage::button::button_2)) == true);

    ENQUEUE_EVENT(event_queue, 1, 400, left_shift_event, key_up, left_shift_event);

    REQUIRE(event_queue.get_modifier_flag_manager().is_pressed(krbn::modifier_flag::left_shift) == false);

    ENQUEUE_EVENT(event_queue, 1, 500, a_event, key_up, a_event);

    REQUIRE(event_queue.get_modifier_flag_manager().is_pressed(krbn::modifier_flag::left_shift) == false);

    ENQUEUE_EVENT(event_queue, 1, 600, button2_event, key_up, button2_event);

    REQUIRE(event_queue.get_pointing_button_manager().is_pressed(
                pqrs::hid::usage_pair(
                    pqrs::hid::usage_page::button,
                    pqrs::hid::usage::button::button_2)) == false);

    std::vector<krbn::event_queue::entry> expected;
    PUSH_BACK_ENTRY(expected, 1, 100, a_event, key_down, a_event);
    PUSH_BACK_ENTRY(expected, 1, 200, left_shift_event, key_down, left_shift_event);
    PUSH_BACK_ENTRY(expected, 1, 300, button2_event, key_down, button2_event);
    PUSH_BACK_ENTRY(expected, 1, 400, left_shift_event, key_up, left_shift_event);
    PUSH_BACK_ENTRY(expected, 1, 500, a_event, key_up, a_event);
    PUSH_BACK_ENTRY(expected, 1, 600, button2_event, key_up, button2_event);
    REQUIRE(event_queue.get_entries() == expected);

    REQUIRE(event_queue.get_entries()[0].get_valid() == true);
    REQUIRE(event_queue.get_entries()[0].get_lazy() == false);
  }

  // Reorder events
  {
    krbn::event_queue::queue event_queue;

    // Push `a, left_control, left_shift (key_down)` at the same time.

    ENQUEUE_EVENT(event_queue, 1, 100, left_control_event, key_down, left_control_event);
    ENQUEUE_EVENT(event_queue, 1, 100, a_event, key_down, a_event);
    ENQUEUE_EVENT(event_queue, 1, 100, left_shift_event, key_down, left_shift_event);
    ENQUEUE_EVENT(event_queue, 1, 200, spacebar_event, key_down, spacebar_event);

    // Push `a, left_control, left_shift (key_up)` at the same time.

    ENQUEUE_EVENT(event_queue, 1, 300, left_shift_event, key_up, left_shift_event);
    ENQUEUE_EVENT(event_queue, 1, 300, a_event, key_up, a_event);
    ENQUEUE_EVENT(event_queue, 1, 300, left_control_event, key_up, left_control_event);

    // Other events (not key_code) order are preserved.

    ENQUEUE_EVENT(event_queue, 1, 400, left_shift_event, key_down, left_shift_event);
    ENQUEUE_EVENT(event_queue, 1, 400, device_keys_and_pointing_buttons_are_released_event, single, device_keys_and_pointing_buttons_are_released_event);

    ENQUEUE_EVENT(event_queue, 1, 500, device_keys_and_pointing_buttons_are_released_event, single, device_keys_and_pointing_buttons_are_released_event);
    ENQUEUE_EVENT(event_queue, 1, 500, left_shift_event, key_down, left_shift_event);

    std::vector<krbn::event_queue::entry> expected;
    PUSH_BACK_ENTRY(expected, 1, 100, left_control_event, key_down, left_control_event);
    PUSH_BACK_ENTRY(expected, 1, 100, left_shift_event, key_down, left_shift_event);
    PUSH_BACK_ENTRY(expected, 1, 100, a_event, key_down, a_event);
    PUSH_BACK_ENTRY(expected, 1, 200, spacebar_event, key_down, spacebar_event);
    PUSH_BACK_ENTRY(expected, 1, 300, a_event, key_up, a_event);
    PUSH_BACK_ENTRY(expected, 1, 300, left_shift_event, key_up, left_shift_event);
    PUSH_BACK_ENTRY(expected, 1, 300, left_control_event, key_up, left_control_event);
    PUSH_BACK_ENTRY(expected, 1, 400, left_shift_event, key_down, left_shift_event);
    PUSH_BACK_ENTRY(expected, 1, 400, device_keys_and_pointing_buttons_are_released_event, single, device_keys_and_pointing_buttons_are_released_event);
    PUSH_BACK_ENTRY(expected, 1, 500, device_keys_and_pointing_buttons_are_released_event, single, device_keys_and_pointing_buttons_are_released_event);
    PUSH_BACK_ENTRY(expected, 1, 500, left_shift_event, key_down, left_shift_event);
    REQUIRE(event_queue.get_entries() == expected);
  }
  {
    krbn::event_queue::queue event_queue;

    ENQUEUE_EVENT(event_queue, 1, 100, a_event, key_down, a_event);
    ENQUEUE_EVENT(event_queue, 1, 100, left_shift_event, key_down, left_shift_event);
    ENQUEUE_EVENT(event_queue, 1, 100, b_event, key_down, b_event);
    ENQUEUE_EVENT(event_queue, 1, 100, left_control_event, key_down, left_control_event);

    std::vector<krbn::event_queue::entry> expected;
    PUSH_BACK_ENTRY(expected, 1, 100, left_shift_event, key_down, left_shift_event);
    PUSH_BACK_ENTRY(expected, 1, 100, left_control_event, key_down, left_control_event);
    PUSH_BACK_ENTRY(expected, 1, 100, a_event, key_down, a_event);
    PUSH_BACK_ENTRY(expected, 1, 100, b_event, key_down, b_event);
    REQUIRE(event_queue.get_entries() == expected);
  }
  {
    krbn::event_queue::queue event_queue;

    ENQUEUE_EVENT(event_queue, 1, 100, b_event, key_down, b_event);
    ENQUEUE_EVENT(event_queue, 1, 100, a_event, key_down, a_event);
    ENQUEUE_EVENT(event_queue, 1, 100, left_control_event, key_down, left_control_event);
    ENQUEUE_EVENT(event_queue, 1, 100, left_shift_event, key_down, left_shift_event);

    std::vector<krbn::event_queue::entry> expected;
    PUSH_BACK_ENTRY(expected, 1, 100, left_control_event, key_down, left_control_event);
    PUSH_BACK_ENTRY(expected, 1, 100, left_shift_event, key_down, left_shift_event);
    PUSH_BACK_ENTRY(expected, 1, 100, b_event, key_down, b_event);
    PUSH_BACK_ENTRY(expected, 1, 100, a_event, key_down, a_event);
    REQUIRE(event_queue.get_entries() == expected);
  }
  {
    krbn::event_queue::queue event_queue;

    ENQUEUE_EVENT(event_queue, 1, 100, b_event, key_up, b_event);
    ENQUEUE_EVENT(event_queue, 1, 100, a_event, key_up, a_event);
    ENQUEUE_EVENT(event_queue, 1, 100, left_control_event, key_down, left_control_event);
    ENQUEUE_EVENT(event_queue, 1, 100, left_shift_event, key_down, left_shift_event);

    std::vector<krbn::event_queue::entry> expected;
    PUSH_BACK_ENTRY(expected, 1, 100, left_control_event, key_down, left_control_event);
    PUSH_BACK_ENTRY(expected, 1, 100, left_shift_event, key_down, left_shift_event);
    PUSH_BACK_ENTRY(expected, 1, 100, b_event, key_up, b_event);
    PUSH_BACK_ENTRY(expected, 1, 100, a_event, key_up, a_event);
    REQUIRE(event_queue.get_entries() == expected);
  }
  {
    krbn::event_queue::queue event_queue;

    ENQUEUE_EVENT(event_queue, 1, 100, b_event, key_up, b_event);
    ENQUEUE_EVENT(event_queue, 1, 100, a_event, key_up, a_event);
    ENQUEUE_EVENT(event_queue, 1, 100, device_keys_and_pointing_buttons_are_released_event, single, device_keys_and_pointing_buttons_are_released_event);
    ENQUEUE_EVENT(event_queue, 1, 100, left_control_event, key_down, left_control_event);
    ENQUEUE_EVENT(event_queue, 1, 100, left_shift_event, key_down, left_shift_event);

    std::vector<krbn::event_queue::entry> expected;
    PUSH_BACK_ENTRY(expected, 1, 100, b_event, key_up, b_event);
    PUSH_BACK_ENTRY(expected, 1, 100, a_event, key_up, a_event);
    PUSH_BACK_ENTRY(expected, 1, 100, device_keys_and_pointing_buttons_are_released_event, single, device_keys_and_pointing_buttons_are_released_event);
    PUSH_BACK_ENTRY(expected, 1, 100, left_control_event, key_down, left_control_event);
    PUSH_BACK_ENTRY(expected, 1, 100, left_shift_event, key_down, left_shift_event);
    REQUIRE(event_queue.get_entries() == expected);
  }
}

TEST_CASE("needs_swap") {
  krbn::event_queue::entry spacebar_down(krbn::device_id(1),
                                         krbn::event_queue::event_time_stamp(
                                             krbn::absolute_time_point(100)),
                                         spacebar_event,
                                         krbn::event_type::key_down,
                                         spacebar_event,
                                         krbn::event_queue::state::original);
  krbn::event_queue::entry right_shift_down(krbn::device_id(1),
                                            krbn::event_queue::event_time_stamp(
                                                krbn::absolute_time_point(100)),
                                            right_shift_event,
                                            krbn::event_type::key_down,
                                            right_shift_event,
                                            krbn::event_queue::state::original);
  krbn::event_queue::entry escape_down(krbn::device_id(1),
                                       krbn::event_queue::event_time_stamp(
                                           krbn::absolute_time_point(200)),
                                       escape_event,
                                       krbn::event_type::key_down,
                                       escape_event,
                                       krbn::event_queue::state::original);
  krbn::event_queue::entry spacebar_up(krbn::device_id(1),
                                       krbn::event_queue::event_time_stamp(
                                           krbn::absolute_time_point(300)),
                                       spacebar_event,
                                       krbn::event_type::key_up,
                                       spacebar_event,
                                       krbn::event_queue::state::original);
  krbn::event_queue::entry right_shift_up(krbn::device_id(1),
                                          krbn::event_queue::event_time_stamp(
                                              krbn::absolute_time_point(300)),
                                          right_shift_event,
                                          krbn::event_type::key_up,
                                          right_shift_event,
                                          krbn::event_queue::state::original);

  REQUIRE(krbn::event_queue::queue::needs_swap(spacebar_down, spacebar_down) == false);
  REQUIRE(krbn::event_queue::queue::needs_swap(spacebar_down, escape_down) == false);
  REQUIRE(krbn::event_queue::queue::needs_swap(escape_down, spacebar_down) == false);

  REQUIRE(krbn::event_queue::queue::needs_swap(spacebar_down, right_shift_down) == true);
  REQUIRE(krbn::event_queue::queue::needs_swap(right_shift_down, spacebar_down) == false);

  REQUIRE(krbn::event_queue::queue::needs_swap(spacebar_down, right_shift_up) == false);
  REQUIRE(krbn::event_queue::queue::needs_swap(right_shift_up, spacebar_down) == false);

  REQUIRE(krbn::event_queue::queue::needs_swap(spacebar_up, right_shift_up) == false);
  REQUIRE(krbn::event_queue::queue::needs_swap(right_shift_up, spacebar_up) == true);

  REQUIRE(krbn::event_queue::queue::needs_swap(spacebar_up, right_shift_down) == false);
  REQUIRE(krbn::event_queue::queue::needs_swap(right_shift_down, spacebar_up) == false);
}

TEST_CASE("increase_time_stamp_delay") {
  {
    krbn::event_queue::queue event_queue;

    ENQUEUE_EVENT(event_queue, 1, 100, tab_event, key_down, tab_event);

    event_queue.increase_time_stamp_delay(krbn::absolute_time_duration(10));

    ENQUEUE_EVENT(event_queue, 1, 200, tab_event, key_up, tab_event);

    ENQUEUE_EVENT(event_queue, 1, 300, tab_event, key_down, tab_event);

    ENQUEUE_EVENT(event_queue, 1, 400, tab_event, key_up, tab_event);

    std::vector<krbn::event_queue::entry> expected;
    PUSH_BACK_ENTRY(expected, 1, 100, tab_event, key_down, tab_event);
    PUSH_BACK_ENTRY(expected, 1, 210, tab_event, key_up, tab_event);
    PUSH_BACK_ENTRY(expected, 1, 310, tab_event, key_down, tab_event);
    PUSH_BACK_ENTRY(expected, 1, 410, tab_event, key_up, tab_event);
    REQUIRE(event_queue.get_entries() == expected);

    while (!event_queue.empty()) {
      event_queue.erase_front_event();
    }
    REQUIRE(event_queue.get_time_stamp_delay() == krbn::absolute_time_duration(0));
  }
}

TEST_CASE("caps_lock_state_changed") {
  {
    krbn::event_queue::queue event_queue;

    // modifier_flag_manager's caps lock state will not be changed by key_down event.
    ENQUEUE_EVENT(event_queue, 1, 100, caps_lock_event, key_down, caps_lock_event);

    REQUIRE(event_queue.get_modifier_flag_manager().is_pressed(krbn::modifier_flag::caps_lock) == false);

    ENQUEUE_EVENT(event_queue, 1, 100, caps_lock_state_changed_1_event, single, caps_lock_state_changed_1_event);

    REQUIRE(event_queue.get_modifier_flag_manager().is_pressed(krbn::modifier_flag::caps_lock) == true);

    // Send twice

    ENQUEUE_EVENT(event_queue, 1, 100, caps_lock_state_changed_1_event, single, caps_lock_state_changed_1_event);

    REQUIRE(event_queue.get_modifier_flag_manager().is_pressed(krbn::modifier_flag::caps_lock) == true);

    ENQUEUE_EVENT(event_queue, 1, 100, caps_lock_state_changed_0_event, single, caps_lock_state_changed_0_event);

    REQUIRE(event_queue.get_modifier_flag_manager().is_pressed(krbn::modifier_flag::caps_lock) == false);

    ENQUEUE_EVENT(event_queue, 1, 100, caps_lock_state_changed_1_event, single, caps_lock_state_changed_1_event);

    REQUIRE(event_queue.get_modifier_flag_manager().is_pressed(krbn::modifier_flag::caps_lock) == true);
  }
}

TEST_CASE("hash") {
  using event = krbn::event_queue::event;
  REQUIRE(std::hash<event>{}(event(krbn::key_code::keyboard_a)) !=
          std::hash<event>{}(event(krbn::key_code::keyboard_b)));
}
