import Foundation

public class ServicesMonitor {
  static let shared = ServicesMonitor()

  private var timer: Timer?

  @Published var daemonRunning = true
  @Published var agentRunning = true

  public func start() {
    timer = Timer.scheduledTimer(
      withTimeInterval: 3.0,
      repeats: true
    ) { [weak self] (_: Timer) in
      guard let self = self else { return }

      self.daemonRunning = libkrbn_services_grabber_daemon_running()
      //let agentRunning = libkrbn_services_console_user_server_agent_running()
      self.agentRunning = false

      let servicesRunning = daemonRunning && agentRunning

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
