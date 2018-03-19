#include "configuration_monitor.hpp"
#include "constants.hpp"
#include "logger.hpp"
#include "system_preferences_monitor.hpp"
#include <iostream>

namespace {
void system_preferences_updated_callback(const krbn::system_preferences& system_preferences) {
  std::cout << "system_preferences_updated_callback:" << std::endl;
  std::cout << "  com.apple.keyboard.fnState: " << system_preferences.get_keyboard_fn_state() << std::endl;
  std::cout << "  com.apple.swipescrolldirection: " << system_preferences.get_swipe_scroll_direction() << std::endl;
  std::cout << "  keyboard_type: " << static_cast<int>(system_preferences.get_keyboard_type()) << std::endl;
}
} // namespace

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  auto configuration_monitor = std::make_shared<krbn::configuration_monitor>(krbn::constants::get_user_core_configuration_file_path(),
                                                                             [](auto core_configuration) {});
  krbn::system_preferences_monitor monitor(system_preferences_updated_callback, configuration_monitor);

  CFRunLoopRun();

  return 0;
}
