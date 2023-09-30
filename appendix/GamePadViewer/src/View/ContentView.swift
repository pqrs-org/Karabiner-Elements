import SwiftUI

struct ContentView: View {
  @ObservedObject var inputMonitoringAlertData = InputMonitoringAlertData.shared
  @ObservedObject var eventObserver = EventObserver.shared

  var body: some View {
    VStack {
      Text("right stick X: \(eventObserver.rightStickX)")
      Text("right stick Y: \(eventObserver.rightStickY)")
    }
    .alert(isPresented: inputMonitoringAlertData.showing) {
      InputMonitoringAlertView()
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
