import SwiftUI

struct SetupInputMonitoringView: View {
  private let inputMonitoringImage: String

  init() {
    inputMonitoringImage = "input-monitoring-macos26"
  }

  var body: some View {
    VStack(alignment: .leading, spacing: 20.0) {
      Label(
        "Please allow to monitor input events",
        systemImage: "lightbulb"
      )
      .font(.system(size: 24))

      Text(
        """
        Karabiner-Core-Service requires Input Monitoring permission.
        Please allow them on Privacy & Security System Settings.
        """
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

      Image(decorative: inputMonitoringImage)
        .resizable()
        .aspectRatio(contentMode: .fit)
        .border(Color.gray, width: 1)
    }
  }
}
