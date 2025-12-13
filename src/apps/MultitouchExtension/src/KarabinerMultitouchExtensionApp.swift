import Combine
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
  private var userSettingsCancellable: AnyCancellable?

  public func applicationDidFinishLaunching(_: Notification) {
    //
    // Enable core_service_client
    //

    MECoreServiceClient.shared.start()
    MultitouchDeviceManager.shared.observeIONotification()

    observeUserInteractiveActivitySettings()

    observeSystemSleep()
  }

  public func applicationWillTerminate(_: Notification) {
    cancelSystemSleepObservers()

    stopActivity()
    userSettingsCancellable = nil

    MultitouchDeviceManager.shared.setCallback(false)

    libkrbn_terminate()
  }

  private func startActivity() {
    //
    // Disable App Nap
    //

    if UserSettings.shared.allowUserInteractiveActivity {
      print("beginActivity")

      activity = ProcessInfo.processInfo.beginActivity(
        options: .userInteractive,
        reason:
          "Disable App Nap in order to receive multitouch events even if this app is background"
      )
    }
  }

  private func stopActivity() {
    if let a = activity {
      print("endActivity")

      ProcessInfo.processInfo.endActivity(a)
      activity = nil
    }
  }

  private func observeUserInteractiveActivitySettings() {
    userSettingsCancellable = UserSettings.shared.$allowUserInteractiveActivity
      .sink { [weak self] isAllowed in
        guard let self else { return }

        Task { @MainActor in
          if isAllowed {
            self.startActivity()
          } else {
            self.stopActivity()
          }
        }
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
