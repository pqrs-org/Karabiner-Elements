#include "../../../src/core/grabber/include/grabber/device_grabber_details/fn_function_keys_manipulator_manager.hpp"
#include <boost/ut.hpp>

int main(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "fn_function_keys_manipulator_manager"_test = [] {
    auto device_id_1 = krbn::device_id(1);
    auto state_original = krbn::event_queue::state::original;

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

    auto manager = std::make_shared<krbn::grabber::device_grabber_details::fn_function_keys_manipulator_manager>();

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
    // f1 -> mission_control, fn+f1 -> f1
    //

    {
      pqrs::osx::system_preferences::properties properties;
      properties.set_use_fkeys_as_standard_function_keys(false);

      manager->update(core_configuration->get_selected_profile(), properties);

      std::cout << core_configuration->get_selected_profile().get_fn_function_keys() << std::endl;

      auto input_event_queue = std::make_shared<krbn::event_queue::queue>();
      auto output_event_queue = std::make_shared<krbn::event_queue::queue>();

      //
      // f1
      //

      auto t = krbn::absolute_time_point(1);
      input_event_queue->emplace_back_entry(device_id_1, krbn::event_queue::event_time_stamp(t), f1, krbn::event_type::key_down, f1, state_original);

      t += pqrs::osx::chrono::make_absolute_time_duration(std::chrono::milliseconds(1));
      input_event_queue->emplace_back_entry(device_id_1, krbn::event_queue::event_time_stamp(t), f1, krbn::event_type::key_up, f1, state_original);

      //
      // f2
      //

      t += pqrs::osx::chrono::make_absolute_time_duration(std::chrono::milliseconds(1));
      input_event_queue->emplace_back_entry(device_id_1, krbn::event_queue::event_time_stamp(t), f2, krbn::event_type::key_down, f2, state_original);

      t += pqrs::osx::chrono::make_absolute_time_duration(std::chrono::milliseconds(1));
      input_event_queue->emplace_back_entry(device_id_1, krbn::event_queue::event_time_stamp(t), f2, krbn::event_type::key_up, f2, state_original);

      //
      // f3
      //

      t += pqrs::osx::chrono::make_absolute_time_duration(std::chrono::milliseconds(1));
      input_event_queue->emplace_back_entry(device_id_1, krbn::event_queue::event_time_stamp(t), f3, krbn::event_type::key_down, f3, state_original);

      t += pqrs::osx::chrono::make_absolute_time_duration(std::chrono::milliseconds(1));
      input_event_queue->emplace_back_entry(device_id_1, krbn::event_queue::event_time_stamp(t), f3, krbn::event_type::key_up, f3, state_original);

      //
      // manipulate
      //

      t += pqrs::osx::chrono::make_absolute_time_duration(std::chrono::milliseconds(1));
      while (manager->get_manipulator_manager()->manipulate(input_event_queue,
                                                            output_event_queue,
                                                            t)) {
      }

      expect(8 == output_event_queue->get_entries().size());

      expect(mission_control == output_event_queue->get_entries()[0].get_event()); // key_down
      expect(mission_control == output_event_queue->get_entries()[1].get_event()); // key_up
      expect(fn == output_event_queue->get_entries()[2].get_event());              // key_down
      expect(f3 == output_event_queue->get_entries()[3].get_event());              // key_down
      expect(fn == output_event_queue->get_entries()[4].get_event());              // key_up
      expect(f3 == output_event_queue->get_entries()[5].get_event());              // key_up
      expect(spacebar == output_event_queue->get_entries()[6].get_event());        // key_down
      expect(spacebar == output_event_queue->get_entries()[7].get_event());        // key_up
    }
  };

  return 0;
}
