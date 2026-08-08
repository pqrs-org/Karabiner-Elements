import Foundation

func settingsWindowGuidanceReceivedCallback(_ jsonString: UnsafePointer<CChar>) {
  let data = Data(String(cString: jsonString).utf8)

  if let state = try? JSONDecoder().decode(SettingsWindowGuidanceState.self, from: data) {
    Task { @MainActor in
      SettingsConsoleUserServerClient.shared.settingsWindowGuidanceReceived(state)
    }
  }
}

func consoleUserServerClientStatusChangedCallback() {
  Task { @MainActor in
    SettingsConsoleUserServerClient.shared.updateConsoleUserServerClientState()
    SettingsConsoleUserServerClient.shared.updateLocalServicesGuidanceContext()
  }
}

@MainActor
final class SettingsConsoleUserServerClient {
  static let shared = SettingsConsoleUserServerClient()

  private var disconnectedForAWhileTask: Task<Void, Never>?
  private var consoleUserServerClientReady = false

  public func start() {
    updateConsoleUserServerClientState()
    updateLocalServicesGuidanceContext()
  }

  func componentsManagerStopped() {
    consoleUserServerClientReady = false
    disconnectedForAWhileTask?.cancel()
    disconnectedForAWhileTask = nil

    ContentViewStates.shared.updateConsoleUserServerClientReady(false)
    ContentViewStates.shared.updateConsoleUserServerClientDisconnectedForAWhile(false)

    updateConsoleUserServerClientState()
    updateLocalServicesGuidanceContext()
  }

  func updateConsoleUserServerClientState() {
    if !krbn_console_user_server_client_connected() {
      consoleUserServerClientReady = false
    }

    ContentViewStates.shared.updateConsoleUserServerClientReady(consoleUserServerClientReady)

    if !consoleUserServerClientReady && disconnectedForAWhileTask == nil {
      disconnectedForAWhileTask = Task { @MainActor [weak self] in
        try? await Task.sleep(for: .seconds(5))

        guard !Task.isCancelled,
          let self,
          !self.consoleUserServerClientReady
        else { return }

        ContentViewStates.shared.updateConsoleUserServerClientDisconnectedForAWhile(true)
        self.updateLocalServicesGuidanceContext()
      }
    }
  }

  func settingsWindowGuidanceReceived(_ state: SettingsWindowGuidanceState) {
    // The transport's connected status does not guarantee that peer verification has completed.
    // If the code signatures do not match, the transport repeatedly connects and disconnects
    // as peer verification fails. Receiving a guidance response proves that the connection has
    // passed verification and is ready for communication.
    consoleUserServerClientReady = true
    disconnectedForAWhileTask?.cancel()
    disconnectedForAWhileTask = nil

    ContentViewStates.shared.updateConsoleUserServerClientReady(true)
    ContentViewStates.shared.updateConsoleUserServerClientDisconnectedForAWhile(false)
    ContentViewStates.shared.updateGuidanceState(state)

    // The C++ client requests settings_window_guidance once per second while connected,
    // so this also keeps the locally obtained services state up to date without a Swift timer.
    updateLocalServicesGuidanceContext()
  }

  func updateLocalServicesGuidanceContext() {
    // settings_window_guidance is provided by console_user_server. If Settings cannot connect to
    // console_user_server, it cannot fetch that guidance. Keep the service-enabled states updated
    // locally so ContentViewStates can open SetupServicesView as a fallback.
    ContentViewStates.shared.updateLocalServicesGuidanceContext(
      coreDaemonsEnabled: krbn_services_daemons_enabled(),
      coreAgentsEnabled: krbn_services_agents_enabled())
  }
}
