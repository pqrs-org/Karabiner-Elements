import SwiftUI

struct ContentView: View {
  @ObservedObject private var contentViewStates = ContentViewStates.shared

  private let padding = 6.0

  var body: some View {
    VStack {
      ContentMainView()
        .alert(
          isPresented: contentViewStates.showDriverNotActivatedAlert
        ) { DriverNotActivatedAlertView() }
        .alert(
          isPresented: contentViewStates.showDriverVersionMismatchedAlert
        ) { DriverVersionMismatchedAlertView() }
        .alert(
          isPresented: contentViewStates.showInputMonitoringPermissionsAlert
        ) { InputMonitoringPermissionsAlertView() }
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
