#pragma once

#include <cstdlib>
#include <pqrs/osx/workspace.hpp>

namespace krbn {
class application_launcher final {
public:
  static void launch_app_icon_switcher() {
    // Note:
    // Updating the icon may trigger Spotlight index updates, so only call it when necessary.
    // (Avoid calling it every time, such as during core_service startup.)
    system("'/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-AppIconSwitcher.app/Contents/MacOS/Karabiner-AppIconSwitcher'");
  }

  static void launch_event_viewer() {
    pqrs::osx::workspace::open_application_by_bundle_path("/Applications/Karabiner-EventViewer.app");
  }

  static void launch_settings() {
    pqrs::osx::workspace::open_application_by_bundle_path("/Applications/Karabiner-Elements.app");
  }

  static void launch_settings_without_activation() {
    pqrs::osx::workspace::open_application_by_bundle_path(
        "/Applications/Karabiner-Elements.app",
        pqrs::osx::workspace::open_configuration{
            .activates = false,
        });
  }

  static void killall_settings() {
    system("/usr/bin/killall Karabiner-Elements");
  }

  static void launch_uninstaller() {
    // Use nohup because uninstaller kill the Settings Window.
    system("/usr/bin/nohup osascript '/Library/Application Support/org.pqrs/Karabiner-Elements/scripts/uninstaller.applescript' >/dev/null 2>&1 &");
  }
};
} // namespace krbn
