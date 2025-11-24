import SwiftUI

@MainActor
class InputMonitoringAlertData: ObservableObject {
  public static let shared = InputMonitoringAlertData()

  @Published var showing = false
}

struct InputMonitoringAlertView: View {
  @FocusState var focus: Bool

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .center, spacing: 20.0) {
        Label(
          "Please allow Karabiner-GamePadViewer to monitor input events",
          systemImage: "lightbulb"
        )
        .font(.system(size: 24))

        VStack(spacing: 0) {
          Text("Karabiner-GamePadViewer requires Input Monitoring permission to show input events.")
          Text("Please allow on Privacy & Security System Settings.")
        }

        OpenSystemSettingsButton(
          url: "x-apple.systempreferences:com.apple.preference.security?Privacy_ListenEvent",
          label: {
            Label(
              "Open Privacy & Security System Settings...",
              systemImage: "arrow.forward.circle.fill")
          }
        )
        .focused($focus)
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
}
