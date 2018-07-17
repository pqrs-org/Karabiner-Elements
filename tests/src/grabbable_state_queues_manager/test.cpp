#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "grabbable_state_queues_manager.hpp"
#include "thread_utility.hpp"

namespace {
auto device_id1 = krbn::device_id::zero;
auto device_id2 = krbn::device_id::zero;
auto registry_entry_id1 = krbn::registry_entry_id(1);
auto registry_entry_id2 = krbn::registry_entry_id(2);
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

TEST_CASE("grabbable_state_queues_manager") {
  {
    krbn::grabbable_state_queues_manager manager;
    REQUIRE(!manager.find_current_grabbable_state(registry_entry_id1));
    REQUIRE(!manager.find_current_grabbable_state(registry_entry_id2));

    for (auto time_stamp = 1000ULL; time_stamp < 10000ULL; time_stamp += 1000ULL) {
      krbn::grabbable_state state(registry_entry_id1,
                                  krbn::grabbable_state::state::grabbable,
                                  krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                  time_stamp);
      manager.update_grabbable_state(state);
      REQUIRE(manager.find_current_grabbable_state(registry_entry_id1)->get_time_stamp() == time_stamp);
      REQUIRE(!manager.find_current_grabbable_state(registry_entry_id2));
    }

    {
      krbn::event_queue event_queue;
      event_queue.emplace_back_event(device_id1,
                                     krbn::event_queue::queued_event::event_time_stamp(5000),
                                     krbn::event_queue::queued_event::event(krbn::key_code::a),
                                     krbn::event_type::key_down,
                                     krbn::event_queue::queued_event::event(krbn::key_code::a));
      manager.update_first_grabbed_event_time_stamp(event_queue);
      REQUIRE(manager.find_current_grabbable_state(registry_entry_id1)->get_time_stamp() == 4000ULL);
      REQUIRE(!manager.find_current_grabbable_state(registry_entry_id2));
    }
  }
}
