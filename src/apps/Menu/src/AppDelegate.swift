import AppKit

@NSApplicationMain
public class AppDelegate: NSObject, NSApplicationDelegate {
  @IBOutlet var menuController: MenuController!

  public func applicationDidFinishLaunching(_: Notification) {
    ProcessInfo.processInfo.enableSuddenTermination()

    libkrbn_initialize()

    KarabinerAppHelper.observeVersionChange()
    KarabinerKit.observeConsoleUserServerIsDisabledNotification()

    MenuController.shared.setup()
  }

  public func applicationWillTerminate(_: Notification) {
    libkrbn_terminate()
  }
}
