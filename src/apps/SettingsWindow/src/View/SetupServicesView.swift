import ServiceManagement
import SwiftUI

struct SetupServicesView: View {
  @ObservedObject private var contentViewStates = ContentViewStates.shared

  var guidanceContextOverride: SettingsWindowGuidanceContext?

  private var guidanceContext: SettingsWindowGuidanceContext {
    guidanceContextOverride ?? contentViewStates.guidanceContext
  }

  private let loginItemsImage: String

  init(guidanceContextOverride: SettingsWindowGuidanceContext? = nil) {
    self.guidanceContextOverride = guidanceContextOverride
    if #available(macOS 26.0, *) {
      loginItemsImage = "login-items-macos26"
    } else {
      loginItemsImage = "login-items-macos15"
    }
  }

  var body: some View {
    VStack(alignment: .leading, spacing: 20.0) {
      AppLocalizedLabel(
        "settings.setup.services.permission",
        systemImage: "lightbulb"
      )
      .font(.system(size: 24))

      GroupBox {
        VStack(alignment: .leading, spacing: 20.0) {
          VStack(alignment: .leading, spacing: 0.0) {
            AppLocalizedText("settings.setup.services.description")
            AppLocalizedText(
              "settings.setup.services.enable_hint"
            )
          }

          Button(
            action: {
              SMAppService.openSystemSettingsLoginItems()
            },
            label: {
              AppLocalizedLabel(
                "settings.system_settings.open_login_items",
                systemImage: "arrow.forward.circle.fill")
            }
          )

          Image(decorative: loginItemsImage)
            .resizable()
            .scaledToFit()
            .border(Color.gray, width: 1)

          VStack(alignment: .leading, spacing: 0.0) {
            Label(
              "Karabiner-Elements Non-Privileged Agents v2",
              systemImage:
                guidanceContext.coreAgentsEnabled != false
                ? "checkmark.circle.fill" : "circle")
            Label(
              "Karabiner-Elements Privileged Daemons v2",
              systemImage:
                guidanceContext.coreDaemonsEnabled != false
                ? "checkmark.circle.fill" : "circle")
          }
        }
        .padding()
      }
    }
  }
}
