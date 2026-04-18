import ServiceManagement
import SwiftUI

struct ServicesNotRunningAlertView: View {
  @ObservedObject private var contentViewStates = ContentViewStates.shared
  @FocusState var focus: Bool

  private let loginItemsImage: String

  init() {
    if #available(macOS 26.0, *) {
      loginItemsImage = "login-items-macos26"
    } else {
      loginItemsImage = "login-items-macos15"
    }
  }

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .center, spacing: 20.0) {
        Label(
          contentViewStates.alertContext.servicesEnabled
            ? "The background services are not running. Please wait a few seconds."
            : "The background services are not enabled. Please enable them.",
          systemImage: "exclamationmark.triangle"
        )
        .font(.system(size: 24))

        GroupBox {
          VStack(alignment: .center, spacing: 20.0) {
            if contentViewStates.alertContext.servicesEnabled {
              VStack(alignment: .center, spacing: 0.0) {
                Text(
                  "Waiting for the background services to start for \(contentViewStates.alertContext.servicesWaitingSeconds) seconds."
                )
                Text(
                  "If the services do not start within 20 seconds, please disable the services and then enable them again."
                )
              }
            } else {
              VStack(alignment: .center, spacing: 0.0) {
                Text("You need to permit the background services to use Karabiner-Elements.")
                Text(
                  "Please enable the following items from System Settings > General > Login Items & Extensions."
                )
              }
            }

            if contentViewStates.alertContext.servicesEnabled {
              ProgressView()
            }

            VStack(alignment: .leading, spacing: 0.0) {
              Label(
                "Karabiner-Elements Non-Privileged Agents v2",
                systemImage:
                  contentViewStates.alertContext.coreAgentsRunning
                  ? "checkmark.circle.fill" : "circle")
              Label(
                "Karabiner-Elements Privileged Daemons v2",
                systemImage:
                  contentViewStates.alertContext.coreDaemonsRunning
                  ? "checkmark.circle.fill" : "circle")
            }

            if !contentViewStates.alertContext.servicesEnabled
              || contentViewStates.alertContext.servicesWaitingSeconds > 15
            {
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

            Label(
              "If these services are already enabled, the settings might not have been correctly applied on the macOS side.\n"
                + "Try disabling them once and then enabling them again.",
              systemImage: InfoBorder.icon
            )
            .modifier(InfoBorder())

            if !contentViewStates.alertContext.servicesEnabled {
              Image(decorative: loginItemsImage)
                .resizable()
                .aspectRatio(contentMode: .fit)
                .frame(width: 600)
                .border(Color.gray, width: 1)
            }
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
