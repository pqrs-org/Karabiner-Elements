import AsyncAlgorithms
import Foundation

private func settingsWindowAlertReceivedCallback(_ jsonString: UnsafePointer<CChar>) {
  let data = Data(String(cString: jsonString).utf8)

  if let state = try? JSONDecoder().decode(SettingsWindowAlertState.self, from: data) {
    Task { @MainActor in
      ContentViewStates.shared.updateAlertState(state)
    }
  }
}

private func consoleUserServerClientStatusChangedCallback() {
  Task { @MainActor in
    SettingsConsoleUserServerClient.shared.updateConsoleUserServerClientState()
  }

  libkrbn_console_user_server_client_async_get_settings_window_alert()
}

@MainActor
final class SettingsConsoleUserServerClient {
  static let shared = SettingsConsoleUserServerClient()

  private let continuousClock: ContinuousClock
  private let currentAlertTimer: AsyncTimerSequence<ContinuousClock>
  private var currentAlertTimerTask: Task<Void, Never>?
  private var consoleUserServerClientDisconnectedAt: ContinuousClock.Instant?

  init() {
    continuousClock = ContinuousClock()
    currentAlertTimer = AsyncTimerSequence(
      interval: .seconds(1),
      clock: continuousClock
    )
  }

  public func start() {
    libkrbn_enable_console_user_server_client(geteuid(), "settings_cus_clnt")

    libkrbn_register_console_user_server_client_status_changed_callback(
      consoleUserServerClientStatusChangedCallback)
    libkrbn_register_console_user_server_client_settings_window_alert_received_callback(
      settingsWindowAlertReceivedCallback)

    libkrbn_console_user_server_client_async_start()

    currentAlertTimerTask = Task { @MainActor in
      updateConsoleUserServerClientState()
      libkrbn_console_user_server_client_async_get_settings_window_alert()

      for await _ in currentAlertTimer {
        updateConsoleUserServerClientState()
        libkrbn_console_user_server_client_async_get_settings_window_alert()
      }
    }
  }

  func updateConsoleUserServerClientState() {
    let connected =
      libkrbn_console_user_server_client_get_status()
      == libkrbn_console_user_server_client_status_connected

    if connected {
      consoleUserServerClientDisconnectedAt = nil
    } else {
      if consoleUserServerClientDisconnectedAt == nil {
        consoleUserServerClientDisconnectedAt = continuousClock.now
      }
    }

    let consoleUserServerClientWaitingSeconds =
      consoleUserServerClientDisconnectedAt.map {
        max(0, Int($0.duration(to: continuousClock.now).components.seconds))
      } ?? 0

    ContentViewStates.shared.updateConsoleUserServerClientConnected(connected)
    ContentViewStates.shared.updateConsoleUserServerClientWaitingSeconds(
      consoleUserServerClientWaitingSeconds)
  }
}
