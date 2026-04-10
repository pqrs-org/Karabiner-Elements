import ServiceManagement
import SwiftUI

struct ConsoleUserServerNotConnectedAlertView: View {
  @ObservedObject private var contentViewStates = ContentViewStates.shared
  @FocusState var focus: Bool

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .center, spacing: 20.0) {
        Label(
          "Waiting to connect to the agent process (karabiner_console_user_server)",
          systemImage: "hourglass"
        )
        .font(.system(size: 24))
        .textSelection(.enabled)

        ProgressView()

        if contentViewStates.consoleUserServerClientWaitingSeconds >= 5 {
          GroupBox {
            VStack(alignment: .center, spacing: 20.0) {
              Text(
                "The connection could not be established. Open System Settings and turn Karabiner-Elements Non-Privileged Agents v2 off and on again."
              )

              Button(
                action: {
                  SMAppService.openSystemSettingsLoginItems()
                },
                label: {
                  Label(
                    "Open System Settings > General > Login Items & Extensions",
                    systemImage: "arrow.forward.circle.fill")
                }
              )
              .focused($focus)
            }
            .padding()
          }
        }
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
