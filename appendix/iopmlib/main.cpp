#include "iopm_client.hpp"
#include "logger.hpp"

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  krbn::iopm_client client([](uint32_t message_type) {
    krbn::logger::get_logger().info("callback message_type:{0}", message_type);
  });

  CFRunLoopRun();

  return 0;
}
