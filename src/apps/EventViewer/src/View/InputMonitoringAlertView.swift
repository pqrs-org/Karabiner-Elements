import SwiftUI

@MainActor
class InputMonitoringAlertData: ObservableObject {
  public static let shared = InputMonitoringAlertData()

  @Published var showing = false
}

struct InputMonitoringAlertView: View {
  @FocusState var focus: Bool

  private let inputMonitoringImage: String

  init() {
    if #available(macOS 26.0, *) {
      inputMonitoringImage = "input-monitoring-macos26"
    } else {
      inputMonitoringImage = "input-monitoring-macos15"
    }
  }

  var body: some View {
    ZStack(alignment: .topLeading) {
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

        Button(
          action: { openSystemSettingsSecurity() },
          label: {
            Label(
              "Open Privacy & Security System Settings...",
              systemImage: "arrow.forward.circle.fill")
          }
        )
        .focused($focus)

        Image(decorative: inputMonitoringImage)
          .resizable()
          .aspectRatio(contentMode: .fit)
          .frame(height: 200.0)
          .border(Color.gray, width: 1)
      }
      .padding()
      .frame(width: 800)

      SheetCloseButton {
        InputMonitoringAlertData.shared.showing = false
      }
    }
    .onAppear {
      focus = true
    }
  }

  func openSystemSettingsSecurity() {
    let url = URL(
      string: "x-apple.systempreferences:com.apple.preference.security?Privacy_ListenEvent")!
    NSWorkspace.shared.open(url)
  }
}
