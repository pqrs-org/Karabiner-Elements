import AppKit
import SwiftUI
import os

@main
struct KarabinerUpdaterApp: App {
  @NSApplicationDelegateAdaptor(AppDelegate.self) var appDelegate

  var body: some Scene {
    // Provide an empty Settings to prevent build errors.
    Settings {}
  }
}

class AppDelegate: NSObject, NSApplicationDelegate {
  private enum Command: String {
    case checkForUpdatesInBackground
    case checkForUpdatesStableOnly
    case checkForUpdatesWithBetaVersion
  }

  private static let command = Command(
    rawValue: CommandLine.arguments.dropFirst().first ?? ""
  )

  public func applicationWillFinishLaunching(_: Notification) {
    switch Self.command {
    case .checkForUpdatesStableOnly, .checkForUpdatesWithBetaVersion:
      NSApp.setActivationPolicy(.regular)

    case .checkForUpdatesInBackground, nil:
      break
    }
  }

  public func applicationDidFinishLaunching(_: Notification) {
    Task { @MainActor in
      switch Self.command {
      case .checkForUpdatesInBackground:
        Updater.shared.checkForUpdatesInBackground()

      case .checkForUpdatesStableOnly:
        Updater.shared.checkForUpdatesStableOnly()

      case .checkForUpdatesWithBetaVersion:
        Updater.shared.checkForUpdatesWithBetaVersion()

      case nil:
        NSApplication.shared.terminate(nil)
      }
    }
  }
}
