#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "thread_utility.hpp"

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("wait") {
  std::cout << "wait" << std::endl;

  for (int i = 0; i < 1000; ++i) {
    krbn::thread_utility::wait wait;
    std::atomic<int> count(0);

    std::thread thread([&] {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));

      REQUIRE(count == 0);
      wait.notify();
    });

    wait.wait_notice();
    count = 1;

    thread.join();
  }
}
