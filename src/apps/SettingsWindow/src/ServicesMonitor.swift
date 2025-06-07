import AsyncAlgorithms
import SwiftUI

@MainActor
public class ServicesMonitor: ObservableObject {
  static let shared = ServicesMonitor()

  private let checkTimer: AsyncTimerSequence<ContinuousClock>
  private var checkTimerTask: Task<Void, Never>?

  private let servicesWaitingSecondsUpdateTimer: AsyncTimerSequence<ContinuousClock>
  private var servicesWaitingSecondsUpdateTimerTask: Task<Void, Never>?

  private var previousServicesRunning: Bool?
  private var servicesWaitingStartedAt = Date()

  @Published var servicesEnabled = true
  @Published var coreDaemonsRunning = true
  @Published var coreAgentsRunning = true
  @Published var servicesWaitingSeconds = 0

  init() {
    checkTimer = AsyncTimerSequence(
      interval: .seconds(3),
      clock: .continuous)

    servicesWaitingSecondsUpdateTimer = AsyncTimerSequence(
      interval: .seconds(1),
      clock: .continuous)
  }

  public func start() {
    // If Karabiner-Elements was manually terminated just before, the agents are in an unregistered state.
    // So we should enable them once before checking the status.
    libkrbn_services_register_core_daemons()
    libkrbn_services_register_core_agents()

    // When updating Karabiner-Elements, after the new version is installed, the daemons, agents, and Settings will restart.
    // To prevent alerts from appearing at that time, if the daemons or agents are enabled, wait for the timer to fire for the process startup check.
    // If either the daemons or agents are not enabled, usually when the daemons are not approved, trigger the timer immediately after startup to show the alert right away.
    if libkrbn_services_core_daemons_enabled() && libkrbn_services_core_agents_enabled() {
      // Wait for the timer to fire.
    } else {
      check()
    }

    checkTimerTask = Task { @MainActor in
      for await _ in checkTimer {
        self.check()
      }
    }

    servicesWaitingSecondsUpdateTimerTask = Task { @MainActor in
      self.updateServicesWaitingSeconds()

      for await _ in servicesWaitingSecondsUpdateTimer {
        self.updateServicesWaitingSeconds()
      }
    }
  }

  public func stop() {
    servicesWaitingSecondsUpdateTimerTask?.cancel()
    checkTimerTask?.cancel()
  }

  private func check() {
    servicesEnabled =
      libkrbn_services_core_daemons_enabled() && libkrbn_services_core_agents_enabled()
    coreDaemonsRunning = libkrbn_services_core_daemons_running()
    coreAgentsRunning = libkrbn_services_core_agents_running()

    let servicesRunning = coreDaemonsRunning && coreAgentsRunning

    // Display alerts only when the status changes.
    if previousServicesRunning == nil || previousServicesRunning != servicesRunning {
      previousServicesRunning = servicesRunning

      ContentViewStates.shared.showServicesNotRunningAlert = !servicesRunning

      if !servicesRunning {
        servicesWaitingStartedAt = Date()
      }
    }

    if !servicesRunning {
      // For approved services, once they are disabled from System Settings > General > Login Items & Extensions,
      // re-enabling them will not automatically start the service, and it is necessary to call SMAppService.register again.
      // Therefore, if the service is not running, periodically register daemons and agents to restart them after enabled.
      libkrbn_services_register_core_daemons()
      libkrbn_services_register_core_agents()
    }
  }

  private func updateServicesWaitingSeconds() {
    if coreDaemonsRunning && coreAgentsRunning {
      servicesWaitingSeconds = 0
    } else {
      servicesWaitingSeconds = Int(
        (Date().timeIntervalSince(servicesWaitingStartedAt)).rounded())
    }
  }
}
