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
      REQUIRE(!timer.fired());

      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      REQUIRE(value == 1);

      REQUIRE(timer.fired());
    }

    {
      class wrapper final {
      public:
        std::shared_ptr<krbn::gcd_utility::main_queue_after_timer> timer;
      };

      wrapper w;
      wrapper* p = &w;
      w.timer = std::make_shared<krbn::gcd_utility::main_queue_after_timer>(dispatch_time(DISPATCH_TIME_NOW, 100 * NSEC_PER_MSEC),
                                                                            ^{
                                                                              // w is copied before timer was constructed.
                                                                              REQUIRE(w.timer.get() == nullptr);

                                                                              // p refers `w` in the timer.
                                                                              REQUIRE(p->timer);
                                                                              REQUIRE(p->timer->fired());

                                                                              ++value;
                                                                            });
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      REQUIRE(value == 2);
    }

    {
      class wrapper final {
      public:
        wrapper(void) : value_(0) {
        }

        void set_timer(void) {
          timer_ = std::make_unique<krbn::gcd_utility::main_queue_after_timer>(dispatch_time(DISPATCH_TIME_NOW, 100 * NSEC_PER_MSEC),
                                                                               ^{
                                                                                 // block binds `this`.

                                                                                 REQUIRE(timer_);
                                                                                 REQUIRE(timer_->fired());

                                                                                 ++value_;
                                                                               });
        }

        int get_value(void) const {
          return value_;
        }

      private:
        std::unique_ptr<krbn::gcd_utility::main_queue_after_timer> timer_;
        int value_;
      };

      wrapper w;
      REQUIRE(w.get_value() == 0);

      w.set_timer();

      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      REQUIRE(w.get_value() == 1);
    }

    {
      {
        krbn::gcd_utility::main_queue_after_timer timer(dispatch_time(DISPATCH_TIME_NOW, 100 * NSEC_PER_MSEC),
                                                        ^{
                                                          ++value;
                                                        });
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // timer will be canceled.
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
