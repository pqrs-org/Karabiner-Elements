import SwiftUI

struct VirtualHidDeviceServiceClientNotConnectedAlertView: View {
  @FocusState var focus: Bool

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .center) {
        AppLocalizedLabel(
          "settings.setup.connection.virtual_hid_waiting",
          systemImage: "hourglass"
        )
        .font(.system(size: 24))
        .textSelection(.enabled)

        ProgressView()

        AppLocalizedLabel(
          "settings.setup.connection.virtual_hid_hint",
          systemImage: "lightbulb"
        )
        .textSelection(.enabled)
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
