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
      } else if contentViewStates.displayedAlert == .servicesNotRunning {
        OverlayAlertView {
          ServicesNotRunningAlertView()
        }
      } else if contentViewStates.displayedAlert == .inputMonitoringPermissions {
        OverlayAlertView {
          InputMonitoringPermissionsAlertView()
        }
      } else if contentViewStates.displayedAlert == .accessibility {
        OverlayAlertView {
          AccessibilityAlertView()
        }
      } else if contentViewStates.displayedAlert == .virtualHidDeviceServiceClientNotConnected {
        OverlayAlertView {
          VirtualHidDeviceServiceClientNotConnectedAlertView()
        }
      } else if contentViewStates.displayedAlert == .driverVersionMismatched {
        OverlayAlertView {
          DriverVersionMismatchedAlertView()
        }
      } else if contentViewStates.displayedAlert == .driverNotActivated {
        if #available(macOS 15.0, *) {
          OverlayAlertView {
            DriverNotActivatedAlertView()
          }
        } else {
          OverlayAlertView {
            DriverNotActivatedAlertViewMacOS14()
          }
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
