#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "boost_defs.hpp"

#include "grabbable_state_queues_manager.hpp"
#include "thread_utility.hpp"
#include <boost/optional/optional_io.hpp>

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
    auto dispatcher = std::make_shared<krbn::dispatcher::dispatcher>();

    auto manager = std::make_unique<krbn::grabbable_state_queues_manager>(dispatcher);

    std::unordered_map<krbn::registry_entry_id, boost::optional<krbn::grabbable_state>> last_changed_grabbable_states;
    last_changed_grabbable_states[registry_entry_id1] = boost::none;
    last_changed_grabbable_states[registry_entry_id2] = boost::none;

    std::unordered_map<krbn::registry_entry_id, int> changed_counts;
    changed_counts[registry_entry_id1] = 0;
    changed_counts[registry_entry_id2] = 0;

    manager->grabbable_state_changed.connect([&](auto&& registry_entry_id, auto&& grabbable_state) {
      last_changed_grabbable_states[registry_entry_id] = grabbable_state;
      ++(changed_counts[registry_entry_id]);
    });

    REQUIRE(!manager->find_current_grabbable_state(registry_entry_id1));
    REQUIRE(!manager->find_current_grabbable_state(registry_entry_id2));

    // Clear
    manager->clear();

    // Check last_changed_grabbable_states
    {
      REQUIRE(last_changed_grabbable_states[registry_entry_id1] == boost::none);
      REQUIRE(changed_counts[registry_entry_id1] == 0);

      REQUIRE(last_changed_grabbable_states[registry_entry_id2] == boost::none);
      REQUIRE(changed_counts[registry_entry_id2] == 0);
    }

    // `update_grabbable_state`
    for (krbn::absolute_time time_stamp(1000); time_stamp < krbn::absolute_time(10000); time_stamp += krbn::absolute_time(1000)) {
      krbn::grabbable_state state(registry_entry_id1,
                                  krbn::grabbable_state::state::grabbable,
                                  krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                  time_stamp);
      REQUIRE(manager->update_grabbable_state(state));
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      REQUIRE(manager->find_current_grabbable_state(registry_entry_id1)->get_time_stamp() == time_stamp);
      REQUIRE(!manager->find_current_grabbable_state(registry_entry_id2));
    }

    // Check last_changed_grabbable_states
    {
      krbn::grabbable_state state(registry_entry_id1,
                                  krbn::grabbable_state::state::grabbable,
                                  krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                  krbn::absolute_time(1000));
      REQUIRE(last_changed_grabbable_states[registry_entry_id1] == state);
      REQUIRE(changed_counts[registry_entry_id1] == 1);

      REQUIRE(last_changed_grabbable_states[registry_entry_id2] == boost::none);
      REQUIRE(changed_counts[registry_entry_id2] == 0);
    }

    // `update_first_grabbed_event_time_stamp`
    {
      krbn::event_queue::queue event_queue;
      event_queue.emplace_back_event(device_id1,
                                     krbn::event_queue::event_time_stamp(krbn::absolute_time(5000)),
                                     krbn::event_queue::event(krbn::key_code::a),
                                     krbn::event_type::key_down,
                                     krbn::event_queue::event(krbn::key_code::a));
      REQUIRE(manager->update_first_grabbed_event_time_stamp(event_queue));
      REQUIRE(manager->find_current_grabbable_state(registry_entry_id1)->get_time_stamp() == krbn::absolute_time(4000));
      REQUIRE(!manager->find_current_grabbable_state(registry_entry_id2));
    }

    // Ignore events after first_grabbed_event_time_stamp_.
    {
      krbn::grabbable_state state(registry_entry_id1,
                                  krbn::grabbable_state::state::grabbable,
                                  krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                  krbn::absolute_time(6000));
      REQUIRE(!manager->update_grabbable_state(state));
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      REQUIRE(manager->find_current_grabbable_state(registry_entry_id1)->get_time_stamp() == krbn::absolute_time(4000));
      REQUIRE(!manager->find_current_grabbable_state(registry_entry_id2));
    }

    // Ignore events after first_grabbed_event_time_stamp_.
    {
      krbn::event_queue::queue event_queue;
      event_queue.emplace_back_event(device_id1,
                                     krbn::event_queue::event_time_stamp(krbn::absolute_time(4000)),
                                     krbn::event_queue::event(krbn::key_code::a),
                                     krbn::event_type::key_down,
                                     krbn::event_queue::event(krbn::key_code::a));
      REQUIRE(!manager->update_first_grabbed_event_time_stamp(event_queue));
      REQUIRE(manager->find_current_grabbable_state(registry_entry_id1)->get_time_stamp() == krbn::absolute_time(4000));
      REQUIRE(!manager->find_current_grabbable_state(registry_entry_id2));
    }

    manager = nullptr;

    dispatcher->terminate();
    dispatcher = nullptr;
  }
}
