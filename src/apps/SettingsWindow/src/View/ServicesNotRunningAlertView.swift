import ServiceManagement
import SwiftUI

struct ServicesNotRunningAlertView: View {
  @ObservedObject private var contentViewStates = ContentViewStates.shared
  @FocusState var focus: Bool
  let debugGuidanceContextOverride: SettingsWindowGuidanceContext?

  init(debugGuidanceContextOverride: SettingsWindowGuidanceContext? = nil) {
    self.debugGuidanceContextOverride = debugGuidanceContextOverride
  }

  private var guidanceContext: SettingsWindowGuidanceContext {
    debugGuidanceContextOverride ?? contentViewStates.guidanceContext
  }

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .center, spacing: 20.0) {
        AppLocalizedLabel(
          "settings.setup.services.not_running",
          systemImage: "hourglass"
        )
        .font(.system(size: 24))

        VStack(alignment: .leading, spacing: 0.0) {
          AppLocalizedText(
            "settings.setup.services.restart_hint"
          )
        }

        ProgressView()

        GroupBox {
          VStack(alignment: .leading, spacing: 20.0) {
            VStack(alignment: .leading, spacing: 0.0) {
              Label(
                "Karabiner-Elements Non-Privileged Agents v2",
                systemImage:
                  guidanceContext.coreAgentsRunning != false
                  ? "checkmark.circle.fill" : "circle")
              Label(
                "Karabiner-Elements Privileged Daemons v2",
                systemImage:
                  guidanceContext.coreDaemonsRunning != false
                  ? "checkmark.circle.fill" : "circle")
            }

            Button(
              action: {
                SMAppService.openSystemSettingsLoginItems()
              },
              label: {
                AppLocalizedLabel(
                  "settings.setup.system_settings.open_login_items",
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
