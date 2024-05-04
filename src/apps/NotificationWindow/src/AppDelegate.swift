import AppKit

@NSApplicationMain
public class AppDelegate: NSObject, NSApplicationDelegate {
  public func applicationDidFinishLaunching(_: Notification) {
    ProcessInfo.processInfo.enableSuddenTermination()

    libkrbn_initialize()

    // There is no need for Karabiner-NotificationWindow to relaunch itself, as it is restarted by launchd.
    KarabinerAppHelper.shared.observeVersionUpdated(relaunch: false)

    NotificationWindowManager.shared.start()
  }

  public func applicationWillTerminate(_: Notification) {
    libkrbn_terminate()
  }
}
