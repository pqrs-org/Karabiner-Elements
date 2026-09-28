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
  private let servicesStatusQueue = DispatchQueue(
    label: "org.pqrs.Karabiner-Elements.Settings.services-status", qos: .utility)
  private var servicesQueryInFlight = false
  private var servicesQueryPending = false
  private var servicesQueryGeneration = UUID()
  private var coreDaemonsOnceEnabled = false
  private var coreAgentsOnceEnabled = false

  public func start() {
    updateConsoleUserServerClientState()
    updateLocalServicesGuidanceContext()
  }

  func componentsManagerStopped() {
    // An external command already running cannot be cancelled. Ignore its result and
    // retry unconfirmed services after it finishes. Preserve confirmed enabled states.
    servicesQueryGeneration = UUID()
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
        var elapsedSeconds = 0
        while !Task.isCancelled {
          try? await Task.sleep(for: .seconds(1))

          guard !Task.isCancelled,
            let self,
            !self.consoleUserServerClientReady
          else { return }

          elapsedSeconds += 1
          if elapsedSeconds == 5 {
            ContentViewStates.shared.updateConsoleUserServerClientDisconnectedForAWhile(true)
          }
          // Failed connection attempts do not emit another status change. Poll locally
          // until login items have been confirmed enabled while disconnected.
          self.updateLocalServicesGuidanceContext()
        }
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
    // so this retries unconfirmed services while connected.
    updateLocalServicesGuidanceContext()
  }

  func updateLocalServicesGuidanceContext() {
    // settings_window_guidance is provided by console_user_server. If Settings cannot connect to
    // console_user_server, it cannot fetch that guidance. Keep the service-enabled states updated
    // locally so ContentViewStates can open SetupServicesView as a fallback.
    // These calls launch processes and wait for them. Keep them off MainActor and
    // coalesce requests arriving during a query into one follow-up, without a queue backlog.
    // Once enabled, stop checking each service group for this Settings process's lifetime.
    guard !coreDaemonsOnceEnabled || !coreAgentsOnceEnabled else { return }
    guard !servicesQueryInFlight else {
      servicesQueryPending = true
      return
    }
    servicesQueryInFlight = true
    let generation = servicesQueryGeneration
    let daemonsOnceEnabled = coreDaemonsOnceEnabled
    let agentsOnceEnabled = coreAgentsOnceEnabled
    servicesStatusQueue.async { [weak self] in
      let daemonsEnabled =
        daemonsOnceEnabled ? true : Self.enabledValue(krbn_services_daemons_enabled())
      let agentsEnabled =
        agentsOnceEnabled ? true : Self.enabledValue(krbn_services_agents_enabled())
      Task { @MainActor [weak self] in
        guard let self else { return }
        self.servicesQueryInFlight = false
        if self.servicesQueryGeneration == generation {
          self.coreDaemonsOnceEnabled = daemonsEnabled == true
          self.coreAgentsOnceEnabled = agentsEnabled == true
          ContentViewStates.shared.updateLocalServicesGuidanceContext(
            coreDaemonsEnabled: daemonsEnabled,
            coreAgentsEnabled: agentsEnabled)
        }
        if self.servicesQueryPending {
          self.servicesQueryPending = false
          self.updateLocalServicesGuidanceContext()
        }
      }
    }
  }

  private nonisolated static func enabledValue(_ state: krbn_service_enabled_state) -> Bool? {
    switch state {
    case krbn_service_enabled_state_enabled: true
    case krbn_service_enabled_state_disabled: false
    default: nil  // Query failures must not be treated as disabled services.
    }
  }
}
