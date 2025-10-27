import Combine

private func callback() {
  Task { @MainActor in
    EVGrabberClient.shared.temporarilyIgnoreAllDevices = false
  }
}

@MainActor
final class EVGrabberClient: ObservableObject {
  static let shared = EVGrabberClient()

  // We register the callback in the `start` method rather than in `init`.
  // If libkrbn_register_*_callback is called within init,
  // there is a risk that `init` could be invoked again from the callback through `shared` before the initial `init` completes.

  public func start() {
    // Note:
    // The socket file path length must be <= 103 because sizeof(sockaddr_un.sun_path) == 104.
    // So we use the shorten name event_viewer_grabber_client -> ev_grb_clnt.
    //
    // Example:
    // `/Library/Application Support/org.pqrs/tmp/user/501/ev_grb_clnt/186745e8160a7b98.sock`

    libkrbn_enable_grabber_client("ev_grb_clnt")

    libkrbn_register_grabber_client_status_changed_callback(callback)

    libkrbn_grabber_client_async_start()
  }

  @Published var temporarilyIgnoreAllDevices: Bool = false {
    didSet {
      libkrbn_grabber_client_async_temporarily_ignore_all_devices(temporarilyIgnoreAllDevices)
    }
  }
}
