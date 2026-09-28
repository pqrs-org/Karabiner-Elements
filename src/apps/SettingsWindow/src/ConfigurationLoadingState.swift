// A successful file load does not make settings editable until its snapshot is applied.
enum ConfigurationLoadingState: Equatable {
  enum Failure: Equatable {
    case permission
    case json
    case other
  }

  enum Event {
    case loadSucceeded
    case loadFailed(Failure)
    case snapshotApplied
    case unavailable
    case stopped
  }

  case stopped
  case loading
  case ready
  case failed(Failure)

  var isReady: Bool {
    self == .ready
  }

  mutating func handle(_ event: Event) {
    switch event {
    case .loadSucceeded:
      // The snapshot may already have arrived through a separate callback.
      if !isReady {
        self = .loading
      }
    case .loadFailed(let failure):
      self = .failed(failure)
    case .snapshotApplied:
      self = .ready
    case .unavailable:
      // Keep the error reason if the load-state callback arrived first.
      if case .failed = self {
        return
      }
      self = .loading
    case .stopped:
      self = .stopped
    }
  }
}
