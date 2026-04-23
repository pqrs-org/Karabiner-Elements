import Combine
import SwiftUI

@main
struct KarabinerSettingsApp: App {
  @NSApplicationDelegateAdaptor(AppDelegate.self) var appDelegate

  init() {
    libkrbn_initialize()
    libkrbn_load_custom_environment_variables()

    //
    // Unregister old agents
    //

    libkrbn_services_bootout_old_agents()

    //
    // If Karabiner-Elements was manually terminated just before, the agents are in an unregistered state.
    // So we should enable them once before checking the status.
    //

    libkrbn_services_register_core_daemons()
    libkrbn_services_register_core_agents()

    //
    // Setup CoreServiceClient
    //

    SettingsCoreServiceClient.shared.start()
    SettingsConsoleUserServerClient.shared.start()

    //
    // Start components
    //

    KarabinerAppHelper.shared.observeVersionUpdated()
    LibKrbn.Settings.shared.watch()
    SettingsCoreServiceClient.shared.startSystemVariablesMonitoring()
    SystemPreferences.shared.start()
  }

  var body: some Scene {
    Window(
      "Karabiner-Elements Settings",
      id: "main",
      content: {
        ContentView()
      })
  }
}

@MainActor
class AppDelegate: NSObject, NSApplicationDelegate {
  private var cancellables = Set<AnyCancellable>()
  private var userAttentionRequestIdentifier: Int?

  public func applicationWillFinishLaunching(_: Notification) {
    NSAppleEventManager.shared().setEventHandler(
      self,
      andSelector: #selector(handleGetURLEvent(_:withReplyEvent:)),
      forEventClass: AEEventClass(kInternetEventClass),
      andEventID: AEEventID(kAEGetURL))

    ContentViewStates.shared.$currentAlert
      .sink { [weak self] currentAlert in
        self?.updateUserAttentionRequest(currentAlert: currentAlert)
      }
      .store(in: &cancellables)
  }

  public func applicationWillTerminate(_: Notification) {
    if let identifier = userAttentionRequestIdentifier {
      NSApp.cancelUserAttentionRequest(identifier)
      userAttentionRequestIdentifier = nil
    }

    libkrbn_terminate()
  }

  public func applicationShouldTerminateAfterLastWindowClosed(_: NSApplication) -> Bool {
    return true
  }

  private func updateUserAttentionRequest(currentAlert: SettingsWindowGuidanceAlert) {
    if currentAlert == .none {
      if let identifier = userAttentionRequestIdentifier {
        NSApp.cancelUserAttentionRequest(identifier)
        userAttentionRequestIdentifier = nil
      }
    } else if userAttentionRequestIdentifier == nil {
      userAttentionRequestIdentifier = NSApp.requestUserAttention(.criticalRequest)
    }
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
