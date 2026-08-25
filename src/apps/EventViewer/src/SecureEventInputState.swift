import Foundation

func secureEventInputEnabledChangedCallback(_ enabled: Bool) {
  Task { @MainActor in
    SecureEventInputState.shared.update(enabled)
  }
}

@MainActor
final class SecureEventInputState: ObservableObject {
  static let shared = SecureEventInputState()

  @Published private(set) var enabled = false

  func update(_ enabled: Bool) {
    self.enabled = enabled
  }
}
