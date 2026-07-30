#include "console_user_server/runtime.h"
#include "console_user_server/components_manager.hpp"
#include "console_user_server/ui_bridge.h"
#include "console_user_server/ui_bridge.hpp"
#include "constants.hpp"
#include "dispatcher_utility.hpp"
#include "environment_variable_utility.hpp"
#include "filesystem_utility.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "process_lifecycle_manager.hpp"
#include "run_loop_thread_utility.hpp"
#include "services_utility.hpp"
#include "update_utility.hpp"
#include <pqrs/filesystem.hpp>

namespace {
std::shared_ptr<krbn::dispatcher_utility::scoped_dispatcher_manager> scoped_dispatcher_manager;
std::shared_ptr<krbn::run_loop_thread_utility::scoped_run_loop_thread_manager> scoped_run_loop_thread_manager;
std::shared_ptr<krbn::console_user_server::ui_bridge> ui_bridge_instance;
bool started = false;
} // namespace

void console_user_server_start(console_user_server_terminated_callback callback) {
  if (started) {
    return;
  }
  started = true;

  //
  // Initialize
  //

  scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();
  scoped_run_loop_thread_manager = krbn::run_loop_thread_utility::initialize_scoped_run_loop_thread_manager(
      pqrs::cf::run_loop_thread::failure_policy::exit);
  ui_bridge_instance = std::make_shared<krbn::console_user_server::ui_bridge>();

  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);

  umask(0022);

  pqrs::osx::process_info::enable_sudden_termination();

  //
  // Load custom environment variables
  //

  auto environment_variables = krbn::environment_variable_utility::load_custom_environment_variables();

  //
  // Setup logger
  //

  if (!krbn::constants::get_user_log_directory().empty()) {
    krbn::logger::set_async_rotating_logger("console_user_server",
                                            krbn::constants::get_user_log_directory() / "console_user_server.log",
                                            pqrs::spdlog::filesystem::log_directory_perms_0700);
  }

  krbn::logger::get_logger()->info("version {0}", karabiner_version);

  //
  // Log custom environment variables
  //

  krbn::environment_variable_utility::log(environment_variables);

  //
  // Get codesign
  //

  krbn::get_shared_codesign_manager()->log();

  //
  // Activate driver
  //

  system("/Applications/.Karabiner-VirtualHIDDevice-Manager.app/Contents/MacOS/Karabiner-VirtualHIDDevice-Manager forceActivate &");

  //
  // Register services
  //

  krbn::services_utility::bootout_old_agents();
  // Register services when console_user_server starts to avoid missing registrations,
  // for example when new services are added in an update.
  krbn::services_utility::register_core_daemons();
  krbn::services_utility::register_core_agents();

  //
  // Create directories
  //

  krbn::filesystem_utility::prepare_user_directories();

  //
  // Run process_lifecycle_manager
  //

  krbn::process_lifecycle_manager::initialize_shared_instance(
      krbn::process_lifecycle_manager::configuration{
          .components_manager_maker =
              [ui_bridge = ui_bridge_instance] {
                return std::make_unique<krbn::console_user_server::components_manager>(ui_bridge);
              },
          .termination_completion_handler =
              [callback] {
                if (callback) {
                  callback();
                }
              },
      });
  krbn::process_lifecycle_manager::async_start();
}

void console_user_server_terminate(void) {
  if (!started) {
    return;
  }

  //
  // Cleanup
  //

  krbn::process_lifecycle_manager::terminate_shared_instance();
  ui_bridge_instance = nullptr;
  scoped_run_loop_thread_manager = nullptr;
  scoped_dispatcher_manager = nullptr;
  krbn::logger::get_logger()->info("karabiner_console_user_server is terminated.");
  started = false;
}

void console_user_server_register_ui_state_callback(console_user_server_string_callback callback) {
  if (ui_bridge_instance) {
    ui_bridge_instance->register_ui_state_callback(callback);
  }
}

void console_user_server_register_notification_message_callback(console_user_server_string_callback callback) {
  if (ui_bridge_instance) {
    ui_bridge_instance->register_notification_message_callback(callback);
  }
}

void console_user_server_select_profile(size_t index) {
  if (ui_bridge_instance) {
    ui_bridge_instance->select_profile(index);
  }
}

void console_user_server_launch_settings(void) {
  krbn::application_launcher::launch_settings();
}

void console_user_server_launch_event_viewer(void) {
  krbn::application_launcher::launch_event_viewer();
}

void console_user_server_check_for_updates(bool include_beta_versions) {
  if (include_beta_versions) {
    krbn::update_utility::check_for_updates_with_beta_version();
  } else {
    krbn::update_utility::check_for_updates_stable_only();
  }
}

void console_user_server_restart(void) {
  krbn::services_utility::restart_console_user_server_agent();
}

void console_user_server_quit(void) {
  krbn::application_launcher::killall_settings();
  krbn::services_utility::unregister_multitouch_extension_agent();
  // This unregisters console_user_server itself, so it must be last.
  krbn::services_utility::unregister_core_agents();
}
