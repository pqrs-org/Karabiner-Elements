#include "configuration_monitor.hpp"
#include "constants.hpp"
#include "logger.hpp"
#include "system_preferences_monitor.hpp"
#include <iostream>

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  signal(SIGINT, [](int) {
    CFRunLoopStop(CFRunLoopGetMain());
  });

  auto configuration_monitor = std::make_shared<krbn::configuration_monitor>(krbn::constants::get_user_core_configuration_file_path());

  krbn::system_preferences_monitor monitor(configuration_monitor);

  monitor.system_preferences_changed.connect([](auto&& system_preferences) {
    std::cout << "system_preferences_updated_callback:" << std::endl;
    std::cout << "  com.apple.keyboard.fnState: " << system_preferences.get_keyboard_fn_state() << std::endl;
    std::cout << "  com.apple.swipescrolldirection: " << system_preferences.get_swipe_scroll_direction() << std::endl;
    std::cout << "  keyboard_type: " << static_cast<int>(system_preferences.get_keyboard_type()) << std::endl;
  });

  monitor.async_start();

  configuration_monitor->async_start();

  CFRunLoopRun();

  configuration_monitor = nullptr;

  return 0;
}
