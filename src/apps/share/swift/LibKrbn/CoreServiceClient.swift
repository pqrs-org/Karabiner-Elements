import SwiftUI

private func callback() {
  let status = libkrbn_core_service_client_get_status()

  Task { @MainActor in
    if status == libkrbn_core_service_client_status_connected {
      LibKrbn.CoreServiceClient.shared.connected = true
    } else {
      LibKrbn.CoreServiceClient.shared.connected = false
    }
  }
}

extension LibKrbn {
  @MainActor
  public class CoreServiceClient: ObservableObject {
    public static let shared = CoreServiceClient()

    @Published public fileprivate(set) var connected = false

    // We register the callback in the `start` method rather than in `init`.
    // If libkrbn_register_*_callback is called within init, there is a risk that `init` could be invoked again from the callback through `shared` before the initial `init` completes.

    public func start(_ clientSocketDirectoryName: String) {
      clientSocketDirectoryName.withCString {
        libkrbn_enable_core_service_client(clientSocketDirectoryName == "" ? nil : $0)
      }
      libkrbn_register_core_service_client_status_changed_callback(callback)
      libkrbn_core_service_client_async_start()
    }

    public func setAppIcon(_ number: Int32) {
      libkrbn_core_service_client_async_set_app_icon(number)
    }
  }
}
