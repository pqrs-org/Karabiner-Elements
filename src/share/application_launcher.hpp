#pragma once

#include <cstdlib>
#include <string>

namespace krbn {
class application_launcher final {
public:
  static void launch_event_viewer(void) {
    system("open '/Applications/Karabiner-EventViewer.app'");
  }

  static void launch_menu(void) {
    system("open -g '/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-Menu.app'");
  }

  static void kill_menu(void) {
    system("killall Karabiner-Menu");
  }

  static void launch_notification_window(void) {
    system("open -g '/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-NotificationWindow.app'");
  }

  static void launch_preferences(void) {
    system("open '/Applications/Karabiner-Elements.app'");
  }

  static void launch_multitouch_extension(bool as_start_at_login) {
    std::string command("open '/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-MultitouchExtension.app'");

    if (as_start_at_login) {
      command += " --args --start-at-login";
    }

    system(command.c_str());
  }

  static void kill_multitouch_extension(void) {
    system("killall Karabiner-MultitouchExtension");
  }
};
} // namespace krbn
