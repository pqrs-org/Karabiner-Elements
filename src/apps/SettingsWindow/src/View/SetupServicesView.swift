import ServiceManagement
import SwiftUI

struct SetupServicesView: View {
  @ObservedObject private var contentViewStates = ContentViewStates.shared

  let debugGuidanceContextOverride: LocalServicesGuidanceContext?

  private var localServicesGuidanceContext: LocalServicesGuidanceContext {
    debugGuidanceContextOverride ?? contentViewStates.localServicesGuidanceContext
  }

  private let loginItemsImage: String

  init(
    debugGuidanceContextOverride: LocalServicesGuidanceContext? = nil,
    debugLoginItemsImageOverride: String? = nil
  ) {
    self.debugGuidanceContextOverride = debugGuidanceContextOverride
    if let debugLoginItemsImageOverride {
      loginItemsImage = debugLoginItemsImageOverride
    } else if #available(macOS 26.0, *) {
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
              AppLocalizedConstrainedLabel(
                "settings.setup.system_settings.open_login_items",
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
                localServicesGuidanceContext.coreAgentsEnabled == true
                ? "checkmark.circle.fill" : "circle")
            Label(
              "Karabiner-Elements Privileged Daemons v2",
              systemImage:
                localServicesGuidanceContext.coreDaemonsEnabled == true
                ? "checkmark.circle.fill" : "circle")
          }
        }
        .padding()
      }
    }
  }
}
