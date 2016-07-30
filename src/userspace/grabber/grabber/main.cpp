#include "grabber.hpp"

int main(int argc, const char* argv[]) {
  event_grabber observer;

  grabber_server server;
  std::thread th = server.start();

  CFRunLoopRun();
  return 0;
}
