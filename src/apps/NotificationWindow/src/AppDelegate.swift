import AppKit

@NSApplicationMain
public class AppDelegate: NSObject, NSApplicationDelegate {
  private var notificationWindowManager: NotificationWindowManager?

  public func applicationDidFinishLaunching(_: Notification) {
    ProcessInfo.processInfo.enableSuddenTermination()

    libkrbn_initialize()

    KarabinerKit.setup()

    notificationWindowManager = NotificationWindowManager()
  }

  public func applicationWillTerminate(_: Notification) {
    notificationWindowManager = nil

    libkrbn_terminate()
  }
}
