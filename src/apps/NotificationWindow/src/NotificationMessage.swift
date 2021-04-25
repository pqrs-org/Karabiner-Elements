import Combine

final class NotificationMessage: ObservableObject {
    static let shared = NotificationMessage()

    @Published var text = ""
}
