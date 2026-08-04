import AsyncAlgorithms
import Foundation

private func settingsWindowGuidanceReceivedCallback(_ jsonString: UnsafePointer<CChar>) {
  let data = Data(String(cString: jsonString).utf8)

  if let state = try? JSONDecoder().decode(SettingsWindowGuidanceState.self, from: data) {
    Task { @MainActor in
      SettingsConsoleUserServerClient.shared.settingsWindowGuidanceReceived(state)
    }
  }
}

private func consoleUserServerClientStatusChangedCallback() {
  Task { @MainActor in
    SettingsConsoleUserServerClient.shared.updateConsoleUserServerClientState()
    SettingsConsoleUserServerClient.shared.updateLocalServicesGuidanceContext()
  }

  krbn_console_user_server_client_async_get_settings_window_guidance()
}

@MainActor
final class SettingsConsoleUserServerClient {
  static let shared = SettingsConsoleUserServerClient()

  private let continuousClock: ContinuousClock
  private let currentAlertTimer: AsyncTimerSequence<ContinuousClock>
  private var currentAlertTimerTask: Task<Void, Never>?
  private var consoleUserServerClientReady = false
  private var consoleUserServerClientDisconnectedAt: ContinuousClock.Instant?

  init() {
    continuousClock = ContinuousClock()
    currentAlertTimer = AsyncTimerSequence(
      interval: .seconds(1),
      clock: continuousClock
    )
  }

  public func start() {
    krbn_enable_console_user_server_client(geteuid())

    krbn_register_console_user_server_client_status_changed_callback(
      consoleUserServerClientStatusChangedCallback)
    krbn_register_console_user_server_client_settings_window_guidance_received_callback(
      settingsWindowGuidanceReceivedCallback)

    krbn_console_user_server_client_async_start()

    currentAlertTimerTask = Task { @MainActor in
      updateConsoleUserServerClientState()
      updateLocalServicesGuidanceContext()
      krbn_console_user_server_client_async_get_settings_window_guidance()

      for await _ in currentAlertTimer {
        updateConsoleUserServerClientState()
        updateLocalServicesGuidanceContext()
        krbn_console_user_server_client_async_get_settings_window_guidance()
      }
    }
  }

  func updateConsoleUserServerClientState() {
    if krbn_console_user_server_client_get_status()
      != krbn_console_user_server_client_status_connected
    {
      consoleUserServerClientReady = false
    }

    if !consoleUserServerClientReady && consoleUserServerClientDisconnectedAt == nil {
      consoleUserServerClientDisconnectedAt = continuousClock.now
    }

    let disconnectedForAWhile =
      consoleUserServerClientDisconnectedAt.map {
        $0.duration(to: continuousClock.now) >= .seconds(5)
      } ?? false

    ContentViewStates.shared.updateConsoleUserServerClientReady(consoleUserServerClientReady)
    ContentViewStates.shared.updateConsoleUserServerClientDisconnectedForAWhile(
      disconnectedForAWhile)
  }

  func settingsWindowGuidanceReceived(_ state: SettingsWindowGuidanceState) {
    // The transport's connected status does not guarantee that peer verification has completed.
    // If the code signatures do not match, the transport repeatedly connects and disconnects
    // as peer verification fails. Receiving a guidance response proves that the connection has
    // passed verification and is ready for communication.
    consoleUserServerClientReady = true
    consoleUserServerClientDisconnectedAt = nil

    ContentViewStates.shared.updateConsoleUserServerClientReady(true)
    ContentViewStates.shared.updateConsoleUserServerClientDisconnectedForAWhile(false)
    ContentViewStates.shared.updateGuidanceState(state)
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
