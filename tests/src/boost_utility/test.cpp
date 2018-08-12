#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "boost_utility.hpp"
#include "thread_utility.hpp"

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("signals2_combiner_call_while_true") {
  boost::signals2::signal<bool(void),
                          krbn::boost_utility::signals2_combiner_call_while_true>
      signal;

  REQUIRE(signal() == true);

  int counter = 0;

  signal.connect([&] {
    ++counter;
    return true;
  });

  signal.connect([&] {
    ++counter;
    return true;
  });

  signal.connect([&] {
    ++counter;
    return false;
  });

  signal.connect([&] {
    // never called
    ++counter;
    return true;
  });

  REQUIRE(signal() == false);
  REQUIRE(counter == 3);
}

TEST_CASE("signals2_connections") {
  boost::signals2::signal<void(void)> signal;
  int counter = 0;

  // Connect a slot

  {
    signal.connect([&] {
      ++counter;
      return true;
    });

    signal();
    REQUIRE(counter == 1);
  }

  // Connect slots and disconnect them by `signals2_connections`.

  {
    krbn::boost_utility::signals2_connections signals2_connections;

    signals2_connections.push_back(
        signal.connect([&] {
          ++counter;
          return true;
        }));

    signals2_connections.push_back(
        signal.connect([&] {
          ++counter;
          return true;
        }));

    signal();
    REQUIRE(counter == 4);
  }

  // Run the first slot.

  {
    signal();
    REQUIRE(counter == 5);
  }
}
