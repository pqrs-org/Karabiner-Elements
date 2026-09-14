import SwiftUI

struct DriverNotConnectedAlertView: View {
  @FocusState var focus: Bool

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .center) {
        AppLocalizedLabel(
          "settings.connection.iokit_waiting",
          systemImage: "hourglass"
        )
        .font(.system(size: 24))
        .textSelection(.enabled)

        ProgressView()
      }
      .padding()
      .frame(width: 850)

      SheetCloseButton {
        ContentViewStates.shared.dismissCurrentAlert()
      }
    }
    .onAppear {
      focus = true
    }
  }
}
