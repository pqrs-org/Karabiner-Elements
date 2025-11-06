import SwiftUI

struct InputMonitoringPermissionsAlertView: View {
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
      VStack(spacing: 20.0) {
        Label(
          "Please allow Karabiner apps to monitor input events",
          systemImage: "lightbulb"
        )
        .font(.system(size: 24))

        Text(
          "Karabiner-Core-Daemon requires Input Monitoring permission.\nPlease allow them on Privacy & Security System Settings."
        )
        .fixedSize(horizontal: false, vertical: true)

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

  private func openSystemSettingsSecurity() {
    let url = URL(
      string: "x-apple.systempreferences:com.apple.preference.security?Privacy_ListenEvent")!
    NSWorkspace.shared.open(url)
  }
}
