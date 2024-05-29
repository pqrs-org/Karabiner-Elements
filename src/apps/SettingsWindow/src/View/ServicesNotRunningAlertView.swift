import ServiceManagement
import SwiftUI

struct ServicesNotRunningAlertView: View {
  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .center, spacing: 20.0) {
        Label(
          "The background service is not running. Please enable it.",
          systemImage: "exclamationmark.triangle"
        )
        .font(.system(size: 24))

        GroupBox {
          VStack(alignment: .center, spacing: 12.0) {
            VStack(alignment: .center, spacing: 0.0) {
              Text("To use Karabiner-Elements, you need to run the background service.")
              Text(
                "Please enable Karabiner-Elements-Services from System Settings > General > Login Items."
              )
            }

            Button(
              action: {
                SMAppService.openSystemSettingsLoginItems()
              },
              label: {
                Label(
                  "Open System Settings > General > Login Items",
                  systemImage: "arrow.forward.circle.fill")
              })

            Image(decorative: "login-items")
              .resizable()
              .aspectRatio(contentMode: .fit)
              .frame(height: 300)
              .border(Color.gray, width: 1)

            VStack {
              Label(
                "If it is already enabled, the settings might not be properly reflected on the macOS side. Please disable Karabiner-Elements-Services once and then enable it again.",
                systemImage: "lightbulb"
              )
              .padding()
              .foregroundColor(Color.warningForeground)
              .background(Color.warningBackground)
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
