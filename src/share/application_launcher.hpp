#pragma once

#include <cstdlib>
#include <iostream>
#include <pqrs/osx/workspace.hpp>
#include <pqrs/process.hpp>
#include <spdlog/fmt/fmt.h>
#include <sstream>
#include <sys/wait.h>

namespace krbn {
class application_launcher final {
public:
  static void launch_app_icon_switcher(void) {
    // Note:
    // Updating the icon may trigger Spotlight index updates, so only call it when necessary.
    // (Avoid calling it every time, such as during core_service startup.)
    system("'/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-AppIconSwitcher.app/Contents/MacOS/Karabiner-AppIconSwitcher'");
  }

  static void launch_event_viewer(void) {
    pqrs::osx::workspace::open_application_by_file_path("/Applications/Karabiner-EventViewer.app");
  }

  static void launch_settings(void) {
    pqrs::osx::workspace::open_application_by_file_path("/Applications/Karabiner-Elements.app");
  }

  static void killall_settings(void) {
    system("/usr/bin/killall Karabiner-Elements");
  }

  static void killall_system_settings(void) {
    system("/usr/bin/killall 'System Settings'");
    wait_for_process_termination("System Settings");
  }

  static void wait_for_process_termination(const std::string_view name) {
    auto pgrep = fmt::format("/usr/bin/pgrep -x -U {0} '{1}'",
                             getuid(),
                             name);
    for (int i = 0; i < 5; ++i) {
      if (auto exit_code = pqrs::process::system(pgrep)) {
        // exit_codes
        //
        // - 0: One or more processes were matched.
        // - 1: No processes were matched.
        // - 2: Invalid options were specified on the command line.
        // - 3: An internal error occurred.
        if (*exit_code == 1) {
          break;
        }
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // Since Launch Services may still hold onto entries right after the process ends, we wait briefly.
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  static void launch_uninstaller(void) {
    // Use nohup because uninstaller kill the Settings Window.
    system("/usr/bin/nohup osascript '/Library/Application Support/org.pqrs/Karabiner-Elements/scripts/uninstaller.applescript' >/dev/null 2>&1 &");
  }
};
} // namespace krbn
