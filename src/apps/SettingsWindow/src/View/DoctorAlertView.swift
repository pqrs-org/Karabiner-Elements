import SwiftUI

struct DoctorAlertView: View {
  @ObservedObject private var contentViewStates = ContentViewStates.shared

  let debugParseErrorMessageOverride: String?

  init(debugParseErrorMessageOverride: String? = nil) {
    self.debugParseErrorMessageOverride = debugParseErrorMessageOverride
  }

  private var parseErrorMessage: String {
    debugParseErrorMessageOverride
      ?? contentViewStates.coreServiceDaemonState.karabinerJsonParseErrorMessage
  }

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .center, spacing: 20.0) {
        if !parseErrorMessage.isEmpty {
          AppLocalizedLabel(
            "settings.configuration.parse_error",
            systemImage: ErrorBorder.icon
          )
          .font(.title)

          AppLocalizedText("settings.configuration.parse_error_hint")

          Text(parseErrorMessage)
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
