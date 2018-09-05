#include "logger.hpp"
#include "manipulator/manipulator_timer.hpp"
#include "thread_utility.hpp"
#include <CoreFoundation/CoreFoundation.h>

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  signal(SIGINT, [](int) {
    CFRunLoopStop(CFRunLoopGetMain());
  });

  auto manipulator_timer = std::make_unique<krbn::manipulator::manipulator_timer>();

  for (int i = -10; i <= 10; ++i) {
    auto now = krbn::time_utility::mach_absolute_time();
    auto now_ms = krbn::time_utility::to_milliseconds(now);
    auto when_ms = std::chrono::milliseconds(now_ms.count() + 1000 * i);
    auto when = krbn::time_utility::to_absolute_time(when_ms);

    manipulator_timer->enqueue(
        [i, when] {
          krbn::logger::get_logger().info("i:{0} when:{1}",
                                          i,
                                          type_safe::get(when));

          if (i == 10) {
            CFRunLoopStop(CFRunLoopGetMain());
          }
        },
        when);
    manipulator_timer->async_invoke(now);
  }

  CFRunLoopRun();

  manipulator_timer = nullptr;

  return 0;
}
