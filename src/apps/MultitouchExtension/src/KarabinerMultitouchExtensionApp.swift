import SettingsAccess
import SwiftUI

@main
struct KarabinerMultitouchExtensionApp: App {
  @NSApplicationDelegateAdaptor(AppDelegate.self) var appDelegate

  private let version =
    Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as? String ?? ""

  init() {
    libkrbn_initialize()
    libkrbn_load_custom_environment_variables()
  }

  var body: some Scene {
    MenuBarExtra(
      "Karabiner-MultitouchExtension", systemImage: "rectangle.and.hand.point.up.left.filled",
    ) {
      Text("Karabiner-MultitouchExtension \(version)")

      Divider()

      SettingsLink {
        Label("Settings...", systemImage: "gear")
          .labelStyle(.titleAndIcon)
      } preAction: {
        NSApp.activate(ignoringOtherApps: true)
      } postAction: {
      }
    }

    Settings {
      SettingsView()
    }
  }
}

@MainActor
class AppDelegate: NSObject, NSApplicationDelegate {
  private var activity: NSObjectProtocol?
  private var sleepTask: Task<Void, Never>?
  private var wakeTask: Task<Void, Never>?

  public func applicationDidFinishLaunching(_: Notification) {
    //
    // Enable core_service_client
    //

    MECoreServiceClient.shared.start()
    MultitouchDeviceManager.shared.observeIONotification()

    startActivity()

    observeSystemSleep()
  }

  public func applicationWillTerminate(_: Notification) {
    cancelSystemSleepObservers()

    stopActivity()

    MultitouchDeviceManager.shared.setCallback(false)

    libkrbn_terminate()
  }

  private func startActivity() {
    print("startActivity")

    //
    // Disable App Nap
    //

    activity = ProcessInfo.processInfo.beginActivity(
      options: .userInteractive,
      reason: "Disable App Nap in order to receive multitouch events even if this app is background"
    )
  }

  private func stopActivity() {
    print("stopActivity")

    if let a = activity {
      ProcessInfo.processInfo.endActivity(a)
      activity = nil
    }
  }

  private func observeSystemSleep() {
    sleepTask = Task { @MainActor in
      let notifications = NSWorkspace.shared.notificationCenter.notifications(
        named: NSWorkspace.willSleepNotification,
        object: nil
      )

      for await _ in notifications {
        self.stopActivity()
      }
    }

    wakeTask = Task { @MainActor in
      let notifications = NSWorkspace.shared.notificationCenter.notifications(
        named: NSWorkspace.didWakeNotification,
        object: nil
      )

      for await _ in notifications {
        self.startActivity()
      }
    }
  }

  private func cancelSystemSleepObservers() {
    sleepTask?.cancel()
    sleepTask = nil

    wakeTask?.cancel()
    wakeTask = nil
  }
}
