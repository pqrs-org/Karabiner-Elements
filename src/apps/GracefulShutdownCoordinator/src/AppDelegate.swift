import AppKit

@main
class AppDelegate: NSObject, NSApplicationDelegate {
    public func applicationShouldTerminate(_: NSApplication) -> NSApplication.TerminateReply {
        // macOS Big Sur 11.0 - 11.2 have an issue that causes kernel panic  if virtual HID devices exist at shutdown.
        // So we have to detach these devices before shutdown.
        //
        // Shutting down console_user_server performs the device termination.

        libkrbn_launchctl_bootout_console_user_server()

        for _ in 1 ... 10 {
            // Wait until the device is detached.

            if !libkrbn_virtual_hid_keyboard_exists(),
               !libkrbn_virtual_hid_pointing_exists()
            {
                break
            }

            sleep(1)
        }

        return .terminateNow
    }
}
