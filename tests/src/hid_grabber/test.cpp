#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "hid_grabber.hpp"
#include "thread_utility.hpp"

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("signal2_combiner_call_while_grabbable") {
  boost::signals2::signal<krbn::grabbable_state::state(void),
                          krbn::hid_grabber::signal2_combiner_call_while_grabbable>
      signal;

  REQUIRE(signal() == krbn::grabbable_state::state::grabbable);

  int counter = 0;

  signal.connect([&] {
    ++counter;
    return krbn::grabbable_state::state::grabbable;
  });

  signal.connect([&] {
    ++counter;
    return krbn::grabbable_state::state::grabbable;
  });

  signal.connect([&] {
    ++counter;
    return krbn::grabbable_state::state::ungrabbable_temporarily;
  });

  signal.connect([&] {
    // never called
    ++counter;
    return krbn::grabbable_state::state::ungrabbable_permanently;
  });

  REQUIRE(signal() == krbn::grabbable_state::state::ungrabbable_temporarily);
  REQUIRE(counter == 3);
}
