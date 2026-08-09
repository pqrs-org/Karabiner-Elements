import SwiftUI

struct ActionView: View {
  var body: some View {
    ScrollView {
      VStack(alignment: .leading, spacing: 24.0) {
        GroupBox(label: Text("Action")) {
          VStack(alignment: .leading, spacing: 16) {
            Button(
              action: {
                krbn_services_restart_console_user_server_agent()
                Relauncher.relaunch()
              },
              label: {
                Label("Restart Karabiner-Elements", systemImage: "arrow.clockwise")
              })

            Button(
              action: {
                krbn_services_unregister_all_agents()
                krbn_killall_settings()
              },
              label: {
                Label("Quit Karabiner-Elements", systemImage: "xmark.rectangle")
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
