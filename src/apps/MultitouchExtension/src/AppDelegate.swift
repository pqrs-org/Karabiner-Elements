import AppKit

//
// AppDelegate
//

@NSApplicationMain
public class AppDelegate: NSObject, NSApplicationDelegate {
  private var activity: NSObjectProtocol?

  public func applicationDidFinishLaunching(_: Notification) {
    ProcessInfo.processInfo.enableSuddenTermination()

    libkrbn_initialize()

    KarabinerKit.setup()
    KarabinerKit.observeConsoleUserServerIsDisabledNotification()

    NSApplication.shared.disableRelaunchOnLogin()

    //
    // Handle kHideIconInDock
    //

    if !UserSettings.shared.hideIconInDock {
      var psn = ProcessSerialNumber(highLongOfPSN: 0, lowLongOfPSN: UInt32(kCurrentProcess))
      TransformProcessType(
        &psn, ProcessApplicationTransformState(kProcessTransformToForegroundApplication))
    }

    //
    // Handle --start-at-login
    //

    if CommandLine.arguments.contains("--start-at-login") {
      if !UserSettings.shared.openAtLogin {
        NSApplication.shared.terminate(self)
      }
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

    MEGrabberClient.shared.setGrabberVariable(FingerCount(), true)
    MultitouchDeviceManager.shared.observeIONotification()

    //
    // Register wake up handler
    //

    MultitouchDeviceManager.shared.observeWakeNotification()

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

    MEGrabberClient.shared.setGrabberVariable(FingerCount(), true)

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
