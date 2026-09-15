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
    inputMonitoringImage = "input-monitoring-macos26"
  }

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .center, spacing: 20.0) {
        AppLocalizedLabel(
          "event_viewer.input_monitoring.title",
          systemImage: "lightbulb"
        )
        .font(.system(size: 24))

        VStack(spacing: 0) {
          AppLocalizedText("event_viewer.input_monitoring.description")
          AppLocalizedText("event_viewer.input_monitoring.hint")
        }

        OpenSystemSettingsButton(
          url: "x-apple.systempreferences:com.apple.preference.security?Privacy_ListenEvent",
          label: {
            AppLocalizedLabel(
              "event_viewer.input_monitoring.open_settings",
              systemImage: "arrow.forward.circle.fill")
          }
        )
        .focused($focus)

        Image(decorative: inputMonitoringImage)
          .resizable()
          .scaledToFit()
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
}
