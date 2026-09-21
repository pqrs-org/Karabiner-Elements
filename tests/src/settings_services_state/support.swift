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
  private var calls = 0
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
  static var queryCount: Int { shared.lock.withLock { shared.calls } }
  static var maxConcurrentCalls: Int { shared.lock.withLock { shared.maximumActiveCalls } }
  static func blockNextQuery() -> QueryGate {
    let gate = QueryGate()
    shared.lock.withLock {
      precondition(shared.nextQueryGate == nil)
      shared.nextQueryGate = gate
    }
    return gate
  }

  static func read(daemons: Bool) -> krbn_service_enabled_state {
    precondition(!Thread.isMainThread, "Service queries must not block the UI thread")
    let (value, gate) = shared.lock.withLock {
      shared.activeCalls += 1
      shared.maximumActiveCalls = max(shared.maximumActiveCalls, shared.activeCalls)
      if daemons { shared.calls += 1 }
      let gate = daemons ? shared.nextQueryGate : nil
      if daemons { shared.nextQueryGate = nil }
      return (daemons ? shared.daemons : shared.agents, gate)
    }
    gate?.waitForRelease()
    shared.lock.withLock { shared.activeCalls -= 1 }
    switch value {
    case true?: return krbn_service_enabled_state_enabled
    case false?: return krbn_service_enabled_state_disabled
    case nil: return krbn_service_enabled_state_unknown
    }
  }
}
@_cdecl("krbn_console_user_server_client_connected")
@MainActor func mockConsoleUserServerConnected() -> Bool { ServiceStatus.connected }
@_cdecl("krbn_services_daemons_enabled")
func mockDaemonsEnabled() -> krbn_service_enabled_state { ServiceStatus.read(daemons: true) }
@_cdecl("krbn_services_agents_enabled")
func mockAgentsEnabled() -> krbn_service_enabled_state { ServiceStatus.read(daemons: false) }
