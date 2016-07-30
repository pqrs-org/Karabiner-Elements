#include "grabber.hpp"

int main(int argc, const char* argv[]) {
  event_grabber observer;
  grabber_server server;
  server.start();
  CFRunLoopRun();
  return 0;
}
