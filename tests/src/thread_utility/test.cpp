#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "thread_utility.hpp"

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("timer") {
  {
    size_t count = 0;

    krbn::thread_utility::timer timer(
        std::chrono::milliseconds(100),
        false,
        [&] {
          ++count;
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    timer.cancel();

    REQUIRE(count == 1);
  }

  // Wait

  {
    size_t count = 0;
    size_t wait_count = 0;

    krbn::thread_utility::timer timer(
        std::chrono::milliseconds(500),
        false,
        [&] {
          ++count;
        });

    auto thread = std::thread([&] {
      for (int i = 0; i < 5; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        ++wait_count;
      }
    });

    timer.wait();

    REQUIRE(count == 1);
    REQUIRE(wait_count > 1);

    thread.join();
  }

  // Cancel (1)

  {
    size_t count = 0;

    krbn::thread_utility::timer timer(
        std::chrono::milliseconds(500),
        false,
        [&] {
          ++count;
        });

    timer.cancel();

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    REQUIRE(count == 0);
  }

  // Cancel (2)

  {
    size_t count = 0;

    {
      krbn::thread_utility::timer timer(
          std::chrono::milliseconds(50000),
          false,
          [&] {
            ++count;
          });
    }
  }

  // Cancel while wait

  {
    size_t count = 0;

    krbn::thread_utility::timer timer(
        std::chrono::milliseconds(500),
        false,
        [&] {
          ++count;
        });

    auto thread = std::thread([&] {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      timer.cancel();
    });

    timer.wait();

    REQUIRE(count == 0);

    thread.join();
  }
}

TEST_CASE("timer.repeats") {
  // Cancel while wait

  {
    size_t count = 0;

    krbn::thread_utility::timer timer(
        std::chrono::milliseconds(100),
        true,
        [&] {
          ++count;
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    timer.cancel();
    timer.wait();

    REQUIRE(count >= 2);
    REQUIRE(count <= 10);
  }

  {
    size_t count = 0;

    // Call `cancel` in `~timer`.

    {
      krbn::thread_utility::timer timer(
          std::chrono::milliseconds(100),
          true,
          [&] {
            ++count;
          });

      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    REQUIRE(count >= 2);
    REQUIRE(count <= 10);
  }
}
