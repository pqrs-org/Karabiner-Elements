import AsyncAlgorithms
import Foundation

private func settingsWindowGuidanceReceivedCallback(_ jsonString: UnsafePointer<CChar>) {
  let data = Data(String(cString: jsonString).utf8)

  if let state = try? JSONDecoder().decode(SettingsWindowGuidanceState.self, from: data) {
    Task { @MainActor in
      ContentViewStates.shared.updateGuidanceState(state)
    }
  }
}

private func consoleUserServerClientStatusChangedCallback() {
  Task { @MainActor in
    SettingsConsoleUserServerClient.shared.updateConsoleUserServerClientState()
  }

  libkrbn_console_user_server_client_async_get_settings_window_guidance()
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
    libkrbn_register_console_user_server_client_settings_window_guidance_received_callback(
      settingsWindowGuidanceReceivedCallback)

    libkrbn_console_user_server_client_async_start()

    currentAlertTimerTask = Task { @MainActor in
      updateConsoleUserServerClientState()
      libkrbn_console_user_server_client_async_get_settings_window_guidance()

      for await _ in currentAlertTimer {
        updateConsoleUserServerClientState()
        libkrbn_console_user_server_client_async_get_settings_window_guidance()
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
