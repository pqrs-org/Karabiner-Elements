import AppKit

@NSApplicationMain
public class AppDelegate: NSObject, NSApplicationDelegate {
  public func applicationDidFinishLaunching(_: Notification) {
    ProcessInfo.processInfo.enableSuddenTermination()

    libkrbn_initialize()

    MenuController.shared.setup()
  }

  public func applicationWillTerminate(_: Notification) {
    libkrbn_terminate()
  }
}
