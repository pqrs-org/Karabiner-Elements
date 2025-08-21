import SwiftUI

@main
struct KarabinerNotificationWindowApp: App {
  @NSApplicationDelegateAdaptor(AppDelegate.self) var appDelegate

  init() {
    libkrbn_initialize()
    libkrbn_load_custom_environment_variables()
  }

  var body: some Scene {
    // The main window is manually managed by NotificationWindowManager.

    // Provide an empty Settings to prevent build errors.
    Settings {}
  }
}

class AppDelegate: NSObject, NSApplicationDelegate {
  public func applicationDidFinishLaunching(_: Notification) {
    NotificationWindowManager.shared.start()
  }

  public func applicationWillTerminate(_: Notification) {
    libkrbn_terminate()
  }
}
