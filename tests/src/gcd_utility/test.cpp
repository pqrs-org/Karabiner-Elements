#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "gcd_utility.hpp"
#include "thread_utility.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

TEST_CASE("main_queue_after_timer") {
  auto thread = std::thread([] {
    std::atomic<int> v(0);
    std::atomic<int>& __block value = v;

    {
      krbn::gcd_utility::main_queue_after_timer timer(dispatch_time(DISPATCH_TIME_NOW, 100 * NSEC_PER_MSEC),
                                                      ^{
                                                        ++value;
                                                      });
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      REQUIRE(value == 1);
    }

    {
      krbn::gcd_utility::main_queue_after_timer timer(dispatch_time(DISPATCH_TIME_NOW, 100 * NSEC_PER_MSEC),
                                                      ^{
                                                        ++value;
                                                      });
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      REQUIRE(value == 2);
    }

    {
      {
        krbn::gcd_utility::main_queue_after_timer main_queue_after_timer(dispatch_time(DISPATCH_TIME_NOW, 100 * NSEC_PER_MSEC),
                                                                         ^{
                                                                           ++value;
                                                                         });
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
      }
      REQUIRE(value == 2);

      // The dispatch_after_timer is not called after destructed.
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      REQUIRE(value == 2);
    }

    exit(0);
  });

  CFRunLoopRun();
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
