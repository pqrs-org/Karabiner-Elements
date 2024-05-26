import AppKit
import SwiftUI
import os

@NSApplicationMain
public class AppDelegate: NSObject, NSApplicationDelegate {
  private var window: NSWindow?
  private var updaterMode = false

  override public init() {
    super.init()
    libkrbn_initialize()
  }

  public func applicationWillFinishLaunching(_: Notification) {
    NSAppleEventManager.shared().setEventHandler(
      self,
      andSelector: #selector(handleGetURLEvent(_:withReplyEvent:)),
      forEventClass: AEEventClass(kInternetEventClass),
      andEventID: AEEventID(kAEGetURL))
  }

  public func applicationDidFinishLaunching(_: Notification) {
    NSApplication.shared.disableRelaunchOnLogin()

    ProcessInfo.processInfo.enableSuddenTermination()

    KarabinerAppHelper.shared.observeVersionUpdated()
    KarabinerAppHelper.shared.observeConsoleUserServerIsDisabledNotification()

    NotificationCenter.default.addObserver(
      forName: Updater.didFindValidUpdate,
      object: nil,
      queue: .main
    ) { [weak self] _ in
      guard let self = self else { return }

      self.window?.makeKeyAndOrderFront(self)
      NSApp.activate(ignoringOtherApps: true)
    }

    NotificationCenter.default.addObserver(
      forName: Updater.didFinishUpdateCycleFor,
      object: nil,
      queue: .main
    ) { [weak self] _ in
      guard let self = self else { return }

      if self.updaterMode {
        NSApplication.shared.terminate(nil)
      }
    }

    //
    // Start components
    //

    _ = Doctor.shared

    LibKrbn.ConnectedDevices.shared.watch()
    LibKrbn.GrabberClient.shared.start()
    LibKrbn.Settings.shared.watch()
    StateJsonMonitor.shared.start()
    SystemPreferences.shared.start()

    //
    // Run updater or open settings.
    //

    if CommandLine.arguments.count > 1 {
      let command = CommandLine.arguments[1]
      switch command {
      case "checkForUpdatesInBackground":
        #if USE_SPARKLE
          if !libkrbn_lock_single_application_with_user_pid_file(
            "check_for_updates_in_background.pid")
          {
            print("Exit since another process is running.")
            NSApplication.shared.terminate(self)
          }

          updaterMode = true
          Updater.shared.checkForUpdatesInBackground()
          return
        #else
          NSApplication.shared.terminate(self)
        #endif
      default:
        break
      }
    }

    var psn = ProcessSerialNumber(highLongOfPSN: 0, lowLongOfPSN: UInt32(kCurrentProcess))
    TransformProcessType(
      &psn, ProcessApplicationTransformState(kProcessTransformToForegroundApplication))

    window = NSWindow(
      contentRect: .zero,
      styleMask: [
        .titled,
        .closable,
        .miniaturizable,
        .resizable,
        .fullSizeContentView,
      ],
      backing: .buffered,
      defer: false
    )
    window!.title = "Karabiner-Elements Settings"
    window!.contentView = NSHostingView(rootView: ContentView())
    window!.center()
    window!.makeKeyAndOrderFront(self)

    NSApp.activate(ignoringOtherApps: true)

    //
    // Register services
    //

    libkrbn_services_bootout_old_agents()
    libkrbn_services_register_core_daemons()
    libkrbn_services_register_core_agents()
  }

  public func applicationWillTerminate(_: Notification) {
    libkrbn_terminate()
  }

  public func applicationShouldTerminateAfterLastWindowClosed(_: NSApplication) -> Bool {
    if Updater.shared.sessionInProgress {
      return false
    }
    return true
  }

  @objc func handleGetURLEvent(
    _ event: NSAppleEventDescriptor,
    withReplyEvent _: NSAppleEventDescriptor
  ) {
    // - url == "karabiner://karabiner/assets/complex_modifications/import?url=xxx"
    guard let url = event.paramDescriptor(forKeyword: AEKeyword(keyDirectObject))?.stringValue
    else { return }

    Task { @MainActor in
      if let window = self.window {
        KarabinerAppHelper.shared.endAllAttachedSheets(window)
      }

      let urlComponents = URLComponents(string: url)

      if urlComponents?.path == "/assets/complex_modifications/import" {
        if let queryItems = urlComponents?.queryItems {
          for pair in queryItems where pair.name == "url" {
            ComplexModificationsFileImport.shared.fetchJson(URL(string: pair.value!)!)

            ContentViewStates.shared.navigationSelection = .complexModifications
            ContentViewStates.shared.complexModificationsViewSheetView = .fileImport
            ContentViewStates.shared.complexModificationsViewSheetPresented = true
            return
          }
        }
      }

      if let window = self.window {
        let alert = NSAlert()
        alert.messageText = "Error"
        alert.informativeText = "Unknown URL"
        alert.addButton(withTitle: "OK")

        alert.beginSheetModal(for: window) { _ in
        }
      }
    }
  }
}
