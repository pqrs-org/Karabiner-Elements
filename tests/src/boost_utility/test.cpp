#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "boost_utility.hpp"
#include "thread_utility.hpp"

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("signals2_combiner_call_while_true") {
  boost::signals2::signal<bool(void), krbn::boost_utility::signals2_combiner_call_while_true> signal;

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

  krbn::boost_utility::signals2_connections signals2_connections;

  // ============================================================
  // `wait_disconnect_all_connections` for empty connections.
  // ============================================================

  {
    signals2_connections.wait_disconnect_all_connections();
  }

  // ============================================================
  // `disconnect_all_connections` and `wait_disconnect_all_connections`.
  // ============================================================

  {
    counter = 0;

    {
      auto c = signal.connect([&] {
        ++counter;
      });
      signals2_connections.push_back(c);
    }

    signal();
    REQUIRE(counter == 1);

    std::thread thread([&] {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));

      signals2_connections.disconnect_all_connections();
    });

    signal();
    REQUIRE(counter == 2);

    signals2_connections.wait_disconnect_all_connections();

    signal();
    REQUIRE(counter == 2);

    thread.join();
  }

  // ============================================================
  // `disconnect_all_connections` and `wait_disconnect_all_connections` in the same thread.
  // ============================================================

  {
    counter = 0;

    {
      auto c = signal.connect([&] {
        ++counter;
      });
      signals2_connections.push_back(c);
    }

    signal();
    REQUIRE(counter == 1);

    signals2_connections.disconnect_all_connections();
    signals2_connections.wait_disconnect_all_connections();

    signal();
    REQUIRE(counter == 1);
  }
}
