import AppKit

@NSApplicationMain
public class AppDelegate: NSObject, NSApplicationDelegate {
  public func applicationDidFinishLaunching(_: Notification) {
    ProcessInfo.processInfo.enableSuddenTermination()

    libkrbn_initialize()

    NotificationWindowManager.shared.start()
  }

  public func applicationWillTerminate(_: Notification) {
    libkrbn_terminate()
  }
}
