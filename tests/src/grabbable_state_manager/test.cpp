#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "grabbable_state_manager.hpp"
#include "stream_utility.hpp"
#include "thread_utility.hpp"
#include <boost/optional/optional_io.hpp>

namespace {
auto device_id1 = krbn::device_id::zero;
auto device_id2 = krbn::device_id::zero;
auto registry_entry_id1 = krbn::registry_entry_id(4001);
auto registry_entry_id2 = krbn::registry_entry_id(4002);
} // namespace

TEST_CASE("initialize") {
  using namespace std::string_literals;

  krbn::thread_utility::register_main_thread();

  {
    auto device_detail = std::make_shared<krbn::device_detail>(krbn::vendor_id(1001),
                                                               krbn::product_id(2001),
                                                               krbn::location_id(3001),
                                                               "manufacturer_1"s,
                                                               "product_1"s,
                                                               "serial_number_1"s,
                                                               "transport_1"s,
                                                               registry_entry_id1,
                                                               true,
                                                               true);
    device_id1 = krbn::types::make_new_device_id(device_detail);

    auto actual = krbn::types::find_device_detail(device_id1)->get_registry_entry_id();
    REQUIRE(*actual == registry_entry_id1);
  }

  {
    auto device_detail = std::make_shared<krbn::device_detail>(krbn::vendor_id(1002),
                                                               krbn::product_id(2002),
                                                               krbn::location_id(3002),
                                                               "manufacturer_2"s,
                                                               "product_2"s,
                                                               "serial_number_2"s,
                                                               "transport_2"s,
                                                               registry_entry_id2,
                                                               true,
                                                               true);
    device_id2 = krbn::types::make_new_device_id(device_detail);

    auto actual = krbn::types::find_device_detail(device_id2)->get_registry_entry_id();
    REQUIRE(*actual == registry_entry_id2);
  }
}

TEST_CASE("grabbable_state_manager") {
  krbn::grabbable_state_manager grabbable_state_manager;
  krbn::event_queue event_queue;

  std::vector<krbn::grabbable_state> actual_grabbable_state_changed_history;
  std::vector<krbn::grabbable_state> expected_grabbable_state_changed_history;

  grabbable_state_manager.grabbable_state_changed.connect([&](auto&& grabbable_state) {
    actual_grabbable_state_changed_history.emplace_back(grabbable_state);
  });

  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id1);
    REQUIRE(!actual);
  }

  event_queue.clear_events();
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::queued_event::event_time_stamp(1000),
                                 krbn::event_queue::queued_event::event(krbn::key_code::a),
                                 krbn::event_type::key_up,
                                 krbn::event_queue::queued_event::event(krbn::key_code::a));
  event_queue.emplace_back_event(device_id2,
                                 krbn::event_queue::queued_event::event_time_stamp(1010),
                                 krbn::event_queue::queued_event::event(krbn::key_code::a),
                                 krbn::event_type::key_up,
                                 krbn::event_queue::queued_event::event(krbn::key_code::a));

  grabbable_state_manager.update(event_queue);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  expected_grabbable_state_changed_history.emplace_back(registry_entry_id1,
                                                        krbn::grabbable_state::state::grabbable,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                                        1000);
  expected_grabbable_state_changed_history.emplace_back(registry_entry_id2,
                                                        krbn::grabbable_state::state::grabbable,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                                        1010);

  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id1);
    auto expected = krbn::grabbable_state(registry_entry_id1,
                                          krbn::grabbable_state::state::grabbable,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                          1000);
    REQUIRE(*actual == expected);
  }
  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id2);
    auto expected = krbn::grabbable_state(registry_entry_id2,
                                          krbn::grabbable_state::state::grabbable,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                          1010);
    REQUIRE(*actual == expected);
  }

  // key repeating

  event_queue.clear_events();
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::queued_event::event_time_stamp(2000),
                                 krbn::event_queue::queued_event::event(krbn::key_code::a),
                                 krbn::event_type::key_down,
                                 krbn::event_queue::queued_event::event(krbn::key_code::a));

  grabbable_state_manager.update(event_queue);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  expected_grabbable_state_changed_history.emplace_back(registry_entry_id1,
                                                        krbn::grabbable_state::state::ungrabbable_temporarily,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::key_repeating,
                                                        2000);

  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id1);
    auto expected = krbn::grabbable_state(registry_entry_id1,
                                          krbn::grabbable_state::state::ungrabbable_temporarily,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::key_repeating,
                                          2000);
    REQUIRE(*actual == expected);
  }
  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id2);
    auto expected = krbn::grabbable_state(registry_entry_id2,
                                          krbn::grabbable_state::state::grabbable,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                          1010);
    REQUIRE(*actual == expected);
  }

  // cancel key repeating

  event_queue.clear_events();
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::queued_event::event_time_stamp(3000),
                                 krbn::event_queue::queued_event::event(krbn::key_code::a),
                                 krbn::event_type::key_up,
                                 krbn::event_queue::queued_event::event(krbn::key_code::a));

  grabbable_state_manager.update(event_queue);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  expected_grabbable_state_changed_history.emplace_back(registry_entry_id1,
                                                        krbn::grabbable_state::state::grabbable,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                                        3000);

  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id1);
    auto expected = krbn::grabbable_state(registry_entry_id1,
                                          krbn::grabbable_state::state::grabbable,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                          3000);
    REQUIRE(*actual == expected);
  }
  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id2);
    auto expected = krbn::grabbable_state(registry_entry_id2,
                                          krbn::grabbable_state::state::grabbable,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                          1010);
    REQUIRE(*actual == expected);
  }

  // key repeating (consumer_key)

  event_queue.clear_events();
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::queued_event::event_time_stamp(4000),
                                 krbn::event_queue::queued_event::event(krbn::consumer_key_code::volume_increment),
                                 krbn::event_type::key_down,
                                 krbn::event_queue::queued_event::event(krbn::consumer_key_code::volume_increment));

  grabbable_state_manager.update(event_queue);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  expected_grabbable_state_changed_history.emplace_back(registry_entry_id1,
                                                        krbn::grabbable_state::state::ungrabbable_temporarily,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::key_repeating,
                                                        4000);

  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id1);
    auto expected = krbn::grabbable_state(registry_entry_id1,
                                          krbn::grabbable_state::state::ungrabbable_temporarily,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::key_repeating,
                                          4000);
    REQUIRE(*actual == expected);
  }
  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id2);
    auto expected = krbn::grabbable_state(registry_entry_id2,
                                          krbn::grabbable_state::state::grabbable,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                          1010);
    REQUIRE(*actual == expected);
  }

  // cancel key repeating (consumer_key)

  event_queue.clear_events();
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::queued_event::event_time_stamp(5000),
                                 krbn::event_queue::queued_event::event(krbn::consumer_key_code::volume_increment),
                                 krbn::event_type::key_up,
                                 krbn::event_queue::queued_event::event(krbn::consumer_key_code::volume_increment));

  grabbable_state_manager.update(event_queue);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  expected_grabbable_state_changed_history.emplace_back(registry_entry_id1,
                                                        krbn::grabbable_state::state::grabbable,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                                        5000);

  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id1);
    auto expected = krbn::grabbable_state(registry_entry_id1,
                                          krbn::grabbable_state::state::grabbable,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                          5000);
    REQUIRE(*actual == expected);
  }
  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id2);
    auto expected = krbn::grabbable_state(registry_entry_id2,
                                          krbn::grabbable_state::state::grabbable,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                          1010);
    REQUIRE(*actual == expected);
  }

  // modifier_key_pressed

  event_queue.clear_events();
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::queued_event::event_time_stamp(6000),
                                 krbn::event_queue::queued_event::event(krbn::key_code::left_shift),
                                 krbn::event_type::key_down,
                                 krbn::event_queue::queued_event::event(krbn::key_code::left_shift));

  grabbable_state_manager.update(event_queue);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  expected_grabbable_state_changed_history.emplace_back(registry_entry_id1,
                                                        krbn::grabbable_state::state::ungrabbable_temporarily,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::modifier_key_pressed,
                                                        6000);

  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id1);
    auto expected = krbn::grabbable_state(registry_entry_id1,
                                          krbn::grabbable_state::state::ungrabbable_temporarily,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::modifier_key_pressed,
                                          6000);
    REQUIRE(*actual == expected);
  }
  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id2);
    auto expected = krbn::grabbable_state(registry_entry_id2,
                                          krbn::grabbable_state::state::grabbable,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                          1010);
    REQUIRE(*actual == expected);
  }

  // cancel modifier_key_pressed

  event_queue.clear_events();
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::queued_event::event_time_stamp(7000),
                                 krbn::event_queue::queued_event::event(krbn::key_code::left_shift),
                                 krbn::event_type::key_up,
                                 krbn::event_queue::queued_event::event(krbn::key_code::left_shift));

  grabbable_state_manager.update(event_queue);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  expected_grabbable_state_changed_history.emplace_back(registry_entry_id1,
                                                        krbn::grabbable_state::state::grabbable,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                                        7000);

  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id1);
    auto expected = krbn::grabbable_state(registry_entry_id1,
                                          krbn::grabbable_state::state::grabbable,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                          7000);
    REQUIRE(*actual == expected);
  }
  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id2);
    auto expected = krbn::grabbable_state(registry_entry_id2,
                                          krbn::grabbable_state::state::grabbable,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                          1010);
    REQUIRE(*actual == expected);
  }

  // pointing_button_pressed

  event_queue.clear_events();
  event_queue.emplace_back_event(device_id2,
                                 krbn::event_queue::queued_event::event_time_stamp(8000),
                                 krbn::event_queue::queued_event::event(krbn::pointing_button::button1),
                                 krbn::event_type::key_down,
                                 krbn::event_queue::queued_event::event(krbn::pointing_button::button1));

  grabbable_state_manager.update(event_queue);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  expected_grabbable_state_changed_history.emplace_back(registry_entry_id2,
                                                        krbn::grabbable_state::state::ungrabbable_temporarily,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::pointing_button_pressed,
                                                        8000);

  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id1);
    auto expected = krbn::grabbable_state(registry_entry_id1,
                                          krbn::grabbable_state::state::grabbable,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                          7000);
    REQUIRE(*actual == expected);
  }
  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id2);
    auto expected = krbn::grabbable_state(registry_entry_id2,
                                          krbn::grabbable_state::state::ungrabbable_temporarily,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::pointing_button_pressed,
                                          8000);
    REQUIRE(*actual == expected);
  }

  // cancel pointing_button_pressed

  event_queue.clear_events();
  event_queue.emplace_back_event(device_id2,
                                 krbn::event_queue::queued_event::event_time_stamp(9000),
                                 krbn::event_queue::queued_event::event(krbn::pointing_button::button1),
                                 krbn::event_type::key_up,
                                 krbn::event_queue::queued_event::event(krbn::pointing_button::button1));

  grabbable_state_manager.update(event_queue);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  expected_grabbable_state_changed_history.emplace_back(registry_entry_id2,
                                                        krbn::grabbable_state::state::grabbable,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                                        9000);

  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id1);
    auto expected = krbn::grabbable_state(registry_entry_id1,
                                          krbn::grabbable_state::state::grabbable,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                          7000);
    REQUIRE(*actual == expected);
  }
  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id2);
    auto expected = krbn::grabbable_state(registry_entry_id2,
                                          krbn::grabbable_state::state::grabbable,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                          9000);
    REQUIRE(*actual == expected);
  }

  // multiple reasons

  event_queue.clear_events();
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::queued_event::event_time_stamp(10000),
                                 krbn::event_queue::queued_event::event(krbn::key_code::left_shift),
                                 krbn::event_type::key_down,
                                 krbn::event_queue::queued_event::event(krbn::key_code::left_shift));
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::queued_event::event_time_stamp(11000),
                                 krbn::event_queue::queued_event::event(krbn::key_code::a),
                                 krbn::event_type::key_down,
                                 krbn::event_queue::queued_event::event(krbn::key_code::a));
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::queued_event::event_time_stamp(12000),
                                 krbn::event_queue::queued_event::event(krbn::key_code::a),
                                 krbn::event_type::key_up,
                                 krbn::event_queue::queued_event::event(krbn::key_code::a));
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::queued_event::event_time_stamp(13000),
                                 krbn::event_queue::queued_event::event(krbn::key_code::left_shift),
                                 krbn::event_type::key_up,
                                 krbn::event_queue::queued_event::event(krbn::key_code::left_shift));

  grabbable_state_manager.update(event_queue);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  expected_grabbable_state_changed_history.emplace_back(registry_entry_id1,
                                                        krbn::grabbable_state::state::ungrabbable_temporarily,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::modifier_key_pressed,
                                                        10000);
  expected_grabbable_state_changed_history.emplace_back(registry_entry_id1,
                                                        krbn::grabbable_state::state::ungrabbable_temporarily,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::key_repeating,
                                                        11000);
  expected_grabbable_state_changed_history.emplace_back(registry_entry_id1,
                                                        krbn::grabbable_state::state::ungrabbable_temporarily,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::modifier_key_pressed,
                                                        12000);
  expected_grabbable_state_changed_history.emplace_back(registry_entry_id1,
                                                        krbn::grabbable_state::state::grabbable,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                                        13000);

  // Test actual_grabbable_state_changed_history

  REQUIRE(actual_grabbable_state_changed_history == expected_grabbable_state_changed_history);
}

TEST_CASE("device_error") {
  krbn::grabbable_state_manager grabbable_state_manager;
  krbn::event_queue event_queue;

  std::vector<krbn::grabbable_state> actual_grabbable_state_changed_history;
  std::vector<krbn::grabbable_state> expected_grabbable_state_changed_history;

  grabbable_state_manager.grabbable_state_changed.connect([&](auto&& grabbable_state) {
    actual_grabbable_state_changed_history.emplace_back(grabbable_state);
  });

  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id1);
    REQUIRE(!actual);
  }

  // set device_error

  grabbable_state_manager.update(krbn::grabbable_state(registry_entry_id1,
                                                       krbn::grabbable_state::state::device_error,
                                                       krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                                       1000));
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  expected_grabbable_state_changed_history.emplace_back(registry_entry_id1,
                                                        krbn::grabbable_state::state::device_error,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                                        1000);

  // key repeating (device_error will be canceled by `grabbable_state_manager::update`.)

  event_queue.clear_events();
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::queued_event::event_time_stamp(2000),
                                 krbn::event_queue::queued_event::event(krbn::key_code::a),
                                 krbn::event_type::key_down,
                                 krbn::event_queue::queued_event::event(krbn::key_code::a));

  expected_grabbable_state_changed_history.emplace_back(registry_entry_id1,
                                                        krbn::grabbable_state::state::ungrabbable_temporarily,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::key_repeating,
                                                        2000);

  grabbable_state_manager.update(event_queue);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id1);
    auto expected = krbn::grabbable_state(registry_entry_id1,
                                          krbn::grabbable_state::state::ungrabbable_temporarily,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::key_repeating,
                                          2000);
    REQUIRE(*actual == expected);
  }

  // Test actual_grabbable_state_changed_history

  REQUIRE(actual_grabbable_state_changed_history == expected_grabbable_state_changed_history);
}
