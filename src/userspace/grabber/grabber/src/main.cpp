#include "grabber.hpp"

int main(int argc, const char* argv[]) {
  logger::get_logger()->info("hello!");

  event_grabber observer;

  grabber_server server;
  std::thread th = server.start();

  userspace_connection_manager connection_manager;

  CFRunLoopRun();
  return 0;
}
