// OS X headers
#include <CoreFoundation/CoreFoundation.h>

#include <iostream>

#include "system_preferences.hpp"

int main(int argc, const char* argv[]) {
  std::cout << "com.apple.keyboard.fnState: " << system_preferences::get_keyboard_fn_state() << std::endl;
  std::cout << "InitialKeyRepeat: " << system_preferences::get_initial_key_repeat_milliseconds() << std::endl;
  std::cout << "KeyRepeat: " << system_preferences::get_key_repeat_milliseconds() << std::endl;
  return 0;
}
