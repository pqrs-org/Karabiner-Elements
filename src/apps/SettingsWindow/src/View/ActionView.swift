import SwiftUI

struct ActionView: View {
  var body: some View {
    ScrollView {
      VStack(alignment: .leading, spacing: 24.0) {
        GroupBox(label: AppLocalizedText("settings.action.title")) {
          VStack(alignment: .leading, spacing: 16) {
            Button(
              action: {
                krbn_services_restart_console_user_server_agent()
                Relauncher.relaunch()
              },
              label: {
                AppLocalizedLabel("menu_bar_extra.restart", systemImage: "arrow.clockwise")
              })

            Button(
              action: {
                krbn_services_unregister_all_agents()
                krbn_killall_settings()
              },
              label: {
                AppLocalizedLabel("menu_bar_extra.quit", systemImage: "xmark.rectangle")
              })
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }
      }
      .padding()
    }
  }
}
