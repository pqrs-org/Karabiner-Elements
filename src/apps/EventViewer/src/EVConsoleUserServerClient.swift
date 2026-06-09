import AsyncAlgorithms
import Combine
import Foundation

@MainActor
final class EVConsoleUserServerClient: ObservableObject {
  static let shared = EVConsoleUserServerClient()

  // We register the callback in the `start` method rather than in `init`.
  // If libkrbn_register_*_callback is called within init,
  // there is a risk that `init` could be invoked again from the callback through `shared` before the initial `init` completes.

  public func start() {
    libkrbn_enable_console_user_server_client(geteuid())

    libkrbn_console_user_server_client_async_start()
  }
}
