import Foundation

public class ServicesMonitor {
  static let shared = ServicesMonitor()

  private var timer: Timer?

  public func start() {
    timer = Timer.scheduledTimer(
      withTimeInterval: 3.0,
      repeats: true
    ) { (_: Timer) in
      ContentViewStates.shared.showServicesNotRunningAlert =
        !libkrbn_services_grabber_daemon_running()
    }

    timer?.fire()
  }

  public func stop() {
    timer?.invalidate()
    timer = nil
  }
}
