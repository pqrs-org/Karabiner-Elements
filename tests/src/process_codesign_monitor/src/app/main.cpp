#include <csignal>
#include <iostream>
#include <pqrs/osx/application.hpp>
#include <unistd.h>

#ifndef PROCESS_CODESIGN_TEST_HELPER_VARIANT
#error PROCESS_CODESIGN_TEST_HELPER_VARIANT is not defined
#endif

namespace {
volatile std::sig_atomic_t running = 1;

void signal_handler(int) {
  running = 0;
}
} // namespace

int main() {
  pqrs::osx::application::finish_launching();

  std::signal(SIGTERM, signal_handler);
  std::signal(SIGINT, signal_handler);

  constexpr char variant[] = PROCESS_CODESIGN_TEST_HELPER_VARIANT;
  std::cout << variant << std::flush;
  if (!std::cout) {
    return 1;
  }

  while (running) {
    pause();
  }

  return 0;
}
