#include <chrono>
#include <iostream>
#include <thread>

#include "hid_system_client.hpp"

class logger final {
public:
  static spdlog::logger& get_logger(void) {
    static std::shared_ptr<spdlog::logger> logger;
    if (!logger) {
      logger = spdlog::stdout_logger_mt("post_key", true);
    }
    return *logger;
  }
};

int main(int argc, const char* argv[]) {
  hid_system_client client(logger::get_logger());

  auto t = std::thread([&] {
    using namespace std::chrono_literals;

    for (;;) {
      std::this_thread::sleep_for(1s);

      std::cout << "shift" << std::endl;
      if (auto key_code = krbn::types::get_key_code("left_shift")) {
        client.post_modifier_flags(*key_code, NX_SHIFTMASK | NX_DEVICELSHIFTKEYMASK, krbn::keyboard_type::none);
        client.post_modifier_flags(*key_code, 0, krbn::keyboard_type::none);
      }

      std::cout << "mission control" << std::endl;
      client.post_key(0xa0, krbn::event_type::key_down, 0, krbn::keyboard_type::none, false);
      client.post_key(0xa0, krbn::event_type::key_up, 0, krbn::keyboard_type::none, false);

      std::this_thread::sleep_for(1s);

      std::cout << "mission control" << std::endl;
      client.post_key(0xa0, krbn::event_type::key_down, 0, krbn::keyboard_type::none, false);
      client.post_key(0xa0, krbn::event_type::key_up, 0, krbn::keyboard_type::none, false);

      std::this_thread::sleep_for(5s);
    }

  });

  CFRunLoopRun();

  return 0;
}
