#pragma once

#include <cstdlib>
#include <pqrs/osx/workspace.hpp>
#include <spdlog/fmt/fmt.h>
#include <sstream>

namespace krbn {
class application_launcher final {
public:
  static void launch_app_icon_switcher(void) {
    // Note:
    // Updating the icon may trigger Spotlight index updates, so only call it when necessary.
    // (Avoid calling it every time, such as during grabber startup.)
    system("'/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-AppIconSwitcher.app/Contents/MacOS/Karabiner-AppIconSwitcher'");
  }

  static void launch_event_viewer(void) {
    pqrs::osx::workspace::open_application_by_file_path("/Applications/Karabiner-EventViewer.app");
  }

  static void launch_settings(void) {
    pqrs::osx::workspace::open_application_by_file_path("/Applications/Karabiner-Elements.app");
  }

  static void killall_settings(void) {
    system("killall Karabiner-Elements");
  }

  static void launch_uninstaller(void) {
    // Use nohup because uninstaller kill the Settings Window.
    system("/usr/bin/nohup osascript '/Library/Application Support/org.pqrs/Karabiner-Elements/scripts/uninstaller.applescript' >/dev/null 2>&1 &");
  }
};
} // namespace krbn
