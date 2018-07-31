#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "thread_utility.hpp"

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("timer") {
  {
    size_t count = 0;

    krbn::thread_utility::timer timer(std::chrono::milliseconds(100),
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

    krbn::thread_utility::timer timer(std::chrono::milliseconds(500),
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

    krbn::thread_utility::timer timer(std::chrono::milliseconds(500),
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
      krbn::thread_utility::timer timer(std::chrono::milliseconds(50000),
                                        [&] {
                                          ++count;
                                        });
    }
  }
}
