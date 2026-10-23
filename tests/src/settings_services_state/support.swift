import Foundation

// UI-only dependencies of ContentViewStates; service and guidance logic use production sources.
struct SettingsToast { let message: String }
struct ConnectedDevice {}
enum ComplexModificationsSheetView { case unused }
enum SidebarItem { case simpleModifications, setup }
enum SetupItem: CaseIterable {
  case services, accessibility, inputMonitoring, driverExtension

  static func from(setup: SettingsWindowGuidanceSetup) -> Self? {
    switch setup {
    case .none: nil
    case .services: .services
    case .accessibility: .accessibility
    case .inputMonitoring: .inputMonitoring
    case .driverExtension: .driverExtension
    }
  }
}

// Only the background query blocks here; the test continues running on MainActor.
final class QueryGate: @unchecked Sendable {
  private let lock = NSLock()
  private let completion = DispatchSemaphore(value: 0)
  private var started = false

  var hasStarted: Bool { lock.withLock { started } }

  func waitForRelease() {
    lock.withLock { started = true }
    precondition(completion.wait(timeout: .now() + 15) == .success, "Query was never released")
  }

  func release() {
    completion.signal()
  }
}

// Simulate system service changes without changing login items on the test machine.
// The production client now reads enabled status on a background queue.
final class ServiceStatus: @unchecked Sendable {
  static let shared = ServiceStatus()
  private let lock = NSLock()
  private var daemons: Bool? = false
  private var agents: Bool? = false
  private var daemonsRunning: Bool? = true
  private var agentsRunning: Bool? = true
  private var calls = 0
  private var agentCalls = 0
  private var daemonRunningCalls = 0
  private var agentRunningCalls = 0
  private var daemonRegistrations = 0
  private var agentRegistrations = 0
  private var activeCalls = 0
  private var maximumActiveCalls = 0
  private var nextQueryGate: QueryGate?
  @MainActor static var connected = false

  static var daemonsEnabled: Bool? {
    get { shared.lock.withLock { shared.daemons } }
    set { shared.lock.withLock { shared.daemons = newValue } }
  }
  static var agentsEnabled: Bool? {
    get { shared.lock.withLock { shared.agents } }
    set { shared.lock.withLock { shared.agents = newValue } }
  }
  static var daemonsAreRunning: Bool? {
    get { shared.lock.withLock { shared.daemonsRunning } }
    set { shared.lock.withLock { shared.daemonsRunning = newValue } }
  }
  static var agentsAreRunning: Bool? {
    get { shared.lock.withLock { shared.agentsRunning } }
    set { shared.lock.withLock { shared.agentsRunning = newValue } }
  }
  static var queryCount: Int { shared.lock.withLock { shared.calls } }
  static var agentQueryCount: Int { shared.lock.withLock { shared.agentCalls } }
  static var daemonRunningQueryCount: Int { shared.lock.withLock { shared.daemonRunningCalls } }
  static var agentRunningQueryCount: Int { shared.lock.withLock { shared.agentRunningCalls } }
  static var daemonRegistrationCount: Int { shared.lock.withLock { shared.daemonRegistrations } }
  static var agentRegistrationCount: Int { shared.lock.withLock { shared.agentRegistrations } }
  static var maxConcurrentCalls: Int { shared.lock.withLock { shared.maximumActiveCalls } }
  static func blockNextQuery() -> QueryGate {
    let gate = QueryGate()
    shared.lock.withLock {
      precondition(shared.nextQueryGate == nil)
      shared.nextQueryGate = gate
    }
    return gate
  }

  static func readEnabled(daemons: Bool) -> krbn_service_state {
    precondition(!Thread.isMainThread, "Service queries must not block the UI thread")
    let (value, gate) = shared.lock.withLock {
      shared.activeCalls += 1
      shared.maximumActiveCalls = max(shared.maximumActiveCalls, shared.activeCalls)
      if daemons { shared.calls += 1 } else { shared.agentCalls += 1 }
      let gate = daemons ? shared.nextQueryGate : nil
      if daemons { shared.nextQueryGate = nil }
      return (daemons ? shared.daemons : shared.agents, gate)
    }
    gate?.waitForRelease()
    shared.lock.withLock { shared.activeCalls -= 1 }
    switch value {
    case true?: return krbn_service_state_true
    case false?: return krbn_service_state_false
    case nil: return krbn_service_state_unknown
    }
  }

  static func readRunning(daemons: Bool) -> krbn_service_state {
    precondition(!Thread.isMainThread, "Service queries must not block the UI thread")
    let value = shared.lock.withLock {
      if daemons {
        shared.daemonRunningCalls += 1
      } else {
        shared.agentRunningCalls += 1
      }
      return daemons ? shared.daemonsRunning : shared.agentsRunning
    }
    switch value {
    case true?: return krbn_service_state_true
    case false?: return krbn_service_state_false
    case nil: return krbn_service_state_unknown
    }
  }

  static func register(daemons: Bool) {
    precondition(!Thread.isMainThread, "Service registration must not block the UI thread")
    shared.lock.withLock {
      if daemons {
        shared.daemonRegistrations += 1
      } else {
        shared.agentRegistrations += 1
      }
    }
  }
}
@_cdecl("krbn_console_user_server_client_connected")
@MainActor func mockConsoleUserServerConnected() -> Bool { ServiceStatus.connected }
@_cdecl("krbn_services_daemons_enabled")
func mockDaemonsEnabled() -> krbn_service_state { ServiceStatus.readEnabled(daemons: true) }
@_cdecl("krbn_services_agents_enabled")
func mockAgentsEnabled() -> krbn_service_state { ServiceStatus.readEnabled(daemons: false) }
@_cdecl("krbn_services_daemons_running")
func mockDaemonsRunning() -> krbn_service_state { ServiceStatus.readRunning(daemons: true) }
@_cdecl("krbn_services_agents_running")
func mockAgentsRunning() -> krbn_service_state { ServiceStatus.readRunning(daemons: false) }
@_cdecl("krbn_services_register_core_daemons")
func mockRegisterCoreDaemons() { ServiceStatus.register(daemons: true) }
@_cdecl("krbn_services_register_core_agents")
func mockRegisterCoreAgents() { ServiceStatus.register(daemons: false) }
