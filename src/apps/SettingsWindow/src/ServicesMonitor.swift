import Foundation

public class ServicesMonitor: ObservableObject {
  static let shared = ServicesMonitor()

  private var timer: Timer?
  private var previousServicesRunning: Bool?
  private var servicesWaitingSecondsUpdateTimer: Timer?
  private var servicesWaitingStartedAt = Date()

  @Published var servicesEnabled = true
  @Published var coreDaemonsRunning = true
  @Published var coreAgentsRunning = true
  @Published var servicesWaitingSeconds = 0

  public func start() {
    timer = Timer.scheduledTimer(
      withTimeInterval: 3.0,
      repeats: true
    ) { [weak self] (_: Timer) in
      guard let self = self else { return }

      self.servicesEnabled =
        libkrbn_services_core_daemons_enabled() && libkrbn_services_core_agents_enabled()
      self.coreDaemonsRunning = libkrbn_services_core_daemons_running()
      self.coreAgentsRunning = libkrbn_services_core_agents_running()

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
        // For approved services, once they are disabled from System Settings > General > Login Items,
        // re-enabling them will not automatically start the service, and it is necessary to call SMAppService.register again.
        // Therefore, if the service is not running, periodically register daemons and agents to restart them after enabled.
        libkrbn_services_register_core_daemons()
        libkrbn_services_register_core_agents()
      }
    }

    // When updating Karabiner-Elements, after the new version is installed, the daemons, agents, and Settings will restart.
    // To prevent alerts from appearing at that time, if the daemons or agents are enabled, wait for the timer to fire for the process startup check.
    // If either the daemons or agents are not enabled, usually when the daemons are not approved, trigger the timer immediately after startup to show the alert right away.
    if libkrbn_services_core_daemons_enabled() && libkrbn_services_core_agents_enabled() {
      // Wait for the timer to fire.
    } else {
      timer?.fire()
    }

    servicesWaitingSecondsUpdateTimer = Timer.scheduledTimer(
      withTimeInterval: 1.0,
      repeats: true
    ) { [weak self] (_: Timer) in
      guard let self = self else { return }

      if self.coreDaemonsRunning && self.coreAgentsRunning {
        self.servicesWaitingSeconds = 0
      } else {
        self.servicesWaitingSeconds = Int(
          (Date().timeIntervalSince(self.servicesWaitingStartedAt)).rounded())
      }
    }
  }

  public func stop() {
    servicesWaitingSecondsUpdateTimer?.invalidate()
    servicesWaitingSecondsUpdateTimer = nil

    timer?.invalidate()
    timer = nil
  }
}
