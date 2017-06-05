#include "../include/logger.hpp"
#include "iopm_client.hpp"

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  krbn::iopm_client client(logger::get_logger(), [](uint32_t message_type) {
    logger::get_logger().info("callback message_type:{0}", message_type);
  });

  CFRunLoopRun();

  return 0;
}
