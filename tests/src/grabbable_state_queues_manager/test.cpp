#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "boost_defs.hpp"

#include "dispatcher_utility.hpp"
#include "grabbable_state_queues_manager.hpp"
#include <boost/optional/optional_io.hpp>

TEST_CASE("initialize") {
  krbn::dispatcher_utility::initialize_dispatchers();
}

TEST_CASE("grabbable_state_queues_manager") {
  {
    auto manager = std::make_unique<krbn::grabbable_state_queues_manager>();

    std::unordered_map<krbn::device_id, boost::optional<krbn::grabbable_state>> last_changed_grabbable_states;
    last_changed_grabbable_states[krbn::device_id(1)] = boost::none;
    last_changed_grabbable_states[krbn::device_id(2)] = boost::none;

    std::unordered_map<krbn::device_id, int> changed_counts;
    changed_counts[krbn::device_id(1)] = 0;
    changed_counts[krbn::device_id(2)] = 0;

    manager->grabbable_state_changed.connect([&](auto&& registry_entry_id, auto&& grabbable_state) {
      last_changed_grabbable_states[registry_entry_id] = grabbable_state;
      ++(changed_counts[registry_entry_id]);
    });

    REQUIRE(!manager->find_current_grabbable_state(krbn::device_id(1)));
    REQUIRE(!manager->find_current_grabbable_state(krbn::device_id(2)));

    // Clear
    manager->clear();

    // Check last_changed_grabbable_states
    {
      REQUIRE(last_changed_grabbable_states[krbn::device_id(1)] == boost::none);
      REQUIRE(changed_counts[krbn::device_id(1)] == 0);

      REQUIRE(last_changed_grabbable_states[krbn::device_id(2)] == boost::none);
      REQUIRE(changed_counts[krbn::device_id(2)] == 0);
    }

    // `update_grabbable_state`
    for (krbn::absolute_time_point time_stamp(1000); time_stamp < krbn::absolute_time_point(10000); time_stamp += krbn::absolute_time_duration(1000)) {
      krbn::grabbable_state state(krbn::device_id(1),
                                  krbn::grabbable_state::state::grabbable,
                                  krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                  time_stamp);
      REQUIRE(manager->update_grabbable_state(state));
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      REQUIRE(manager->find_current_grabbable_state(krbn::device_id(1))->get_time_stamp() == time_stamp);
      REQUIRE(!manager->find_current_grabbable_state(krbn::device_id(2)));
    }

    // Check last_changed_grabbable_states
    {
      krbn::grabbable_state state(krbn::device_id(1),
                                  krbn::grabbable_state::state::grabbable,
                                  krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                  krbn::absolute_time_point(1000));
      REQUIRE(last_changed_grabbable_states[krbn::device_id(1)] == state);
      REQUIRE(changed_counts[krbn::device_id(1)] == 1);

      REQUIRE(last_changed_grabbable_states[krbn::device_id(2)] == boost::none);
      REQUIRE(changed_counts[krbn::device_id(2)] == 0);
    }

    // `update_first_grabbed_event_time_stamp`
    {
      krbn::event_queue::queue event_queue;
      event_queue.emplace_back_entry(krbn::device_id(1),
                                     krbn::event_queue::event_time_stamp(krbn::absolute_time_point(5000)),
                                     krbn::event_queue::event(krbn::key_code::a),
                                     krbn::event_type::key_down,
                                     krbn::event_queue::event(krbn::key_code::a));
      REQUIRE(manager->update_first_grabbed_event_time_stamp(event_queue));
      REQUIRE(manager->find_current_grabbable_state(krbn::device_id(1))->get_time_stamp() == krbn::absolute_time_point(4000));
      REQUIRE(!manager->find_current_grabbable_state(krbn::device_id(2)));
    }

    // Ignore events after first_grabbed_event_time_stamp_.
    {
      krbn::grabbable_state state(krbn::device_id(1),
                                  krbn::grabbable_state::state::grabbable,
                                  krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                  krbn::absolute_time_point(6000));
      REQUIRE(!manager->update_grabbable_state(state));
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      REQUIRE(manager->find_current_grabbable_state(krbn::device_id(1))->get_time_stamp() == krbn::absolute_time_point(4000));
      REQUIRE(!manager->find_current_grabbable_state(krbn::device_id(2)));
    }

    // Ignore events after first_grabbed_event_time_stamp_.
    {
      krbn::event_queue::queue event_queue;
      event_queue.emplace_back_entry(krbn::device_id(1),
                                     krbn::event_queue::event_time_stamp(krbn::absolute_time_point(4000)),
                                     krbn::event_queue::event(krbn::key_code::a),
                                     krbn::event_type::key_down,
                                     krbn::event_queue::event(krbn::key_code::a));
      REQUIRE(!manager->update_first_grabbed_event_time_stamp(event_queue));
      REQUIRE(manager->find_current_grabbable_state(krbn::device_id(1))->get_time_stamp() == krbn::absolute_time_point(4000));
      REQUIRE(!manager->find_current_grabbable_state(krbn::device_id(2)));
    }

    manager = nullptr;
  }
}

TEST_CASE("terminate") {
  krbn::dispatcher_utility::terminate_dispatchers();
}
