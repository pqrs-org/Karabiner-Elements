#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "grabbable_state_manager.hpp"
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

  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id1);
    REQUIRE(!actual);
  }

  event_queue.empty();
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::queued_event::event_time_stamp(1000),
                                 krbn::event_queue::queued_event::event(krbn::key_code::a),
                                 krbn::event_type::key_up,
                                 krbn::event_queue::queued_event::event(krbn::key_code::a));
  event_queue.emplace_back_event(device_id2,
                                 krbn::event_queue::queued_event::event_time_stamp(1000),
                                 krbn::event_queue::queued_event::event(krbn::key_code::a),
                                 krbn::event_type::key_up,
                                 krbn::event_queue::queued_event::event(krbn::key_code::a));
  grabbable_state_manager.update(event_queue);

  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id1);
    auto expected = std::make_pair(krbn::grabbable_state::grabbable,
                                   krbn::ungrabbable_temporarily_reason::none);
    REQUIRE(*actual == expected);
  }
  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id2);
    auto expected = std::make_pair(krbn::grabbable_state::grabbable,
                                   krbn::ungrabbable_temporarily_reason::none);
    REQUIRE(*actual == expected);
  }

  // key repeating

  event_queue.empty();
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::queued_event::event_time_stamp(1000),
                                 krbn::event_queue::queued_event::event(krbn::key_code::a),
                                 krbn::event_type::key_down,
                                 krbn::event_queue::queued_event::event(krbn::key_code::a));
  grabbable_state_manager.update(event_queue);

  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id1);
    auto expected = std::make_pair(krbn::grabbable_state::ungrabbable_temporarily,
                                   krbn::ungrabbable_temporarily_reason::key_repeating);
    REQUIRE(*actual == expected);
  }
  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id2);
    auto expected = std::make_pair(krbn::grabbable_state::grabbable,
                                   krbn::ungrabbable_temporarily_reason::none);
    REQUIRE(*actual == expected);
  }

  // cancel key repeating

  event_queue.empty();
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::queued_event::event_time_stamp(1000),
                                 krbn::event_queue::queued_event::event(krbn::key_code::a),
                                 krbn::event_type::key_up,
                                 krbn::event_queue::queued_event::event(krbn::key_code::a));
  grabbable_state_manager.update(event_queue);

  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id1);
    auto expected = std::make_pair(krbn::grabbable_state::grabbable,
                                   krbn::ungrabbable_temporarily_reason::none);
    REQUIRE(*actual == expected);
  }
  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id2);
    auto expected = std::make_pair(krbn::grabbable_state::grabbable,
                                   krbn::ungrabbable_temporarily_reason::none);
    REQUIRE(*actual == expected);
  }

  // key repeating (consumer_key)

  event_queue.empty();
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::queued_event::event_time_stamp(1000),
                                 krbn::event_queue::queued_event::event(krbn::consumer_key_code::volume_increment),
                                 krbn::event_type::key_down,
                                 krbn::event_queue::queued_event::event(krbn::consumer_key_code::volume_increment));
  grabbable_state_manager.update(event_queue);

  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id1);
    auto expected = std::make_pair(krbn::grabbable_state::ungrabbable_temporarily,
                                   krbn::ungrabbable_temporarily_reason::key_repeating);
    REQUIRE(*actual == expected);
  }
  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id2);
    auto expected = std::make_pair(krbn::grabbable_state::grabbable,
                                   krbn::ungrabbable_temporarily_reason::none);
    REQUIRE(*actual == expected);
  }

  // cancel key repeating (consumer_key)

  event_queue.empty();
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::queued_event::event_time_stamp(1000),
                                 krbn::event_queue::queued_event::event(krbn::consumer_key_code::volume_increment),
                                 krbn::event_type::key_up,
                                 krbn::event_queue::queued_event::event(krbn::consumer_key_code::volume_increment));
  grabbable_state_manager.update(event_queue);

  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id1);
    auto expected = std::make_pair(krbn::grabbable_state::grabbable,
                                   krbn::ungrabbable_temporarily_reason::none);
    REQUIRE(*actual == expected);
  }
  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id2);
    auto expected = std::make_pair(krbn::grabbable_state::grabbable,
                                   krbn::ungrabbable_temporarily_reason::none);
    REQUIRE(*actual == expected);
  }

  // modifier_key_pressed

  event_queue.empty();
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::queued_event::event_time_stamp(1000),
                                 krbn::event_queue::queued_event::event(krbn::key_code::left_shift),
                                 krbn::event_type::key_down,
                                 krbn::event_queue::queued_event::event(krbn::key_code::left_shift));
  grabbable_state_manager.update(event_queue);

  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id1);
    auto expected = std::make_pair(krbn::grabbable_state::ungrabbable_temporarily,
                                   krbn::ungrabbable_temporarily_reason::modifier_key_pressed);
    REQUIRE(*actual == expected);
  }
  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id2);
    auto expected = std::make_pair(krbn::grabbable_state::grabbable,
                                   krbn::ungrabbable_temporarily_reason::none);
    REQUIRE(*actual == expected);
  }

  // cancel modifier_key_pressed

  event_queue.empty();
  event_queue.emplace_back_event(device_id1,
                                 krbn::event_queue::queued_event::event_time_stamp(1000),
                                 krbn::event_queue::queued_event::event(krbn::key_code::left_shift),
                                 krbn::event_type::key_up,
                                 krbn::event_queue::queued_event::event(krbn::key_code::left_shift));
  grabbable_state_manager.update(event_queue);

  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id1);
    auto expected = std::make_pair(krbn::grabbable_state::grabbable,
                                   krbn::ungrabbable_temporarily_reason::none);
    REQUIRE(*actual == expected);
  }
  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id2);
    auto expected = std::make_pair(krbn::grabbable_state::grabbable,
                                   krbn::ungrabbable_temporarily_reason::none);
    REQUIRE(*actual == expected);
  }

  // pointing_button_pressed

  event_queue.empty();
  event_queue.emplace_back_event(device_id2,
                                 krbn::event_queue::queued_event::event_time_stamp(1000),
                                 krbn::event_queue::queued_event::event(krbn::pointing_button::button1),
                                 krbn::event_type::key_down,
                                 krbn::event_queue::queued_event::event(krbn::pointing_button::button1));
  grabbable_state_manager.update(event_queue);

  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id1);
    auto expected = std::make_pair(krbn::grabbable_state::grabbable,
                                   krbn::ungrabbable_temporarily_reason::none);
    REQUIRE(*actual == expected);
  }
  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id2);
    auto expected = std::make_pair(krbn::grabbable_state::ungrabbable_temporarily,
                                   krbn::ungrabbable_temporarily_reason::pointing_button_pressed);
    REQUIRE(*actual == expected);
  }

  // cancel pointing_button_pressed

  event_queue.empty();
  event_queue.emplace_back_event(device_id2,
                                 krbn::event_queue::queued_event::event_time_stamp(1000),
                                 krbn::event_queue::queued_event::event(krbn::pointing_button::button1),
                                 krbn::event_type::key_up,
                                 krbn::event_queue::queued_event::event(krbn::pointing_button::button1));
  grabbable_state_manager.update(event_queue);

  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id1);
    auto expected = std::make_pair(krbn::grabbable_state::grabbable,
                                   krbn::ungrabbable_temporarily_reason::none);
    REQUIRE(*actual == expected);
  }
  {
    auto actual = grabbable_state_manager.get_grabbable_state(registry_entry_id2);
    auto expected = std::make_pair(krbn::grabbable_state::grabbable,
                                   krbn::ungrabbable_temporarily_reason::none);
    REQUIRE(*actual == expected);
  }
}
