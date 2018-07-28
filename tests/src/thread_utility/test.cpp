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

  // Cancel

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
}
