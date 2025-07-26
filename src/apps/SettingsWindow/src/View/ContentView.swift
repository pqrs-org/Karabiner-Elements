import SwiftUI

struct ContentView: View {
  @ObservedObject private var contentViewStates = ContentViewStates.shared

  private let padding = 6.0

  var body: some View {
    ZStack {
      ContentMainView()

      if contentViewStates.showDoctorAlert {
        OverlayAlertView {
          DoctorAlertView()
        }
      } else if contentViewStates.showSettingsAlert {
        // When performing a clean install, many alerts are displayed.
        // Among them, SettingsAlertView is always displayed first,
        // followed by other permission-related alerts in order.
        // If the display priority of SettingsAlertView is low,
        // it briefly appears before being replaced by other alerts, causing a flickering effect.
        // To avoid this, the display priority of SettingsAlertView should be set higher.
        OverlayAlertView {
          SettingsAlertView()
        }
      } else if contentViewStates.showInputMonitoringPermissionsAlert {
        OverlayAlertView {
          InputMonitoringPermissionsAlertView()
        }
      } else if contentViewStates.showServicesNotRunningAlert {
        OverlayAlertView {
          ServicesNotRunningAlertView()
        }
      } else if contentViewStates.showDriverVersionMismatchedAlert {
        OverlayAlertView {
          DriverVersionMismatchedAlertView()
        }
      } else if contentViewStates.showDriverNotActivatedAlert {
        if #available(macOS 15.0, *) {
          OverlayAlertView {
            DriverNotActivatedAlertView()
          }
        } else {
          OverlayAlertView {
            DriverNotActivatedAlertViewMacOS14()
          }
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
