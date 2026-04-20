import ServiceManagement
import SwiftUI

struct SetupServicesView: View {
  @ObservedObject private var contentViewStates = ContentViewStates.shared

  private let loginItemsImage: String

  init() {
    if #available(macOS 26.0, *) {
      loginItemsImage = "login-items-macos26"
    } else {
      loginItemsImage = "login-items-macos15"
    }
  }

  var body: some View {
    VStack(alignment: .leading, spacing: 20.0) {
      Label(
        contentViewStates.alertContext.servicesEnabled
          ? "Waiting for the background services"
          : "Please enable background services",
        systemImage: "lightbulb"
      )
      .font(.system(size: 24))

      GroupBox {
        VStack(alignment: .leading, spacing: 20.0) {
          if contentViewStates.alertContext.servicesEnabled {
            VStack(alignment: .leading, spacing: 0.0) {
              Text(
                "Waiting for the background services to start for \(contentViewStates.alertContext.servicesWaitingSeconds) seconds."
              )
              Text(
                "If the services do not start within 20 seconds, please disable the services and then enable them again."
              )
            }

            ProgressView()
          } else {
            VStack(alignment: .leading, spacing: 0.0) {
              Text("You need to permit the background services to use Karabiner-Elements.")
              Text(
                "Please enable the following items from System Settings > General > Login Items & Extensions."
              )
            }
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
              .border(Color.gray, width: 1)
          }
        }
        .padding()
      }
    }
  }
}
