#include <csignal>
#include <dispatch/dispatch.h>
#include <launchctl_utility.hpp>
#include <pqrs/karabiner/driverkit/virtual_hid_device_service.hpp>
#include <thread>

int main(int argc, const char* argv[]) {
  std::signal(SIGTERM, SIG_IGN);

  {
    dispatch_source_t sigterm_source = dispatch_source_create(DISPATCH_SOURCE_TYPE_SIGNAL, SIGTERM, 0, dispatch_get_main_queue());
    dispatch_source_set_event_handler(sigterm_source, ^{
      // macOS Big Sur 11.0 - 11.2 have an issue that causes kernel panic if virtual HID devices exist at shutdown.
      // So we have to detach these devices before shutdown.
      //
      // Shutting down console_user_server performs the device termination.

      krbn::launchctl_utility::bootout_console_user_server();

      for (int i = 0; i < 10; ++i) {
        if (!pqrs::karabiner::driverkit::virtual_hid_device_service::utility::virtual_hid_keyboard_exists() &&
            !pqrs::karabiner::driverkit::virtual_hid_device_service::utility::virtual_hid_pointing_exists()) {
          break;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
      }

      exit(0);
    });
    dispatch_resume(sigterm_source);
  }

  dispatch_main();

  return 0;
}
