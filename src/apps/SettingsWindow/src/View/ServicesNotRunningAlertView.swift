import ServiceManagement
import SwiftUI

struct ServicesNotRunningAlertView: View {
  @ObservedObject private var servicesMonitor = ServicesMonitor.shared
  @FocusState var focus: Bool

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .center, spacing: 20.0) {
        Label(
          servicesMonitor.servicesEnabled
            ? "The background services are not running. Please wait a few seconds."
            : "The background services are not enabled. Please enable them.",
          systemImage: "exclamationmark.triangle"
        )
        .font(.system(size: 24))

        GroupBox {
          VStack(alignment: .center, spacing: 20.0) {
            if servicesMonitor.servicesEnabled {
              VStack(alignment: .center, spacing: 0.0) {
                Text(
                  "Waiting for the background services to start for \(servicesMonitor.servicesWaitingSeconds) seconds."
                )
                Text(
                  "If the services do not start within 20 seconds, please disable the services and then enable them again."
                )
              }
            } else {
              VStack(alignment: .center, spacing: 0.0) {
                Text("You need to permit the background services to use Karabiner-Elements.")
                Text(
                  "Please enable the following items from System Settings > General > Login Items.")
              }
            }

            if servicesMonitor.servicesEnabled {
              ProgressView()
            }

            VStack(alignment: .leading, spacing: 0.0) {
              Label(
                "Karabiner-Elements Non-Privileged Agents",
                systemImage:
                  servicesMonitor.coreAgentsRunning ? "checkmark.circle.fill" : "circle")
              Label(
                "Karabiner-Elements Privileged Daemons",
                systemImage:
                  servicesMonitor.coreDaemonsRunning ? "checkmark.circle.fill" : "circle")
            }

            if !servicesMonitor.servicesEnabled || servicesMonitor.servicesWaitingSeconds > 15 {
              Button(
                action: {
                  SMAppService.openSystemSettingsLoginItems()
                },
                label: {
                  Label(
                    "Open System Settings > General > Login Items",
                    systemImage: "arrow.forward.circle.fill")
                }
              )
              .focused($focus)
            }

            Label(
              "If these are already enabled, the settings might not be properly reflected on the macOS side. Please disable them once and then enable them again.",
              systemImage: "lightbulb"
            )
            .padding()
            .foregroundColor(Color.warningForeground)
            .background(Color.warningBackground)

            if !servicesMonitor.servicesEnabled {
              Image(decorative: "login-items")
                .resizable()
                .aspectRatio(contentMode: .fit)
                .border(Color.gray, width: 1)
            }

          }
          .padding()
        }
      }
      .padding()
      .frame(width: 850)

      SheetCloseButton {
        ContentViewStates.shared.showServicesNotRunningAlert = false
      }
    }
    .onAppear {
      focus = true
    }
  }
}

struct ServicesNotRunningAlertView_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      ServicesNotRunningAlertView()
        .previewLayout(.sizeThatFits)
    }
  }
}
