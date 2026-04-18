#include "core_service/main/agent.hpp"
#include "core_service/main/daemon.hpp"
#include "dispatcher_utility.hpp"
#include "run_loop_thread_utility.hpp"
#include <IOKit/hidsystem/IOHIDLib.h>
#include <pqrs/osx/process_info.hpp>
#include <vector>

int main(int argc, const char* argv[]) {
  std::vector<std::string> args;
  args.reserve(static_cast<std::size_t>(argc));

  for (int i = 0; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }

  //
  // Initialize
  //

  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();
  auto scoped_run_loop_thread_manager = krbn::run_loop_thread_utility::initialize_scoped_run_loop_thread_manager(
      pqrs::cf::run_loop_thread::failure_policy::exit);

  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);

  umask(0022);

  pqrs::osx::process_info::enable_sudden_termination();

  //
  // Check euid
  // (core_service is launched from LaunchDaemons (root) and LaunchAgents (user).)
  //

  bool root = (geteuid() == 0);

  if (root) {
    return krbn::core_service::main::daemon();
  } else {
    return krbn::core_service::main::agent(args);
  }
}
