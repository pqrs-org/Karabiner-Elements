#include "../../../src/core/CoreService/include/core_service/device_grabber_details/fn_function_keys_manipulator_manager.hpp"
#include "manipulator/manipulator_managers_connector.hpp"
#include "manipulator/manipulators/post_event_to_virtual_devices/post_event_to_virtual_devices.hpp"
#include <boost/ut.hpp>

int main(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;
  using namespace std::string_literals;
  using namespace std::literals::string_view_literals;

  "fn_function_keys_manipulator_manager"_test = [] {
    auto device_id_1 = krbn::device_id(1);
    auto state_original = krbn::event_queue::state::original;
    auto state_virtual = krbn::event_queue::state::virtual_event;

    auto f1 = krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                    pqrs::hid::usage::keyboard_or_keypad::keyboard_f1));
    auto f2 = krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                    pqrs::hid::usage::keyboard_or_keypad::keyboard_f2));
    auto f3 = krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                    pqrs::hid::usage::keyboard_or_keypad::keyboard_f3));
    auto fn = krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::apple_vendor_top_case,
                                                                    pqrs::hid::usage::apple_vendor_top_case::keyboard_fn));
    auto mission_control = krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::apple_vendor_keyboard,
                                                                                 pqrs::hid::usage::apple_vendor_keyboard::expose_all));
    auto spacebar = krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                          pqrs::hid::usage::keyboard_or_keypad::keyboard_spacebar));
    auto released_event = krbn::event_queue::event::make_device_keys_and_pointing_buttons_are_released_event();

    auto manager = std::make_shared<krbn::core_service::device_grabber_details::fn_function_keys_manipulator_manager>();

    auto console_user_server_client = std::make_shared<krbn::console_user_server_client>(geteuid(),
                                                                                         std::nullopt);
    auto notification_message_manager = std::make_shared<krbn::notification_message_manager>();
    auto post_event_to_virtual_devices_manipulator = std::make_shared<krbn::manipulator::manipulators::post_event_to_virtual_devices::post_event_to_virtual_devices>(
        console_user_server_client,
        notification_message_manager);
    auto post_event_to_virtual_devices_manipulator_manager = std::make_shared<krbn::manipulator::manipulator_manager>();
    post_event_to_virtual_devices_manipulator_manager->push_back_manipulator(post_event_to_virtual_devices_manipulator);

    auto core_configuration = std::make_shared<krbn::core_configuration::core_configuration>();
    auto fn_function_keys = core_configuration->get_selected_profile().get_fn_function_keys();

    // To media keys
    fn_function_keys->replace_pair(0,
                                   R"( { "key_code": "f1" } )",
                                   R"( [ { "apple_vendor_keyboard_key_code" : "mission_control" } ] )");

    // To function keys
    fn_function_keys->replace_pair(1,
                                   R"( { "key_code": "f2" } )",
                                   R"( [ { "key_code" : "f3" } ] )");

    // To standard keys
    fn_function_keys->replace_pair(2,
                                   R"( { "key_code": "f3" } )",
                                   R"( [ { "key_code" : "spacebar" } ] )");

    //
    // input events
    //

    auto input_event_queue = std::make_shared<krbn::event_queue::queue>();

    // f1 key_down
    auto t = krbn::absolute_time_point(1);
    input_event_queue->emplace_back_entry(device_id_1,
                                          krbn::event_queue::event_time_stamp(t),
                                          f1,
                                          krbn::event_type::key_down,
                                          krbn::event_integer_value::value_t(1),
                                          f1,
                                          state_original);

    // f1 key_up
    t += pqrs::osx::chrono::make_absolute_time_duration(std::chrono::milliseconds(1));
    input_event_queue->emplace_back_entry(device_id_1,
                                          krbn::event_queue::event_time_stamp(t),
                                          f1,
                                          krbn::event_type::key_up,
                                          krbn::event_integer_value::value_t(0),
                                          f1,
                                          state_original);

    t += pqrs::osx::chrono::make_absolute_time_duration(std::chrono::milliseconds(1));
    input_event_queue->emplace_back_entry(device_id_1,
                                          krbn::event_queue::event_time_stamp(t),
                                          released_event,
                                          krbn::event_type::single,
                                          std::nullopt,
                                          released_event,
                                          state_virtual);

    // f2 key_down
    t += pqrs::osx::chrono::make_absolute_time_duration(std::chrono::milliseconds(1));
    input_event_queue->emplace_back_entry(device_id_1,
                                          krbn::event_queue::event_time_stamp(t),
                                          f2,
                                          krbn::event_type::key_down,
                                          krbn::event_integer_value::value_t(1),
                                          f2,
                                          state_original);

    // f2 key_up
    t += pqrs::osx::chrono::make_absolute_time_duration(std::chrono::milliseconds(1));
    input_event_queue->emplace_back_entry(device_id_1,
                                          krbn::event_queue::event_time_stamp(t),
                                          f2,
                                          krbn::event_type::key_up,
                                          krbn::event_integer_value::value_t(0),
                                          f2,
                                          state_original);

    // f3 key_down
    t += pqrs::osx::chrono::make_absolute_time_duration(std::chrono::milliseconds(1));
    input_event_queue->emplace_back_entry(device_id_1,
                                          krbn::event_queue::event_time_stamp(t),
                                          f3,
                                          krbn::event_type::key_down,
                                          krbn::event_integer_value::value_t(1),
                                          f3,
                                          state_original);

    // f3 key_up
    t += pqrs::osx::chrono::make_absolute_time_duration(std::chrono::milliseconds(1));
    input_event_queue->emplace_back_entry(device_id_1,
                                          krbn::event_queue::event_time_stamp(t),
                                          f3,
                                          krbn::event_type::key_up,
                                          krbn::event_integer_value::value_t(0),
                                          f3,
                                          state_original);

    // fn key_down
    t += pqrs::osx::chrono::make_absolute_time_duration(std::chrono::milliseconds(1));
    input_event_queue->emplace_back_entry(device_id_1,
                                          krbn::event_queue::event_time_stamp(t),
                                          fn,
                                          krbn::event_type::key_down,
                                          krbn::event_integer_value::value_t(1),
                                          fn,
                                          state_original);

    // fn+f1 key_down
    t += pqrs::osx::chrono::make_absolute_time_duration(std::chrono::milliseconds(1));
    input_event_queue->emplace_back_entry(device_id_1,
                                          krbn::event_queue::event_time_stamp(t),
                                          f1,
                                          krbn::event_type::key_down,
                                          krbn::event_integer_value::value_t(1),
                                          f1,
                                          state_original);

    // fn+f1 key_up
    t += pqrs::osx::chrono::make_absolute_time_duration(std::chrono::milliseconds(1));
    input_event_queue->emplace_back_entry(device_id_1,
                                          krbn::event_queue::event_time_stamp(t),
                                          f1,
                                          krbn::event_type::key_up,
                                          krbn::event_integer_value::value_t(0),
                                          f1,
                                          state_original);

    // fn+f2 key_down
    t += pqrs::osx::chrono::make_absolute_time_duration(std::chrono::milliseconds(1));
    input_event_queue->emplace_back_entry(device_id_1,
                                          krbn::event_queue::event_time_stamp(t),
                                          f2,
                                          krbn::event_type::key_down,
                                          krbn::event_integer_value::value_t(1),
                                          f2,
                                          state_original);

    // fn+f2 key_up
    t += pqrs::osx::chrono::make_absolute_time_duration(std::chrono::milliseconds(1));
    input_event_queue->emplace_back_entry(device_id_1,
                                          krbn::event_queue::event_time_stamp(t),
                                          f2,
                                          krbn::event_type::key_up,
                                          krbn::event_integer_value::value_t(0),
                                          f2,
                                          state_original);

    // fn+f3 key_down
    t += pqrs::osx::chrono::make_absolute_time_duration(std::chrono::milliseconds(1));
    input_event_queue->emplace_back_entry(device_id_1,
                                          krbn::event_queue::event_time_stamp(t),
                                          f3,
                                          krbn::event_type::key_down,
                                          krbn::event_integer_value::value_t(1),
                                          f3,
                                          state_original);

    // fn+f3 key_up
    t += pqrs::osx::chrono::make_absolute_time_duration(std::chrono::milliseconds(1));
    input_event_queue->emplace_back_entry(device_id_1,
                                          krbn::event_queue::event_time_stamp(t),
                                          f3,
                                          krbn::event_type::key_up,
                                          krbn::event_integer_value::value_t(0),
                                          f3,
                                          state_original);

    // fn key_up
    t += pqrs::osx::chrono::make_absolute_time_duration(std::chrono::milliseconds(1));
    input_event_queue->emplace_back_entry(device_id_1,
                                          krbn::event_queue::event_time_stamp(t),
                                          fn,
                                          krbn::event_type::key_up,
                                          krbn::event_integer_value::value_t(0),
                                          fn,
                                          state_original);

    //
    // use_fkeys_as_standard_function_keys: false
    // (f1 -> mission_control, fn+f1 -> f1)
    //

    {
      pqrs::osx::system_preferences::properties properties;
      properties.set_use_fkeys_as_standard_function_keys(false);

      manager->update(core_configuration->get_selected_profile(), properties);

      auto input_event_queue_copy = std::make_shared<krbn::event_queue::queue>();
      auto intermediate_event_queue = std::make_shared<krbn::event_queue::queue>();
      auto output_event_queue = std::make_shared<krbn::event_queue::queue>();

      auto connector = std::make_shared<krbn::manipulator::manipulator_managers_connector>();
      connector->emplace_back_connection(manager->get_manipulator_manager(),
                                         input_event_queue_copy,
                                         intermediate_event_queue);
      connector->emplace_back_connection(post_event_to_virtual_devices_manipulator_manager,
                                         output_event_queue);

      //
      // manipulate
      //

      for (const auto& e : input_event_queue->get_entries()) {
        input_event_queue_copy->push_back_entry(e);
        connector->manipulate(e.get_event_time_stamp().get_time_stamp(),
                              core_configuration);
      }

      auto events = nlohmann::json(post_event_to_virtual_devices_manipulator->get_queue().get_events());
      // std::cout << events << std::endl;

      // f1 -> mission_control
      auto index = 0;
      expect("apple_vendor_keyboard_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [{ "apple_vendor_keyboard_key_code": "mission_control" }] )"_json == events[index][events[index]["type"]]["keys"]);

      ++index;
      expect("apple_vendor_keyboard_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [] )"_json == events[index][events[index]["type"]]["keys"]);

      // f2 -> f3
      ++index;
      expect("apple_vendor_top_case_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [{ "apple_vendor_top_case_key_code": "keyboard_fn" }] )"_json == events[index][events[index]["type"]]["keys"]);

      ++index;
      expect("keyboard_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [{ "key_code": "f3" }] )"_json == events[index][events[index]["type"]]["keys"]);

      ++index;
      expect("keyboard_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [] )"_json == events[index][events[index]["type"]]["keys"]);

      ++index;
      expect("apple_vendor_top_case_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [] )"_json == events[index][events[index]["type"]]["keys"]);

      // f3 -> spacebar
      ++index;
      expect("keyboard_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [{ "key_code": "spacebar" }] )"_json == events[index][events[index]["type"]]["keys"]);

      ++index;
      expect("keyboard_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [] )"_json == events[index][events[index]["type"]]["keys"]);

      // fn key_down
      ++index;
      expect("apple_vendor_top_case_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [{ "apple_vendor_top_case_key_code": "keyboard_fn" }] )"_json == events[index][events[index]["type"]]["keys"]);

      // fn+f1 -> fn+f1
      ++index;
      expect("keyboard_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [{ "key_code": "f1" }] )"_json == events[index][events[index]["type"]]["keys"]);

      ++index;
      expect("keyboard_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [] )"_json == events[index][events[index]["type"]]["keys"]);

      // fn+f2 -> fn+f2
      ++index;
      expect("keyboard_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [{ "key_code": "f2" }] )"_json == events[index][events[index]["type"]]["keys"]);

      ++index;
      expect("keyboard_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [] )"_json == events[index][events[index]["type"]]["keys"]);

      // fn+f3 -> fn+f3
      ++index;
      expect("keyboard_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [{ "key_code": "f3" }] )"_json == events[index][events[index]["type"]]["keys"]);

      ++index;
      expect("keyboard_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [] )"_json == events[index][events[index]["type"]]["keys"]);

      // fn key_up
      ++index;
      expect("apple_vendor_top_case_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [] )"_json == events[index][events[index]["type"]]["keys"]);

      // check size
      expect(index + 1 == events.size());

      // cleanup
      post_event_to_virtual_devices_manipulator->clear_queue();
    }

    //
    // use_fkeys_as_standard_function_keys: true
    // (f1 -> f1, fn+f1 -> mission_control)
    //

    {
      pqrs::osx::system_preferences::properties properties;
      properties.set_use_fkeys_as_standard_function_keys(true);

      manager->update(core_configuration->get_selected_profile(), properties);

      auto input_event_queue_copy = std::make_shared<krbn::event_queue::queue>();
      auto intermediate_event_queue = std::make_shared<krbn::event_queue::queue>();
      auto output_event_queue = std::make_shared<krbn::event_queue::queue>();

      auto connector = std::make_shared<krbn::manipulator::manipulator_managers_connector>();
      connector->emplace_back_connection(manager->get_manipulator_manager(),
                                         input_event_queue_copy,
                                         intermediate_event_queue);
      connector->emplace_back_connection(post_event_to_virtual_devices_manipulator_manager,
                                         output_event_queue);

      //
      // manipulate
      //

      for (const auto& e : input_event_queue->get_entries()) {
        input_event_queue_copy->push_back_entry(e);
        connector->manipulate(e.get_event_time_stamp().get_time_stamp(),
                              core_configuration);
      }

      auto events = nlohmann::json(post_event_to_virtual_devices_manipulator->get_queue().get_events());
      std::cout << events << std::endl;

      // f1 -> f1
      auto index = 0;
      expect("keyboard_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [{ "key_code": "f1" }] )"_json == events[index][events[index]["type"]]["keys"]);

      ++index;
      expect("keyboard_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [] )"_json == events[index][events[index]["type"]]["keys"]);

      // f2 -> f2
      ++index;
      expect("keyboard_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [{ "key_code": "f2" }] )"_json == events[index][events[index]["type"]]["keys"]);

      ++index;
      expect("keyboard_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [] )"_json == events[index][events[index]["type"]]["keys"]);

      // f3 -> f3
      ++index;
      expect("keyboard_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [{ "key_code": "f3" }] )"_json == events[index][events[index]["type"]]["keys"]);

      ++index;
      expect("keyboard_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [] )"_json == events[index][events[index]["type"]]["keys"]);

      // fn key_down
      ++index;
      expect("apple_vendor_top_case_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [{ "apple_vendor_top_case_key_code": "keyboard_fn" }] )"_json == events[index][events[index]["type"]]["keys"]);

      // fn+f1 -> fn+mission_control
      ++index;
      expect("apple_vendor_keyboard_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [{ "apple_vendor_keyboard_key_code": "mission_control" }] )"_json == events[index][events[index]["type"]]["keys"]);

      ++index;
      expect("apple_vendor_keyboard_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [] )"_json == events[index][events[index]["type"]]["keys"]);

      // fn+f2 -> f3
      ++index;
      expect("apple_vendor_top_case_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [] )"_json == events[index][events[index]["type"]]["keys"]);

      ++index;
      expect("keyboard_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [{ "key_code": "f3" }] )"_json == events[index][events[index]["type"]]["keys"]);

      ++index;
      expect("keyboard_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [] )"_json == events[index][events[index]["type"]]["keys"]);

      // fn+f3 -> fn+spacebar
      ++index;
      expect("apple_vendor_top_case_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [{ "apple_vendor_top_case_key_code": "keyboard_fn" }] )"_json == events[index][events[index]["type"]]["keys"]);

      ++index;
      expect("keyboard_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [{ "key_code": "spacebar" }] )"_json == events[index][events[index]["type"]]["keys"]);

      ++index;
      expect("keyboard_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [] )"_json == events[index][events[index]["type"]]["keys"]);

      // fn key_up
      ++index;
      expect("apple_vendor_top_case_input"sv == events[index]["type"].get<std::string>());
      expect(R"( [] )"_json == events[index][events[index]["type"]]["keys"]);

      // check size
      expect(index + 1 == events.size());

      // cleanup
      post_event_to_virtual_devices_manipulator->clear_queue();
    }
  };

  return 0;
}
