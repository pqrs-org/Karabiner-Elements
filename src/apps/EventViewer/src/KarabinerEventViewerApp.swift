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
      hidValueArrived: hidValueArrivedCallback,
      hidInputReportArrived: hidInputReportArrivedCallback,
      hidDeviceOpenStateChanged: hidDeviceOpenStateChangedCallback,
      terminationCompleted: completePendingApplicationTermination)

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
  public func applicationShouldTerminate(_: NSApplication) -> NSApplication.TerminateReply {
    // Keep AppKit running until the asynchronous C++ component cleanup finishes.
    requestApplicationTermination {
      krbn_async_request_termination()
    }
  }

  public func applicationWillTerminate(_: Notification) {
    krbn_finalize()
  }

  public func applicationShouldTerminateAfterLastWindowClosed(_: NSApplication) -> Bool {
    true
  }
}
