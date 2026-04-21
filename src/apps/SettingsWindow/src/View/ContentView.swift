import SwiftUI

struct ContentView: View {
  @ObservedObject private var contentViewStates = ContentViewStates.shared

  private let padding = 6.0

  var body: some View {
    ZStack {
      ContentMainView()

      if contentViewStates.displayedAlert == .doctor {
        OverlayAlertView {
          DoctorAlertView()
        }
      } else if contentViewStates.displayedAlert == .settings {
        OverlayAlertView {
          SettingsAlertView()
        }
      } else if contentViewStates.displayedAlert == .consoleUserServerNotConnected {
        OverlayAlertView {
          ConsoleUserServerNotConnectedAlertView()
        }
      } else if contentViewStates.displayedAlert == .virtualHidDeviceServiceClientNotConnected {
        OverlayAlertView {
          VirtualHidDeviceServiceClientNotConnectedAlertView()
        }
      } else if contentViewStates.displayedAlert == .driverVersionMismatched {
        OverlayAlertView {
          DriverVersionMismatchedAlertView()
        }
      } else if contentViewStates.displayedAlert == .driverNotConnected {
        OverlayAlertView {
          DriverNotConnectedAlertView()
        }
      }
    }
    .frame(
      minWidth: 1100,
      maxWidth: .infinity,
      minHeight: 680,
      maxHeight: .infinity
    )
  }
}
