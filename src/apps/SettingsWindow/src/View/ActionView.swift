import SwiftUI

struct ActionView: View {
  @ObservedObject private var settings = LibKrbn.Settings.shared

  var body: some View {
    ScrollView {
      VStack(alignment: .leading, spacing: 24.0) {
        GroupBox(label: Text("Action")) {
          VStack(alignment: .leading, spacing: 16) {
            Button(
              action: {
                libkrbn_services_restart_console_user_server_agent()
                Relauncher.relaunch()
              },
              label: {
                Label("Restart Karabiner-Elements", systemImage: "arrow.clockwise")
              })

            Button(
              action: {
                KarabinerAppHelper.shared.quitKarabiner(
                  askForConfirmation: settings.askForConfirmationBeforeQuitting,
                  quitFrom: .settings)
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
    .padding(.leading, 2)  // Prevent the header underline from disappearing in NavigationSplitView.
  }
}
