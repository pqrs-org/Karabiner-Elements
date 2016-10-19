#pragma once

#include <thread>

class thread_utility final {
public:
  static std::thread::id get_main_thread_id(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static std::thread::id id = std::this_thread::get_id();
    return id;
  }

  static void register_main_thread(void) {
    get_main_thread_id();
  }

  static bool is_main_thread(void) {
    return get_main_thread_id() == std::this_thread::get_id();
  }
};
