import AppKit
import SwiftUI
import os

@NSApplicationMain
public class AppDelegate: NSObject, NSApplicationDelegate {
  private var window: NSWindow?

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

    //
    // Start components
    //

    Doctor.shared.start()
    LibKrbn.ConnectedDevices.shared.watch()
    LibKrbn.GrabberClient.shared.start("")
    LibKrbn.Settings.shared.watch()
    ServicesMonitor.shared.start()
    SettingsChecker.shared.start()
    StateJsonMonitor.shared.start()
    SystemPreferences.shared.start()

    window = NSWindow(
      contentRect: NSRect(x: 0, y: 0, width: 1100, height: 680),
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
    // Unregister old agents
    //

    libkrbn_services_bootout_old_agents()
  }

  public func applicationWillTerminate(_: Notification) {
    libkrbn_terminate()
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
