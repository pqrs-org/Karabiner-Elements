import SwiftUI

@main
struct KarabinerMultitouchExtensionApp: App {
  @NSApplicationDelegateAdaptor(AppDelegate.self) var appDelegate

  init() {
    libkrbn_initialize()
  }

  var body: some Scene {
    // Provide an empty Settings to prevent build errors.
    Settings {}
  }
}

class AppDelegate: NSObject, NSApplicationDelegate {
  private var activity: NSObjectProtocol?

  public func applicationDidFinishLaunching(_: Notification) {
    //
    // Handle kHideIconInDock
    //

    if !UserSettings.shared.hideIconInDock {
      var psn = ProcessSerialNumber(highLongOfPSN: 0, lowLongOfPSN: UInt32(kCurrentProcess))
      TransformProcessType(
        &psn, ProcessApplicationTransformState(kProcessTransformToForegroundApplication))
    }

    //
    // Handle --show-ui
    //

    if CommandLine.arguments.contains("--show-ui") {
      SettingsWindowManager.shared.show()
    }

    //
    // Enable grabber_client
    //

    MEGrabberClient.shared.start()
    MultitouchDeviceManager.shared.observeIONotification()

    //
    // Disable App Nap
    //

    activity = ProcessInfo.processInfo.beginActivity(
      options: .userInitiated,
      reason: "Disable App Nap in order to receive multitouch events even if this app is background"
    )
  }

  public func applicationWillTerminate(_: Notification) {
    if let a = activity {
      ProcessInfo.processInfo.endActivity(a)
      activity = nil
    }

    MultitouchDeviceManager.shared.setCallback(false)

    libkrbn_terminate()
  }

  public func applicationShouldHandleReopen(
    _: NSApplication,
    hasVisibleWindows _: Bool
  ) -> Bool {
    SettingsWindowManager.shared.show()
    return true
  }
}
