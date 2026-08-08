import Cocoa
import SwiftUI

@main
struct KarabinerEventViewerApp: App {
  @NSApplicationDelegateAdaptor(AppDelegate.self) var appDelegate

  @StateObject private var userSettings: UserSettings

  init() {
    _ = EVCoreServiceDaemonClient.shared
    _ = FrontmostApplicationHistory.shared
    _ = EventHistory.shared

    krbn_initialize(
      coreServiceConnectionChanged: coreServiceConnectionChangedCallback,
      manipulatorEnvironmentReceived: manipulatorEnvironmentReceivedCallback,
      connectedDevicesReceived: connectedDevicesReceivedCallback,
      frontmostApplicationHistoryReceived: frontmostApplicationHistoryReceivedCallback,
      hidValueMonitorStopped: hidValueMonitorStoppedCallback,
      hidValueArrived: hidValueArrivedCallback)

    let userSettings = UserSettings()
    _userSettings = StateObject(wrappedValue: userSettings)

    if !IOHIDRequestAccess(kIOHIDRequestTypeListenEvent) {
      InputMonitoringAlertData.shared.showing = true
    }

    FrontmostApplicationHistory.shared.watch()
  }

  var body: some Scene {
    Window(
      "Karabiner-EventViewer",
      id: "main",
      content: {
        ContentView()
          .environmentObject(userSettings)
      }
    )
    .commands {
      FindCommands()
    }
  }
}

class AppDelegate: NSObject, NSApplicationDelegate {
  public func applicationWillTerminate(_: Notification) {
    krbn_terminate()
  }

  public func applicationShouldTerminateAfterLastWindowClosed(_: NSApplication) -> Bool {
    true
  }
}
