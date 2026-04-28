import ServiceManagement
import SwiftUI

struct ServicesNotRunningAlertView: View {
  @ObservedObject private var contentViewStates = ContentViewStates.shared
  @FocusState var focus: Bool

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .center, spacing: 20.0) {
        Label(
          "Some background services are not running",
          systemImage: "hourglass"
        )
        .font(.system(size: 24))

        VStack(alignment: .leading, spacing: 0.0) {
          Text(
            """
            Although the background services are enabled, some of them are not running.
            Try disabling the background services once, then enable them again.
            """
          )
        }

        ProgressView()

        GroupBox {
          VStack(alignment: .leading, spacing: 20.0) {
            VStack(alignment: .leading, spacing: 0.0) {
              Label(
                "Karabiner-Elements Non-Privileged Agents v2",
                systemImage:
                  contentViewStates.guidanceContext.coreAgentsRunning != false
                  ? "checkmark.circle.fill" : "circle")
              Label(
                "Karabiner-Elements Privileged Daemons v2",
                systemImage:
                  contentViewStates.guidanceContext.coreDaemonsRunning != false
                  ? "checkmark.circle.fill" : "circle")
            }

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
