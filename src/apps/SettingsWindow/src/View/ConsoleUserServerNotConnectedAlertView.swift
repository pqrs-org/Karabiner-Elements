import SwiftUI

struct ConsoleUserServerNotConnectedAlertView: View {
  @FocusState var focus: Bool

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .center) {
        Label(
          "Waiting to connect to the agent process (karabiner_console_user_server)",
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
