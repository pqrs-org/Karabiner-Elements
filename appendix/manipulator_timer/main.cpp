#include "logger.hpp"
#include "manipulator/manipulator_timer.hpp"
#include "thread_utility.hpp"
#include <CoreFoundation/CoreFoundation.h>

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  krbn::manipulator::manipulator_timer::get_instance().enable();

  krbn::manipulator::manipulator_timer::get_instance().timer_invoked.connect([](auto timer_id, auto now) {
    krbn::logger::get_logger().info("timer_id:{0}, now:{1}", static_cast<uint64_t>(timer_id), now);
    if (timer_id == krbn::manipulator::manipulator_timer::timer_id(10)) {
      exit(0);
    }
  });

  for (int i = 0; i < 10; ++i) {
    krbn::manipulator::manipulator_timer::get_instance().add_entry(dispatch_time(DISPATCH_TIME_NOW, i * 300 * NSEC_PER_MSEC));
  }

  CFRunLoopRun();
  return 0;
}
