import Combine

@MainActor
final class NotificationMessage: ObservableObject {
  static let shared = NotificationMessage()

  @Published var body = ""
}
