import Foundation

@main @MainActor struct Tests {
  static func waitUntil(_ condition: @MainActor () -> Bool) async throws {
    // The deadline is only a failure bound; success depends on the observed state.
    let clock = ContinuousClock()
    let deadline = clock.now.advanced(by: .seconds(10))
    while clock.now < deadline {
      if condition() { return }
      try await Task.sleep(for: .milliseconds(10))
    }
    preconditionFailure("Timed out waiting for service state")
  }

  static func main() async throws {
    let state = ContentViewStates.shared
    // Use the production client, but not its real service queries: the test imports
    // settings.hpp without linking settings.cpp. support.swift provides the C symbols
    // via @_cdecl, so the client's krbn_* calls resolve to mocks at link time.
    // No connection to the installed services or changes to login items are made.
    let client = SettingsConsoleUserServerClient.shared

    precondition(state.localServicesGuidanceContext.coreDaemonsEnabled == nil)
    precondition(state.localServicesGuidanceContext.coreAgentsEnabled == nil)
    precondition(!state.setupItemCompleted(.services))

    // An initial failed connection must still obtain the local enabled state.
    ServiceStatus.agentsEnabled = nil
    client.start()
    try await waitUntil { state.localServicesGuidanceContext.coreDaemonsEnabled == false }
    precondition(state.localServicesGuidanceContext.coreAgentsEnabled == nil)
    precondition(state.currentResolvedSetup == .services)
    precondition(state.displayedAlert == .none)

    // Polling must continue past the disconnected warning while services are unconfirmed.
    try await waitUntil { state.consoleUserServerClientDisconnectedForAWhile }
    ServiceStatus.daemonsEnabled = nil
    try await waitUntil { state.localServicesGuidanceContext.coreDaemonsEnabled == nil }
    precondition(state.localServicesGuidanceContext.servicesEnabled == nil)
    precondition(state.currentResolvedSetup == .none)

    // Decode running status from the current wire format.
    let remoteContext = try JSONDecoder().decode(
      SettingsWindowGuidanceContext.self,
      from: Data(
        #"{"core_daemons_running":true,"core_agents_running":null}"#.utf8))
    precondition(remoteContext.coreDaemonsRunning == true)
    precondition(remoteContext.coreAgentsRunning == nil)
    let encodedContext =
      try JSONSerialization.jsonObject(
        with: JSONEncoder().encode(remoteContext)) as! [String: Any]
    precondition(Set(encodedContext.keys) == ["core_daemons_running"])
    let stoppedContext = try JSONDecoder().decode(
      SettingsWindowGuidanceContext.self,
      from: Data(#"{"core_daemons_running":null,"core_agents_running":false}"#.utf8))
    precondition(stoppedContext.coreDaemonsRunning == nil)
    precondition(stoppedContext.coreAgentsRunning == false)

    // Reconnection preserves local state and stops disconnected polling.
    ServiceStatus.connected = true
    client.settingsWindowGuidanceReceived(
      SettingsWindowGuidanceState(
        currentSetup: .none, currentAlert: .none, guidanceContext: remoteContext))
    precondition(!state.consoleUserServerClientDisconnectedForAWhile)
    precondition(state.localServicesGuidanceContext.coreDaemonsEnabled == nil)

    // Hold a query while MainActor submits a burst. Unconfirmed results still get one follow-up.
    ServiceStatus.daemonsEnabled = false
    let firstQuery = ServiceStatus.blockNextQuery()
    client.updateLocalServicesGuidanceContext()
    try await waitUntil { firstQuery.hasStarted }
    let queriesAtFirstStart = ServiceStatus.queryCount
    for _ in 0..<20 { client.updateLocalServicesGuidanceContext() }
    precondition(ServiceStatus.queryCount == queriesAtFirstStart)

    let followUp = ServiceStatus.blockNextQuery()
    ServiceStatus.daemonsEnabled = nil
    firstQuery.release()
    try await waitUntil { followUp.hasStarted }
    precondition(state.localServicesGuidanceContext.coreDaemonsEnabled == false)
    precondition(ServiceStatus.queryCount == queriesAtFirstStart + 1)
    precondition(ServiceStatus.maxConcurrentCalls == 1)
    followUp.release()
    try await waitUntil { state.localServicesGuidanceContext.coreDaemonsEnabled == nil }

    // A true result from before components stopped must not latch the enabled state.
    ServiceStatus.daemonsEnabled = true
    let oldQuery = ServiceStatus.blockNextQuery()
    client.updateLocalServicesGuidanceContext()
    try await waitUntil { oldQuery.hasStarted }
    ServiceStatus.daemonsEnabled = false
    let freshQuery = ServiceStatus.blockNextQuery()
    client.componentsManagerStopped()
    oldQuery.release()
    try await waitUntil { freshQuery.hasStarted }
    precondition(state.localServicesGuidanceContext.coreDaemonsEnabled == nil)
    freshQuery.release()
    try await waitUntil { state.localServicesGuidanceContext.coreDaemonsEnabled == false }
    precondition(state.currentResolvedSetup == .services)

    // Confirm agents first. Further retries must query only daemons.
    ServiceStatus.agentsEnabled = true
    client.updateLocalServicesGuidanceContext()
    try await waitUntil { state.localServicesGuidanceContext.coreAgentsEnabled == true }
    let confirmedAgentQueries = ServiceStatus.agentQueryCount
    ServiceStatus.agentsEnabled = false
    ServiceStatus.daemonsEnabled = nil
    client.updateLocalServicesGuidanceContext()
    try await waitUntil { state.localServicesGuidanceContext.coreDaemonsEnabled == nil }
    precondition(ServiceStatus.agentQueryCount == confirmedAgentQueries)
    precondition(state.localServicesGuidanceContext.coreAgentsEnabled == true)

    // Once the last group is enabled, even a pending follow-up must launch no process.
    ServiceStatus.daemonsEnabled = true
    let finalQuery = ServiceStatus.blockNextQuery()
    client.updateLocalServicesGuidanceContext()
    try await waitUntil { finalQuery.hasStarted }
    for _ in 0..<20 { client.updateLocalServicesGuidanceContext() }
    finalQuery.release()
    try await waitUntil { state.setupItemCompleted(.services) }
    let confirmedDaemonQueries = ServiceStatus.queryCount
    ServiceStatus.daemonsEnabled = false

    // Connection changes and component resets must preserve confirmed states.
    ServiceStatus.connected = false
    client.componentsManagerStopped()
    precondition(state.guidanceContext.coreDaemonsRunning == nil)
    for _ in 0..<20 { client.updateLocalServicesGuidanceContext() }
    try await waitUntil { state.consoleUserServerClientDisconnectedForAWhile }
    precondition(ServiceStatus.queryCount == confirmedDaemonQueries)
    precondition(ServiceStatus.agentQueryCount == confirmedAgentQueries)
    precondition(state.setupItemCompleted(.services))

    ServiceStatus.connected = true
    client.settingsWindowGuidanceReceived(
      SettingsWindowGuidanceState(
        currentSetup: .none, currentAlert: .none, guidanceContext: remoteContext))
    precondition(state.localServicesGuidanceContext.coreDaemonsEnabled == true)
    precondition(state.localServicesGuidanceContext.coreAgentsEnabled == true)

    // The opposite order also skips confirmed daemons while agents still need checking.
    let anotherClient = SettingsConsoleUserServerClient()
    ServiceStatus.daemonsEnabled = true
    ServiceStatus.agentsEnabled = false
    anotherClient.updateLocalServicesGuidanceContext()
    try await waitUntil { state.localServicesGuidanceContext.coreAgentsEnabled == false }
    let daemonQueries = ServiceStatus.queryCount
    ServiceStatus.daemonsEnabled = false
    ServiceStatus.agentsEnabled = true
    anotherClient.updateLocalServicesGuidanceContext()
    try await waitUntil { state.setupItemCompleted(.services) }
    precondition(ServiceStatus.queryCount == daemonQueries)
    precondition(state.localServicesGuidanceContext.coreDaemonsEnabled == true)
    print("Settings services state tests passed")
  }
}
