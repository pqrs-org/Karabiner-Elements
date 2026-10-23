import Foundation

@MainActor
final class ServicesRegister {
  static let shared = ServicesRegister()

  private let queue = DispatchQueue(
    label: "org.pqrs.Karabiner-Elements.Settings.services-register", qos: .utility)
  private var inFlight = false
  private var coreDaemonsOnceRunning = false
  private var coreAgentsOnceRunning = false
  private var pollingTask: Task<Void, Never>?
  private let interval: Duration

  init(interval: Duration = .seconds(3)) {
    self.interval = interval
  }

  func start() {
    guard pollingTask == nil else { return }

    registerCoreServices()

    let interval = interval
    pollingTask = Task { @MainActor [weak self] in
      while !Task.isCancelled {
        try? await Task.sleep(for: interval)
        guard !Task.isCancelled else { return }
        guard let self else { return }
        self.registerStoppedServices()
      }
    }
  }

  private func registerCoreServices() {
    // The services may be unregistered if Karabiner-Elements was manually terminated.
    // Register both service groups once before checking whether they are running.
    guard !inFlight else { return }
    inFlight = true

    queue.async { [weak self] in
      krbn_services_register_core_daemons()
      krbn_services_register_core_agents()

      Task { @MainActor [weak self] in
        self?.inFlight = false
      }
    }
  }

  private func registerStoppedServices() {
    // On macOS 13, approving a previously disabled background item does not always bootstrap
    // its launchd jobs. While a service group is enabled but not running, register it again.
    // Once running, stop checking that group for this Settings process's lifetime.
    guard !coreDaemonsOnceRunning || !coreAgentsOnceRunning else {
      pollingTask?.cancel()
      pollingTask = nil
      return
    }
    guard !inFlight else { return }
    inFlight = true

    let daemonsOnceRunning = coreDaemonsOnceRunning
    let agentsOnceRunning = coreAgentsOnceRunning
    queue.async { [weak self] in
      let daemonsEnabled =
        daemonsOnceRunning ? true : krbn_services_daemons_enabled().boolValue
      let agentsEnabled =
        agentsOnceRunning ? true : krbn_services_agents_enabled().boolValue

      let daemonsRunning =
        daemonsEnabled == true
        ? (daemonsOnceRunning ? true : krbn_services_daemons_running().boolValue) : nil
      let agentsRunning =
        agentsEnabled == true
        ? (agentsOnceRunning ? true : krbn_services_agents_running().boolValue) : nil

      if daemonsEnabled == true && daemonsRunning == false {
        krbn_services_register_core_daemons()
      }
      if agentsEnabled == true && agentsRunning == false {
        krbn_services_register_core_agents()
      }

      Task { @MainActor [weak self] in
        guard let self else { return }
        self.inFlight = false
        self.coreDaemonsOnceRunning = daemonsRunning == true
        self.coreAgentsOnceRunning = agentsRunning == true
      }
    }
  }
}
