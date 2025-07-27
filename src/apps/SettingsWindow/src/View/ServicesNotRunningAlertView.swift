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
                  "Please enable the following items from System Settings > General > Login Items & Extensions."
                )
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

                  NSApp.miniaturizeAll(nil)
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
              systemImage: "lightbulb"
            )
            .modifier(WarningBorder())

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
