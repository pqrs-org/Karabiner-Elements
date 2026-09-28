func testConfigurationLoadingState() {
  var state = ConfigurationLoadingState.stopped
  precondition(!state.isReady)

  state.handle(.loadSucceeded)
  precondition(state == .loading, "A successful load must wait for its snapshot")
  state.handle(.snapshotApplied)
  precondition(state.isReady)
  state.handle(.loadSucceeded)
  precondition(state.isReady, "A late load notification must not hide an applied snapshot")

  state.handle(.loadFailed(.json))
  precondition(!state.isReady)
  state.handle(.unavailable)
  precondition(
    state == .failed(.json), "The unavailable snapshot must preserve the error reason")

  state.handle(.loadSucceeded)
  precondition(state == .loading, "Recovery must also wait for its snapshot")
  state.handle(.snapshotApplied)
  precondition(state.isReady)

  state.handle(.unavailable)
  precondition(!state.isReady)
  state.handle(.loadFailed(.permission))
  precondition(
    state == .failed(.permission), "Both error callback orders must reach the same state")

  state.handle(.stopped)
  precondition(state == .stopped)
  state.handle(.snapshotApplied)
  precondition(state.isReady, "A new session can deliver its snapshot before its load notification")
}
