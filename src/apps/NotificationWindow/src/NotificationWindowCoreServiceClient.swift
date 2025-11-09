import AsyncAlgorithms
import Combine

private func notificationMessageReceivedCallback(_ message: UnsafePointer<CChar>) {
  let s = String(cString: message)

  Task { @MainActor in
    if NotificationWindowCoreServiceClient.shared.message != s {
      NotificationWindowCoreServiceClient.shared.message = s

      NotificationWindowManager.shared.updateWindowsVisibility()
    }
  }
}

@MainActor
final class NotificationWindowCoreServiceClient: ObservableObject {
  static let shared = NotificationWindowCoreServiceClient()

  private let timer: AsyncTimerSequence<ContinuousClock>
  private var timerTask: Task<Void, Never>?

  @Published var message = ""

  init() {
    timer = AsyncTimerSequence(
      interval: .milliseconds(500),
      clock: .continuous
    )
  }

  // We register the callback in the `start` method rather than in `init`.
  // If libkrbn_register_*_callback is called within init,
  // there is a risk that `init` could be invoked again from the callback through `shared` before the initial `init` completes.

  public func start() {
    // Note:
    // The socket file path length must be <= 103 because sizeof(sockaddr_un.sun_path) == 104.
    // So we use the shorten name notification_window_core_service_client -> nw_cs_clnt.
    //
    // Example:
    // `/Library/Application Support/org.pqrs/tmp/user/501/nw_cs_clnt/1875f6b751b3bc98.sock`

    libkrbn_enable_core_service_client("nw_cs_clnt")

    libkrbn_register_core_service_client_notification_message_received_callback(
      notificationMessageReceivedCallback)

    libkrbn_core_service_client_async_start()

    timerTask = Task { @MainActor in
      libkrbn_core_service_client_async_get_notification_message()

      for await _ in timer {
        libkrbn_core_service_client_async_get_notification_message()
      }
    }
  }
}
