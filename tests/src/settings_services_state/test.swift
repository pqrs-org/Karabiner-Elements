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
    ServiceStatus.agentsEnabled = true
    client.start()
    try await waitUntil { state.localServicesGuidanceContext.coreAgentsEnabled == true }
    precondition(state.localServicesGuidanceContext.coreDaemonsEnabled == false)
    precondition(state.localServicesGuidanceContext.coreAgentsEnabled == true)
    precondition(state.currentResolvedSetup == .services)
    precondition(state.displayedAlert == .none)

    // Polling must publish changes even when no further connection callbacks arrive.
    ServiceStatus.daemonsEnabled = true
    try await waitUntil { state.setupItemCompleted(.services) }
    precondition(state.currentResolvedSetup == .none)

    // Keep polling past the five-second disconnected warning threshold.
    try await waitUntil { state.consoleUserServerClientDisconnectedForAWhile }
    ServiceStatus.agentsEnabled = false
    try await waitUntil { state.localServicesGuidanceContext.coreAgentsEnabled == false }
    precondition(!state.setupItemCompleted(.services))
    precondition(state.currentResolvedSetup == .services)

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

    // Reconnection must preserve locally obtained enabled status.
    ServiceStatus.connected = true
    client.settingsWindowGuidanceReceived(
      SettingsWindowGuidanceState(
        currentSetup: .none,
        currentAlert: .none,
        guidanceContext: remoteContext))
    precondition(!state.consoleUserServerClientDisconnectedForAWhile)
    precondition(state.localServicesGuidanceContext.coreDaemonsEnabled == true)
    precondition(state.localServicesGuidanceContext.coreAgentsEnabled == false)
    precondition(!state.setupItemCompleted(.services))

    // Disconnecting clears remote guidance but preserves local enabled status.
    ServiceStatus.connected = false
    client.updateConsoleUserServerClientState()
    precondition(state.guidanceContext.coreDaemonsRunning == nil)
    precondition(state.localServicesGuidanceContext.coreDaemonsEnabled == true)
    precondition(state.localServicesGuidanceContext.coreAgentsEnabled == false)
    ServiceStatus.agentsEnabled = true
    try await waitUntil { state.setupItemCompleted(.services) }

    // Stop disconnected polling and wait for a fresh result, not an estimated delay.
    ServiceStatus.connected = true
    ServiceStatus.daemonsEnabled = false
    client.settingsWindowGuidanceReceived(
      SettingsWindowGuidanceState(
        currentSetup: .none, currentAlert: .none,
        guidanceContext: SettingsWindowGuidanceContext()))
    try await waitUntil { state.localServicesGuidanceContext.coreDaemonsEnabled == false }

    // Hold a query indefinitely while MainActor submits a burst of requests.
    ServiceStatus.daemonsEnabled = true
    let firstQuery = ServiceStatus.blockNextQuery()
    client.updateLocalServicesGuidanceContext()
    try await waitUntil { firstQuery.hasStarted }
    let queriesAtFirstStart = ServiceStatus.queryCount
    for _ in 0..<20 { client.updateLocalServicesGuidanceContext() }
    precondition(ServiceStatus.queryCount == queriesAtFirstStart)

    // Hold the follow-up too, so its start proves the first result was applied.
    let followUp = ServiceStatus.blockNextQuery()
    ServiceStatus.daemonsEnabled = false
    firstQuery.release()
    try await waitUntil { followUp.hasStarted }
    precondition(state.localServicesGuidanceContext.coreDaemonsEnabled == true)
    precondition(ServiceStatus.queryCount == queriesAtFirstStart + 1)
    precondition(ServiceStatus.maxConcurrentCalls == 1)
    followUp.release()
    try await waitUntil { state.localServicesGuidanceContext.coreDaemonsEnabled == false }

    ServiceStatus.daemonsEnabled = true
    client.updateLocalServicesGuidanceContext()
    try await waitUntil { state.setupItemCompleted(.services) }

    // Discard a query started before components stopped, even if it completes later.
    ServiceStatus.daemonsEnabled = false
    let oldQuery = ServiceStatus.blockNextQuery()
    client.updateLocalServicesGuidanceContext()
    try await waitUntil { oldQuery.hasStarted }
    ServiceStatus.daemonsEnabled = true
    ServiceStatus.agentsEnabled = false
    let freshQuery = ServiceStatus.blockNextQuery()
    client.componentsManagerStopped()
    oldQuery.release()
    try await waitUntil { freshQuery.hasStarted }
    // The old query captured false. Keep the fresh query blocked while checking that
    // the old result was discarded, rather than being overwritten quickly by a new one.
    precondition(state.localServicesGuidanceContext.coreDaemonsEnabled == true)
    precondition(state.localServicesGuidanceContext.coreAgentsEnabled == true)
    freshQuery.release()
    try await waitUntil { state.localServicesGuidanceContext.coreAgentsEnabled == false }
    precondition(state.localServicesGuidanceContext.coreDaemonsEnabled == true)
    precondition(!state.setupItemCompleted(.services))
    // Failed commands cross the actual C enum bridge as unknown, not disabled.
    ServiceStatus.daemonsEnabled = nil
    ServiceStatus.agentsEnabled = nil
    client.updateLocalServicesGuidanceContext()
    try await waitUntil {
      state.localServicesGuidanceContext.coreDaemonsEnabled == nil
        && state.localServicesGuidanceContext.coreAgentsEnabled == nil
    }
    precondition(state.localServicesGuidanceContext.servicesEnabled == nil)
    precondition(!state.setupItemCompleted(.services))
    precondition(state.currentResolvedSetup == .none)

    // One known disabled service is sufficient to require setup, even if the other is unknown.
    ServiceStatus.daemonsEnabled = false
    client.updateLocalServicesGuidanceContext()
    try await waitUntil { state.localServicesGuidanceContext.coreDaemonsEnabled == false }
    precondition(state.localServicesGuidanceContext.coreAgentsEnabled == nil)
    precondition(state.localServicesGuidanceContext.servicesEnabled == false)
    precondition(state.currentResolvedSetup == .services)

    ServiceStatus.daemonsEnabled = true
    client.updateLocalServicesGuidanceContext()
    try await waitUntil { state.localServicesGuidanceContext.coreDaemonsEnabled == true }
    precondition(state.localServicesGuidanceContext.servicesEnabled == nil)
    precondition(state.currentResolvedSetup == .none)

    ServiceStatus.agentsEnabled = true
    client.updateLocalServicesGuidanceContext()
    try await waitUntil { state.setupItemCompleted(.services) }
    print("Settings services state tests passed")
  }
}
