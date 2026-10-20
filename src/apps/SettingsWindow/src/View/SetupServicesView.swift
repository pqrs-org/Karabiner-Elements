import ServiceManagement
import SwiftUI

enum SetupServicesPresentation {
  // Case names indicate the first supported macOS version that uses each presentation.
  // The image asset may have been captured on a later version with the same layout.
  case macOS13
  case macOS26

  static var current: Self {
    if #available(macOS 26.0, *) {
      .macOS26
    } else {
      .macOS13
    }
  }

  var imageName: String {
    switch self {
    case .macOS13:
      "login-items-macos15"
    case .macOS26:
      "login-items-macos27"
    }
  }
}

struct SetupServicesView: View {
  @ObservedObject private var contentViewStates = ContentViewStates.shared

  let debugGuidanceContextOverride: LocalServicesGuidanceContext?

  private var localServicesGuidanceContext: LocalServicesGuidanceContext {
    debugGuidanceContextOverride ?? contentViewStates.localServicesGuidanceContext
  }

  private let loginItemsImage: String

  init(
    debugGuidanceContextOverride: LocalServicesGuidanceContext? = nil,
    presentation: SetupServicesPresentation = .current
  ) {
    self.debugGuidanceContextOverride = debugGuidanceContextOverride
    loginItemsImage = presentation.imageName
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
              "Karabiner-Elements Non-Privileged Agents v3",
              systemImage:
                localServicesGuidanceContext.coreAgentsEnabled == true
                ? "checkmark.circle.fill" : "circle")
            Label(
              "Karabiner-Elements Privileged Daemons v3",
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
