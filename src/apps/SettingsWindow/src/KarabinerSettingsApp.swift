import SwiftUI

@main
struct KarabinerSettingsApp: App {
  @NSApplicationDelegateAdaptor(AppDelegate.self) var appDelegate

  init() {
    libkrbn_initialize()

    //
    // Unregister old agents
    //

    libkrbn_services_bootout_old_agents()

    //
    // Start components
    //

    KarabinerAppHelper.shared.observeVersionUpdated()
    Doctor.shared.start()
    LibKrbn.ConnectedDevices.shared.watch()
    LibKrbn.GrabberClient.shared.start("")
    LibKrbn.Settings.shared.watch()
    ServicesMonitor.shared.start()
    SettingsChecker.shared.start()
    StateJsonMonitor.shared.start()
    SystemPreferences.shared.start()
  }

  var body: some Scene {
    WindowGroup(
      "Karabiner-Elements Settings",
      content: {
        ContentView()
      })
  }
}

class AppDelegate: NSObject, NSApplicationDelegate {
  public func applicationWillFinishLaunching(_: Notification) {
    NSAppleEventManager.shared().setEventHandler(
      self,
      andSelector: #selector(handleGetURLEvent(_:withReplyEvent:)),
      forEventClass: AEEventClass(kInternetEventClass),
      andEventID: AEEventID(kAEGetURL))
  }

  public func applicationWillTerminate(_: Notification) {
    libkrbn_terminate()
  }

  public func applicationShouldTerminateAfterLastWindowClosed(_: NSApplication) -> Bool {
    return true
  }

  @objc func handleGetURLEvent(
    _ event: NSAppleEventDescriptor,
    withReplyEvent _: NSAppleEventDescriptor
  ) {
    // If the application is already running when the handler is called, there's no issue.
    // However, if the application is launched via an AppleEvent, the window may not display correctly.
    // To avoid this problem, the handler will explicitly reopen the application to ensure that the window is shown properly.
    Task { @MainActor in
      NSWorkspace.shared.openApplication(
        at: Bundle.main.bundleURL,
        configuration: NSWorkspace.OpenConfiguration())
    }

    // - url == "karabiner://karabiner/assets/complex_modifications/import?url=xxx"
    guard let url = event.paramDescriptor(forKeyword: AEKeyword(keyDirectObject))?.stringValue
    else { return }

    let urlComponents = URLComponents(string: url)
    if urlComponents?.path == "/assets/complex_modifications/import" {
      if let queryItems = urlComponents?.queryItems {
        for pair in queryItems where pair.name == "url" {
          Task { @MainActor in
            ComplexModificationsFileImport.shared.fetchJson(URL(string: pair.value!)!)

            ContentViewStates.shared.navigationSelection = .complexModifications
            ContentViewStates.shared.complexModificationsViewSheetView = .fileImport
            ContentViewStates.shared.complexModificationsViewSheetPresented = true
          }
          return
        }
      }
    }
  }
}
