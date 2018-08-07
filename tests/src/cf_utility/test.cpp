#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "cf_utility.hpp"
#include "logger.hpp"
#include "thread_utility.hpp"

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("run_loop_thread") {
  for (int i = 0; i < 5000; ++i) {
    __block int count1 = 0;
    __block int count2 = 0;

    auto thread1 = std::make_shared<krbn::cf_utility::run_loop_thread>();
    auto thread2 = std::make_shared<krbn::cf_utility::run_loop_thread>();

    // thread1

    for (int j = 0; j < 5; ++j) {
      thread1->enqueue(^{
        ++count1;
        // krbn::logger::get_logger().info("thread1 {0} {1}", j, count1);
      });
    }

    // thread2

    thread2->enqueue(^{
      ++count2;
      // krbn::logger::get_logger().info("thread2 {0}", count2);
    });

    thread1 = nullptr;
    thread2 = nullptr;

    REQUIRE(count1 == 5);
    REQUIRE(count2 == 1);
  }
}
