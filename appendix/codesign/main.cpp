#include "codesign.hpp"
#include "logger.hpp"
#include "thread_utility.hpp"
#include <chrono>
#include <iostream>
#include <thread>

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  if (argc > 1) {
    auto common_name = krbn::codesign::get_common_name_of_process(atoi(argv[1]));
    krbn::logger::get_logger().info("common_name: {0}", common_name ? *common_name : "null");
  }

  return 0;
}
