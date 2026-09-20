import SwiftUI

struct SetupInputMonitoringView: View {
  private let inputMonitoringImage: String

  init() {
    inputMonitoringImage = "input-monitoring-macos26"
  }

  var body: some View {
    VStack(alignment: .leading, spacing: 20.0) {
      AppLocalizedLabel(
        "settings.setup.input_monitoring.permission",
        systemImage: "lightbulb"
      )
      .font(.system(size: 24))

      AppLocalizedText(
        "settings.setup.input_monitoring.description"
      )

      OpenSystemSettingsButton(
        url: "x-apple.systempreferences:com.apple.preference.security?Privacy_ListenEvent",
        label: {
          AppLocalizedConstrainedLabel(
            "settings.setup.system_settings.open_privacy",
            systemImage: "arrow.forward.circle.fill")
        }
      )

      Image(decorative: inputMonitoringImage)
        .resizable()
        .scaledToFit()
        .border(Color.gray, width: 1)
    }
  }
}
