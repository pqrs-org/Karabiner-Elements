import SwiftUI

struct InputMonitoringPermissionsAlertView: View {
  @FocusState var focus: Bool

  private let inputMonitoringImage: String

  init() {
    inputMonitoringImage = "input-monitoring-macos26"
  }

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(spacing: 20.0) {
        Label(
          "Please allow Karabiner apps to monitor input events",
          systemImage: "lightbulb"
        )
        .font(.system(size: 24))

        Text(
          "Karabiner-Core-Service requires Input Monitoring permission.\nPlease allow them on Privacy & Security System Settings."
        )
        .fixedSize(horizontal: false, vertical: true)

        OpenSystemSettingsButton(
          url: "x-apple.systempreferences:com.apple.preference.security?Privacy_ListenEvent",
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
          .border(Color.gray, width: 1)
          .frame(height: 300)
      }
      .padding()
      .frame(width: 700)

      SheetCloseButton {
        ContentViewStates.shared.showInputMonitoringPermissionsAlert = false
      }
    }
    .onAppear {
      focus = true
    }
  }
}
