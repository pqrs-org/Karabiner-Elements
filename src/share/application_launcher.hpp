#pragma once

#include <cstdlib>
#include <sstream>

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

  static void launch_settings(void) {
    system("open '/Applications/Karabiner-Elements.app'");
  }

  static void launch_multitouch_extension(bool as_start_at_login) {
    //
    // Kill an existing process
    //

    if (as_start_at_login) {
      system("killall Karabiner-MultitouchExtension");
    }

    //
    // Launch process
    //

    std::stringstream command;
    command << "open ";

    if (as_start_at_login) {
      command << " -n ";
    }

    command << "'/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-MultitouchExtension.app'";

    if (as_start_at_login) {
      command << " --args --start-at-login";
    } else {
      command << " --args --show-ui";
    }

    system(command.str().c_str());
  }

  static void launch_uninstaller(void) {
    // Use nohup because uninstaller kill the Settings Window.
    system("/usr/bin/nohup osascript '/Library/Application Support/org.pqrs/Karabiner-Elements/scripts/uninstaller.applescript' >/dev/null 2>&1 &");
  }
};
} // namespace krbn
