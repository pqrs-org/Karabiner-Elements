#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "event_queue.hpp"
#include "thread_utility.hpp"

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
