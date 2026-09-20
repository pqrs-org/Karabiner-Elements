import ServiceManagement
import SwiftUI

struct ConsoleUserServerNotConnectedAlertView: View {
  @ObservedObject private var contentViewStates = ContentViewStates.shared
  @FocusState var focus: Bool
  let debugDisconnectedForAWhileOverride: Bool?

  init(debugDisconnectedForAWhileOverride: Bool? = nil) {
    self.debugDisconnectedForAWhileOverride = debugDisconnectedForAWhileOverride
  }

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .center, spacing: 20.0) {
        AppLocalizedLabel(
          "settings.setup.connection.agent_waiting",
          systemImage: "hourglass"
        )
        .font(.system(size: 24))
        .textSelection(.enabled)

        ProgressView()

        if debugDisconnectedForAWhileOverride
          ?? contentViewStates.consoleUserServerClientDisconnectedForAWhile
        {
          GroupBox {
            VStack(alignment: .center, spacing: 20.0) {
              AppLocalizedText(
                "settings.setup.connection.agent_retry"
              )

              Button(
                action: {
                  SMAppService.openSystemSettingsLoginItems()
                },
                label: {
                  AppLocalizedConstrainedLabel(
                    "settings.setup.system_settings.open_login_items",
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
