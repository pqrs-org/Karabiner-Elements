import SwiftUI

struct DoctorAlertView: View {
  @ObservedObject private var contentViewStates = ContentViewStates.shared

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .center, spacing: 20.0) {
        if !contentViewStates.coreServiceDaemonState.karabinerJsonParseErrorMessage.isEmpty {
          AppLocalizedLabel(
            "settings.configuration.parse_error",
            systemImage: ErrorBorder.icon
          )
          .font(.title)

          AppLocalizedText("settings.configuration.parse_error_hint")

          Text(contentViewStates.coreServiceDaemonState.karabinerJsonParseErrorMessage)
            .modifier(ErrorBorder())
        }
      }
      .padding()
      .frame(width: 850)

      SheetCloseButton {
        ContentViewStates.shared.dismissCurrentAlert()
      }
    }
  }
}
