import Foundation

public class ServicesMonitor: ObservableObject {
  static let shared = ServicesMonitor()

  private var timer: Timer?

  @Published var coreDaemonsRunning = true
  @Published var coreAgentsRunning = true

  public func start() {
    timer = Timer.scheduledTimer(
      withTimeInterval: 3.0,
      repeats: true
    ) { [weak self] (_: Timer) in
      guard let self = self else { return }

      self.coreDaemonsRunning = libkrbn_services_core_daemons_running()
      self.coreAgentsRunning = libkrbn_services_core_agents_running()

      let servicesRunning = coreDaemonsRunning && coreAgentsRunning

      ContentViewStates.shared.showServicesNotRunningAlert = !servicesRunning

      if !servicesRunning {
        // For approved services, once they are disabled from System Settings > General > Login Items,
        // re-enabling them will not automatically start the service, and it is necessary to call SMAppService.register again.
        // Therefore, if the service is not running, periodically register daemons and agents to restart them after enabled.
        libkrbn_services_register_core_daemons()
        libkrbn_services_register_core_agents()
      }
    }

    timer?.fire()
  }

  public func stop() {
    timer?.invalidate()
    timer = nil
  }
}
