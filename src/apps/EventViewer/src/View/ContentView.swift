import SwiftUI

struct ContentView: View {
  @ObservedObject var inputMonitoringAlertData = InputMonitoringAlertData.shared

  var body: some View {
    ZStack {
      ContentMainView()

      if inputMonitoringAlertData.showing {
        OverlayAlertView {
          InputMonitoringAlertView()
        }
      }
    }
    .frame(
      minWidth: 1100,
      maxWidth: .infinity,
      minHeight: 650,
      maxHeight: .infinity)
  }
}

struct ContentView_Previews: PreviewProvider {
  static var previews: some View {
    ContentView()
  }
}
