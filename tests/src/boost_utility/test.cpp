#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "boost_utility.hpp"

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
