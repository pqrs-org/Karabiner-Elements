import SwiftUI

class InputMonitoringAlertData: ObservableObject {
  public static let shared = InputMonitoringAlertData()

  @Published var showing = false
}

struct InputMonitoringAlertView: View {
  let window: NSWindow?

  var body: some View {
    VStack(alignment: .center, spacing: 20.0) {
      Label(
        "Please allow Karabiner-EventViewer to monitor input events",
        systemImage: "lightbulb"
      )
      .font(.system(size: 24))

      VStack(spacing: 0) {
        Text("Karabiner-EventViewer requires Input Monitoring permission to show input events.")
        Text("Please allow on Privacy & Security System Settings.")
      }

      Button(action: { openSystemPreferencesSecurity() }) {
        Label(
          "Open Privacy & Security System Settings...",
          systemImage: "arrow.forward.circle.fill")
      }

      Image(decorative: "input_monitoring")
        .resizable()
        .aspectRatio(contentMode: .fit)
        .frame(height: 300.0)
        .border(Color.gray, width: 1)
    }
    .frame(width: 800)
    .padding()
  }

  func openSystemPreferencesSecurity() {
    let url = URL(
      string: "x-apple.systempreferences:com.apple.preference.security?Privacy_ListenEvent")!
    NSWorkspace.shared.open(url)

    window?.orderBack(self)
  }
}

struct InputMonitoringAlertView_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      InputMonitoringAlertView(window: nil)
        .previewLayout(.sizeThatFits)
    }
  }
}
