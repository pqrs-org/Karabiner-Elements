import Cocoa
import SwiftUI

@main
struct KarabinerEventViewerApp: App {
  @AppStorage("uiLanguage") private var language = "auto"
  @NSApplicationDelegateAdaptor(AppDelegate.self) var appDelegate

  @StateObject private var captureSettings: CaptureSettings

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
      secureEventInputEnabledChanged: secureEventInputEnabledChangedCallback,
      terminationCompleted: completePendingApplicationTermination)

    let captureSettings = CaptureSettings.shared
    _captureSettings = StateObject(wrappedValue: captureSettings)

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
          .environmentObject(captureSettings)
          .modifier(StoredAppLanguage(pickerPlacement: .none))
      }
    )
    .commands {
      FindCommands(locale: AppLanguage.locale(for: language))
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
