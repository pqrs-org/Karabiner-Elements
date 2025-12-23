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
    // Note:
    // The socket file path length must be <= 103 because sizeof(sockaddr_un.sun_path) == 104.
    // So we use the shorten name event_viewer_console_user_server_client -> ev_cus_clnt.
    //
    // Example:
    // `/Library/Application Support/org.pqrs/tmp/user/501/ev_cus_clnt/1883ad727b341fe8.sock`

    libkrbn_enable_console_user_server_client(geteuid(), "ev_cus_clnt")

    libkrbn_console_user_server_client_async_start()
  }
}
