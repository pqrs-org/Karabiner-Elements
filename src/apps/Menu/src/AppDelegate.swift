import AppKit

@NSApplicationMain
public class AppDelegate: NSObject, NSApplicationDelegate {
    @IBOutlet var menuController: MenuController!

    public func applicationDidFinishLaunching(_: Notification) {
        ProcessInfo.processInfo.enableSuddenTermination()

        libkrbn_initialize()

        KarabinerKit.setup()
        KarabinerKit.exitIfAnotherProcessIsRunning("menu.pid")
        KarabinerKit.observeConsoleUserServerIsDisabledNotification()

        menuController.setup()
    }

    public func applicationWillTerminate(_: Notification) {
        libkrbn_terminate()
    }
}
