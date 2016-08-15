#include "connection_manager.hpp"

int main(int argc, const char* argv[]) {
  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);

  connection_manager manager;

  CFRunLoopRun();
  return 0;
}
