#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "dispatcher_utility.hpp"
#include "grabbable_state_queue.hpp"

TEST_CASE("initialize") {
  krbn::dispatcher_utility::initialize_dispatchers();
}

TEST_CASE("grabbable_state_queue") {
  {
    auto queue = std::make_unique<krbn::grabbable_state_queue>();

    std::optional<krbn::grabbable_state> last_changed_grabbable_state;
    int grabbable_state_changed_count = 0;

    queue->grabbable_state_changed.connect([&](auto&& grabbable_state) {
      last_changed_grabbable_state = grabbable_state;
      ++grabbable_state_changed_count;
    });

    REQUIRE(!queue->find_current_grabbable_state());
    REQUIRE(!queue->get_first_grabbed_event_time_stamp());

    queue->clear();
    REQUIRE(grabbable_state_changed_count == 0);

    for (krbn::absolute_time_point time_stamp(1000); time_stamp < krbn::absolute_time_point(10000); time_stamp += krbn::absolute_time_duration(1000)) {
      krbn::grabbable_state state(krbn::device_id(1),
                                  krbn::grabbable_state::state::grabbable,
                                  krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                  time_stamp);
      REQUIRE(queue->push_back_grabbable_state(state));

      REQUIRE(*(queue->find_current_grabbable_state()) == state);
    }

    // Check `grabbable_state_changed` signal
    {
      krbn::grabbable_state expected(krbn::device_id(1),
                                     krbn::grabbable_state::state::grabbable,
                                     krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                     krbn::absolute_time_point(1000));
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      REQUIRE(last_changed_grabbable_state == expected);
      REQUIRE(grabbable_state_changed_count == 1);
    }

    // Ignore too small time_stamp from first_grabbed_event_time_stamp_.
    REQUIRE(!queue->update_first_grabbed_event_time_stamp(krbn::absolute_time_point(0)));

    // Remove states after first_grabbed_event_time_stamp_.
    REQUIRE(queue->update_first_grabbed_event_time_stamp(krbn::absolute_time_point(5000)));
    REQUIRE(*(queue->get_first_grabbed_event_time_stamp()) == krbn::absolute_time_point(5000));
    REQUIRE(queue->find_current_grabbable_state()->get_time_stamp() == krbn::absolute_time_point(4000));

    REQUIRE(!queue->update_first_grabbed_event_time_stamp(krbn::absolute_time_point(3000)));
    REQUIRE(*(queue->get_first_grabbed_event_time_stamp()) == krbn::absolute_time_point(5000));
    REQUIRE(queue->find_current_grabbable_state()->get_time_stamp() == krbn::absolute_time_point(4000));

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    REQUIRE(grabbable_state_changed_count == 1);

    // Ignore events after first_grabbed_event_time_stamp_.
    {
      krbn::grabbable_state state(krbn::device_id(1),
                                  krbn::grabbable_state::state::grabbable,
                                  krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                  krbn::absolute_time_point(6000));
      REQUIRE(!queue->push_back_grabbable_state(state));
      REQUIRE(queue->find_current_grabbable_state()->get_time_stamp() == krbn::absolute_time_point(4000));
    }
    {
      krbn::grabbable_state state(krbn::device_id(1),
                                  krbn::grabbable_state::state::ungrabbable_temporarily,
                                  krbn::grabbable_state::ungrabbable_temporarily_reason::key_repeating,
                                  krbn::absolute_time_point(4500));
      REQUIRE(queue->push_back_grabbable_state(state));
      REQUIRE(queue->find_current_grabbable_state()->get_time_stamp() == krbn::absolute_time_point(4500));

      // Check `grabbable_state_changed` signal
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      REQUIRE(last_changed_grabbable_state == state);
      REQUIRE(grabbable_state_changed_count == 2);
    }

    // Unset first_grabbed_event_time_stamp_.
    queue->unset_first_grabbed_event_time_stamp();
    REQUIRE(!queue->get_first_grabbed_event_time_stamp());

    REQUIRE(queue->update_first_grabbed_event_time_stamp(krbn::absolute_time_point(3000)));
    REQUIRE(*(queue->get_first_grabbed_event_time_stamp()) == krbn::absolute_time_point(3000));
    REQUIRE(queue->find_current_grabbable_state()->get_time_stamp() == krbn::absolute_time_point(2000));

    // Check `grabbable_state_changed` signal
    {
      krbn::grabbable_state expected(krbn::device_id(1),
                                     krbn::grabbable_state::state::grabbable,
                                     krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                     krbn::absolute_time_point(2000));
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      REQUIRE(last_changed_grabbable_state == expected);
      REQUIRE(grabbable_state_changed_count == 3);
    }

    // Clear
    queue->clear();

    // Check `grabbable_state_changed` signal
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    REQUIRE(last_changed_grabbable_state == std::nullopt);
    REQUIRE(grabbable_state_changed_count == 4);

    queue = nullptr;
  }
}

TEST_CASE("grabbable_state_queue.circular_buffer") {
  auto queue = std::make_unique<krbn::grabbable_state_queue>();

  for (int i = 0; i < 10000; ++i) {
    queue->push_back_grabbable_state(krbn::grabbable_state(krbn::device_id(1),
                                                           krbn::grabbable_state::state::grabbable,
                                                           krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                                           krbn::absolute_time_point(i)));

    REQUIRE(queue->find_current_grabbable_state()->get_time_stamp() == krbn::absolute_time_point(i));
  }

  queue = nullptr;
}

TEST_CASE("terminate") {
  krbn::dispatcher_utility::terminate_dispatchers();
}
