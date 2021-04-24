import AppKit

@NSApplicationMain
public class AppDelegate: NSObject, NSApplicationDelegate {
    var notificationWindowManager: NotificationWindowManager?

    public func applicationDidFinishLaunching(_: Notification) {
        ProcessInfo.processInfo.enableSuddenTermination()

        libkrbn_initialize()

        KarabinerKit.setup()
        KarabinerKit.exitIfAnotherProcessIsRunning("notification_window.pid")
        KarabinerKit.observeConsoleUserServerIsDisabledNotification()

        notificationWindowManager = NotificationWindowManager()
    }

    public func applicationWillTerminate(_: Notification) {
        notificationWindowManager = nil

        libkrbn_terminate()
    }
}
