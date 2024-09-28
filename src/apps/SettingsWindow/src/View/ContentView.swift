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
        OverlayAlertView {
          DriverNotActivatedAlertView()
        }
      } else if contentViewStates.showSettingsAlert {
        OverlayAlertView {
          SettingsAlertView()
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

struct ContentView_Previews: PreviewProvider {
  static var previews: some View {
    ContentView()
  }
}
