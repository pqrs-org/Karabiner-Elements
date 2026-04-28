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
        "Please enable background services",
        systemImage: "lightbulb"
      )
      .font(.system(size: 24))

      GroupBox {
        VStack(alignment: .leading, spacing: 20.0) {
          VStack(alignment: .leading, spacing: 0.0) {
            Text("You need to permit the background services to use Karabiner-Elements.")
            Text(
              "Please enable the following items from System Settings > General > Login Items & Extensions."
            )
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

          Image(decorative: loginItemsImage)
            .resizable()
            .aspectRatio(contentMode: .fit)
            .border(Color.gray, width: 1)

          VStack(alignment: .leading, spacing: 0.0) {
            Label(
              "Karabiner-Elements Non-Privileged Agents v2",
              systemImage:
                contentViewStates.guidanceContext.coreAgentsEnabled != false
                ? "checkmark.circle.fill" : "circle")
            Label(
              "Karabiner-Elements Privileged Daemons v2",
              systemImage:
                contentViewStates.guidanceContext.coreDaemonsEnabled != false
                ? "checkmark.circle.fill" : "circle")
          }
        }
        .padding()
      }
    }
  }
}
