import SwiftUI

struct DoctorAlertView: View {
  @ObservedObject private var contentViewStates = ContentViewStates.shared

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .center, spacing: 20.0) {
        if !contentViewStates.coreServiceDaemonState.karabinerJsonParseErrorMessage.isEmpty {
          Label(
            "karabiner.json couldn't be loaded due to a parse error",
            systemImage: ErrorBorder.icon
          )
          .font(.title)

          Text("It looks like the file was edited manually and now contains invalid JSON.")

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
