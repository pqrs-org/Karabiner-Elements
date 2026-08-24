import Foundation

@MainActor
final class CaptureSessionManager {
  struct Session: Equatable {
    fileprivate let generation: UInt64
  }

  static let shared = CaptureSessionManager()

  private var generation: UInt64 = 0
  private var currentGeneration: UInt64?

  func begin() -> Session {
    generation &+= 1
    currentGeneration = generation
    return Session(generation: generation)
  }

  func end(_ session: Session) -> Bool {
    // SwiftUI may start the replacement view's task before running the old
    // task's defer. Only the latest view is allowed to stop the capture.
    guard currentGeneration == session.generation else {
      return false
    }

    currentGeneration = nil
    return true
  }
}
