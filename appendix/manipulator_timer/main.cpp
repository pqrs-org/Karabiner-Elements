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

  manipulator_timer->timer_invoked.connect([](auto&& entry) {
    krbn::logger::get_logger().info("timer_id:{0}, when:{1}",
                                    type_safe::get(entry.get_timer_id()),
                                    type_safe::get(entry.get_when()));
  });

  for (int i = -10; i < 10; ++i) {
    auto now = krbn::time_utility::mach_absolute_time();
    auto when = now + krbn::time_utility::milliseconds_to_absolute(std::chrono::milliseconds(1000 * i));
    krbn::manipulator::manipulator_timer::entry entry(when);
    manipulator_timer->enqueue(entry, now);
  }

  CFRunLoopRun();

  manipulator_timer = nullptr;

  return 0;
}
