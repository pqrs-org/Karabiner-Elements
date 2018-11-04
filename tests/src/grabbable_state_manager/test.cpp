#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "dispatcher_utility.hpp"
#include "grabbable_state_manager.hpp"
#include "stream_utility.hpp"
#include <boost/optional/optional_io.hpp>

namespace {
auto device_id1 = krbn::device_id::zero;
auto device_id2 = krbn::device_id::zero;
auto registry_entry_id1 = krbn::registry_entry_id(4001);
auto registry_entry_id2 = krbn::registry_entry_id(4002);
} // namespace

TEST_CASE("initialize") {
  using namespace std::string_literals;

  krbn::dispatcher_utility::initialize_dispatchers();

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
  auto time_source = std::make_shared<pqrs::dispatcher::hardware_time_source>();
  auto dispatcher = std::make_shared<pqrs::dispatcher::dispatcher>(time_source);

  auto grabbable_state_manager = std::make_unique<krbn::grabbable_state_manager>();

  krbn::event_queue::queue event_queue;

  std::vector<krbn::grabbable_state> actual_grabbable_state_changed_history;
  std::vector<krbn::grabbable_state> expected_grabbable_state_changed_history;

  grabbable_state_manager->grabbable_state_changed.connect([&](auto&& grabbable_state) {
    actual_grabbable_state_changed_history.emplace_back(grabbable_state);
  });

  {
    auto actual = grabbable_state_manager->get_grabbable_state(registry_entry_id1);
    REQUIRE(!actual);
  }

  event_queue.clear_events();
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::event_time_stamp(krbn::absolute_time_point(1000)),
                                 krbn::event_queue::event(krbn::key_code::a),
                                 krbn::event_type::key_up,
                                 krbn::event_queue::event(krbn::key_code::a));
  event_queue.emplace_back_event(device_id2,
                                 krbn::event_queue::event_time_stamp(krbn::absolute_time_point(1010)),
                                 krbn::event_queue::event(krbn::key_code::a),
                                 krbn::event_type::key_up,
                                 krbn::event_queue::event(krbn::key_code::a));

  grabbable_state_manager->update(event_queue);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  expected_grabbable_state_changed_history.emplace_back(registry_entry_id1,
                                                        krbn::grabbable_state::state::grabbable,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                                        krbn::absolute_time_point(1000));
  expected_grabbable_state_changed_history.emplace_back(registry_entry_id2,
                                                        krbn::grabbable_state::state::grabbable,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                                        krbn::absolute_time_point(1010));

  {
    auto actual = grabbable_state_manager->get_grabbable_state(registry_entry_id1);
    auto expected = krbn::grabbable_state(registry_entry_id1,
                                          krbn::grabbable_state::state::grabbable,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                          krbn::absolute_time_point(1000));
    REQUIRE(*actual == expected);
  }
  {
    auto actual = grabbable_state_manager->get_grabbable_state(registry_entry_id2);
    auto expected = krbn::grabbable_state(registry_entry_id2,
                                          krbn::grabbable_state::state::grabbable,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                          krbn::absolute_time_point(1010));
    REQUIRE(*actual == expected);
  }

  // key repeating

  event_queue.clear_events();
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::event_time_stamp(krbn::absolute_time_point(2000)),
                                 krbn::event_queue::event(krbn::key_code::a),
                                 krbn::event_type::key_down,
                                 krbn::event_queue::event(krbn::key_code::a));

  grabbable_state_manager->update(event_queue);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  expected_grabbable_state_changed_history.emplace_back(registry_entry_id1,
                                                        krbn::grabbable_state::state::ungrabbable_temporarily,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::key_repeating,
                                                        krbn::absolute_time_point(2000));

  {
    auto actual = grabbable_state_manager->get_grabbable_state(registry_entry_id1);
    auto expected = krbn::grabbable_state(registry_entry_id1,
                                          krbn::grabbable_state::state::ungrabbable_temporarily,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::key_repeating,
                                          krbn::absolute_time_point(2000));
    REQUIRE(*actual == expected);
  }
  {
    auto actual = grabbable_state_manager->get_grabbable_state(registry_entry_id2);
    auto expected = krbn::grabbable_state(registry_entry_id2,
                                          krbn::grabbable_state::state::grabbable,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                          krbn::absolute_time_point(1010));
    REQUIRE(*actual == expected);
  }

  // cancel key repeating

  event_queue.clear_events();
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::event_time_stamp(krbn::absolute_time_point(3000)),
                                 krbn::event_queue::event(krbn::key_code::a),
                                 krbn::event_type::key_up,
                                 krbn::event_queue::event(krbn::key_code::a));

  grabbable_state_manager->update(event_queue);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  expected_grabbable_state_changed_history.emplace_back(registry_entry_id1,
                                                        krbn::grabbable_state::state::grabbable,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                                        krbn::absolute_time_point(3000));

  {
    auto actual = grabbable_state_manager->get_grabbable_state(registry_entry_id1);
    auto expected = krbn::grabbable_state(registry_entry_id1,
                                          krbn::grabbable_state::state::grabbable,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                          krbn::absolute_time_point(3000));
    REQUIRE(*actual == expected);
  }
  {
    auto actual = grabbable_state_manager->get_grabbable_state(registry_entry_id2);
    auto expected = krbn::grabbable_state(registry_entry_id2,
                                          krbn::grabbable_state::state::grabbable,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                          krbn::absolute_time_point(1010));
    REQUIRE(*actual == expected);
  }

  // key repeating (consumer_key)

  event_queue.clear_events();
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::event_time_stamp(krbn::absolute_time_point(4000)),
                                 krbn::event_queue::event(krbn::consumer_key_code::volume_increment),
                                 krbn::event_type::key_down,
                                 krbn::event_queue::event(krbn::consumer_key_code::volume_increment));

  grabbable_state_manager->update(event_queue);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  expected_grabbable_state_changed_history.emplace_back(registry_entry_id1,
                                                        krbn::grabbable_state::state::ungrabbable_temporarily,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::key_repeating,
                                                        krbn::absolute_time_point(4000));

  {
    auto actual = grabbable_state_manager->get_grabbable_state(registry_entry_id1);
    auto expected = krbn::grabbable_state(registry_entry_id1,
                                          krbn::grabbable_state::state::ungrabbable_temporarily,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::key_repeating,
                                          krbn::absolute_time_point(4000));
    REQUIRE(*actual == expected);
  }
  {
    auto actual = grabbable_state_manager->get_grabbable_state(registry_entry_id2);
    auto expected = krbn::grabbable_state(registry_entry_id2,
                                          krbn::grabbable_state::state::grabbable,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                          krbn::absolute_time_point(1010));
    REQUIRE(*actual == expected);
  }

  // cancel key repeating (consumer_key)

  event_queue.clear_events();
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::event_time_stamp(krbn::absolute_time_point(5000)),
                                 krbn::event_queue::event(krbn::consumer_key_code::volume_increment),
                                 krbn::event_type::key_up,
                                 krbn::event_queue::event(krbn::consumer_key_code::volume_increment));

  grabbable_state_manager->update(event_queue);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  expected_grabbable_state_changed_history.emplace_back(registry_entry_id1,
                                                        krbn::grabbable_state::state::grabbable,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                                        krbn::absolute_time_point(5000));

  {
    auto actual = grabbable_state_manager->get_grabbable_state(registry_entry_id1);
    auto expected = krbn::grabbable_state(registry_entry_id1,
                                          krbn::grabbable_state::state::grabbable,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                          krbn::absolute_time_point(5000));
    REQUIRE(*actual == expected);
  }
  {
    auto actual = grabbable_state_manager->get_grabbable_state(registry_entry_id2);
    auto expected = krbn::grabbable_state(registry_entry_id2,
                                          krbn::grabbable_state::state::grabbable,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                          krbn::absolute_time_point(1010));
    REQUIRE(*actual == expected);
  }

  // modifier_key_pressed

  event_queue.clear_events();
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::event_time_stamp(krbn::absolute_time_point(6000)),
                                 krbn::event_queue::event(krbn::key_code::left_shift),
                                 krbn::event_type::key_down,
                                 krbn::event_queue::event(krbn::key_code::left_shift));

  grabbable_state_manager->update(event_queue);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  expected_grabbable_state_changed_history.emplace_back(registry_entry_id1,
                                                        krbn::grabbable_state::state::ungrabbable_temporarily,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::modifier_key_pressed,
                                                        krbn::absolute_time_point(6000));

  {
    auto actual = grabbable_state_manager->get_grabbable_state(registry_entry_id1);
    auto expected = krbn::grabbable_state(registry_entry_id1,
                                          krbn::grabbable_state::state::ungrabbable_temporarily,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::modifier_key_pressed,
                                          krbn::absolute_time_point(6000));
    REQUIRE(*actual == expected);
  }
  {
    auto actual = grabbable_state_manager->get_grabbable_state(registry_entry_id2);
    auto expected = krbn::grabbable_state(registry_entry_id2,
                                          krbn::grabbable_state::state::grabbable,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                          krbn::absolute_time_point(1010));
    REQUIRE(*actual == expected);
  }

  // cancel modifier_key_pressed

  event_queue.clear_events();
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::event_time_stamp(krbn::absolute_time_point(7000)),
                                 krbn::event_queue::event(krbn::key_code::left_shift),
                                 krbn::event_type::key_up,
                                 krbn::event_queue::event(krbn::key_code::left_shift));

  grabbable_state_manager->update(event_queue);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  expected_grabbable_state_changed_history.emplace_back(registry_entry_id1,
                                                        krbn::grabbable_state::state::grabbable,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                                        krbn::absolute_time_point(7000));

  {
    auto actual = grabbable_state_manager->get_grabbable_state(registry_entry_id1);
    auto expected = krbn::grabbable_state(registry_entry_id1,
                                          krbn::grabbable_state::state::grabbable,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                          krbn::absolute_time_point(7000));
    REQUIRE(*actual == expected);
  }
  {
    auto actual = grabbable_state_manager->get_grabbable_state(registry_entry_id2);
    auto expected = krbn::grabbable_state(registry_entry_id2,
                                          krbn::grabbable_state::state::grabbable,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                          krbn::absolute_time_point(1010));
    REQUIRE(*actual == expected);
  }

  // pointing_button_pressed

  event_queue.clear_events();
  event_queue.emplace_back_event(device_id2,
                                 krbn::event_queue::event_time_stamp(krbn::absolute_time_point(8000)),
                                 krbn::event_queue::event(krbn::pointing_button::button1),
                                 krbn::event_type::key_down,
                                 krbn::event_queue::event(krbn::pointing_button::button1));

  grabbable_state_manager->update(event_queue);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  expected_grabbable_state_changed_history.emplace_back(registry_entry_id2,
                                                        krbn::grabbable_state::state::ungrabbable_temporarily,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::pointing_button_pressed,
                                                        krbn::absolute_time_point(8000));

  {
    auto actual = grabbable_state_manager->get_grabbable_state(registry_entry_id1);
    auto expected = krbn::grabbable_state(registry_entry_id1,
                                          krbn::grabbable_state::state::grabbable,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                          krbn::absolute_time_point(7000));
    REQUIRE(*actual == expected);
  }
  {
    auto actual = grabbable_state_manager->get_grabbable_state(registry_entry_id2);
    auto expected = krbn::grabbable_state(registry_entry_id2,
                                          krbn::grabbable_state::state::ungrabbable_temporarily,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::pointing_button_pressed,
                                          krbn::absolute_time_point(8000));
    REQUIRE(*actual == expected);
  }

  // cancel pointing_button_pressed

  event_queue.clear_events();
  event_queue.emplace_back_event(device_id2,
                                 krbn::event_queue::event_time_stamp(krbn::absolute_time_point(9000)),
                                 krbn::event_queue::event(krbn::pointing_button::button1),
                                 krbn::event_type::key_up,
                                 krbn::event_queue::event(krbn::pointing_button::button1));

  grabbable_state_manager->update(event_queue);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  expected_grabbable_state_changed_history.emplace_back(registry_entry_id2,
                                                        krbn::grabbable_state::state::grabbable,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                                        krbn::absolute_time_point(9000));

  {
    auto actual = grabbable_state_manager->get_grabbable_state(registry_entry_id1);
    auto expected = krbn::grabbable_state(registry_entry_id1,
                                          krbn::grabbable_state::state::grabbable,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                          krbn::absolute_time_point(7000));
    REQUIRE(*actual == expected);
  }
  {
    auto actual = grabbable_state_manager->get_grabbable_state(registry_entry_id2);
    auto expected = krbn::grabbable_state(registry_entry_id2,
                                          krbn::grabbable_state::state::grabbable,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                          krbn::absolute_time_point(9000));
    REQUIRE(*actual == expected);
  }

  // multiple reasons

  event_queue.clear_events();
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::event_time_stamp(krbn::absolute_time_point(10000)),
                                 krbn::event_queue::event(krbn::key_code::left_shift),
                                 krbn::event_type::key_down,
                                 krbn::event_queue::event(krbn::key_code::left_shift));
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::event_time_stamp(krbn::absolute_time_point(11000)),
                                 krbn::event_queue::event(krbn::key_code::a),
                                 krbn::event_type::key_down,
                                 krbn::event_queue::event(krbn::key_code::a));
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::event_time_stamp(krbn::absolute_time_point(12000)),
                                 krbn::event_queue::event(krbn::key_code::a),
                                 krbn::event_type::key_up,
                                 krbn::event_queue::event(krbn::key_code::a));
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::event_time_stamp(krbn::absolute_time_point(13000)),
                                 krbn::event_queue::event(krbn::key_code::left_shift),
                                 krbn::event_type::key_up,
                                 krbn::event_queue::event(krbn::key_code::left_shift));

  grabbable_state_manager->update(event_queue);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  expected_grabbable_state_changed_history.emplace_back(registry_entry_id1,
                                                        krbn::grabbable_state::state::ungrabbable_temporarily,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::modifier_key_pressed,
                                                        krbn::absolute_time_point(10000));
  expected_grabbable_state_changed_history.emplace_back(registry_entry_id1,
                                                        krbn::grabbable_state::state::ungrabbable_temporarily,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::key_repeating,
                                                        krbn::absolute_time_point(11000));
  expected_grabbable_state_changed_history.emplace_back(registry_entry_id1,
                                                        krbn::grabbable_state::state::ungrabbable_temporarily,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::modifier_key_pressed,
                                                        krbn::absolute_time_point(12000));
  expected_grabbable_state_changed_history.emplace_back(registry_entry_id1,
                                                        krbn::grabbable_state::state::grabbable,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                                        krbn::absolute_time_point(13000));

  // Test actual_grabbable_state_changed_history

  REQUIRE(actual_grabbable_state_changed_history == expected_grabbable_state_changed_history);

  grabbable_state_manager = nullptr;

  dispatcher->terminate();
  dispatcher = nullptr;
}

TEST_CASE("device_error") {
  auto time_source = std::make_shared<pqrs::dispatcher::hardware_time_source>();
  auto dispatcher = std::make_shared<pqrs::dispatcher::dispatcher>(time_source);

  auto grabbable_state_manager = std::make_unique<krbn::grabbable_state_manager>();

  krbn::event_queue::queue event_queue;

  std::vector<krbn::grabbable_state> actual_grabbable_state_changed_history;
  std::vector<krbn::grabbable_state> expected_grabbable_state_changed_history;

  grabbable_state_manager->grabbable_state_changed.connect([&](auto&& grabbable_state) {
    actual_grabbable_state_changed_history.emplace_back(grabbable_state);
  });

  {
    auto actual = grabbable_state_manager->get_grabbable_state(registry_entry_id1);
    REQUIRE(!actual);
  }

  // set device_error

  grabbable_state_manager->update(krbn::grabbable_state(registry_entry_id1,
                                                        krbn::grabbable_state::state::device_error,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                                        krbn::absolute_time_point(1000)));
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  expected_grabbable_state_changed_history.emplace_back(registry_entry_id1,
                                                        krbn::grabbable_state::state::device_error,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                                        krbn::absolute_time_point(1000));

  // key repeating (device_error will be canceled by `grabbable_state_manager::update`.)

  event_queue.clear_events();
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::event_time_stamp(krbn::absolute_time_point(2000)),
                                 krbn::event_queue::event(krbn::key_code::a),
                                 krbn::event_type::key_down,
                                 krbn::event_queue::event(krbn::key_code::a));

  expected_grabbable_state_changed_history.emplace_back(registry_entry_id1,
                                                        krbn::grabbable_state::state::ungrabbable_temporarily,
                                                        krbn::grabbable_state::ungrabbable_temporarily_reason::key_repeating,
                                                        krbn::absolute_time_point(2000));

  grabbable_state_manager->update(event_queue);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  {
    auto actual = grabbable_state_manager->get_grabbable_state(registry_entry_id1);
    auto expected = krbn::grabbable_state(registry_entry_id1,
                                          krbn::grabbable_state::state::ungrabbable_temporarily,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::key_repeating,
                                          krbn::absolute_time_point(2000));
    REQUIRE(*actual == expected);
  }

  // Test actual_grabbable_state_changed_history

  REQUIRE(actual_grabbable_state_changed_history == expected_grabbable_state_changed_history);

  grabbable_state_manager = nullptr;

  dispatcher->terminate();
  dispatcher = nullptr;
}

TEST_CASE("terminate") {
  krbn::dispatcher_utility::terminate_dispatchers();
}
