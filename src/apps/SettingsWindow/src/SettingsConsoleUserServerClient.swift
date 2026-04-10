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
  let connected =
    libkrbn_console_user_server_client_get_status()
    == libkrbn_console_user_server_client_status_connected

  Task { @MainActor in
    ContentViewStates.shared.updateConsoleUserServerClientConnected(connected)
  }

  libkrbn_console_user_server_client_async_get_settings_window_alert()
}

@MainActor
final class SettingsConsoleUserServerClient {
  static let shared = SettingsConsoleUserServerClient()

  private let currentAlertTimer: AsyncTimerSequence<ContinuousClock>
  private var currentAlertTimerTask: Task<Void, Never>?

  init() {
    currentAlertTimer = AsyncTimerSequence(
      interval: .seconds(1),
      clock: .continuous
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
      libkrbn_console_user_server_client_async_get_settings_window_alert()

      for await _ in currentAlertTimer {
        libkrbn_console_user_server_client_async_get_settings_window_alert()
      }
    }
  }
}
