import SwiftUI

struct ActionView: View {
  @ObservedObject private var settings = LibKrbn.Settings.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 24.0) {
      GroupBox(label: Text("Action")) {
        VStack(alignment: .leading, spacing: 16) {
          HStack {
            Button(
              action: {
                libkrbn_services_restart_console_user_server_agent()
                Relauncher.relaunch()
              },
              label: {
                Label("Restart Karabiner-Elements", systemImage: "arrow.clockwise")
              })

            Spacer()
          }

          HStack {
            Button(
              action: {
                KarabinerAppHelper.shared.quitKarabiner(
                  askForConfirmation: settings.askForConfirmationBeforeQuitting,
                  quitFrom: .settings)
              },
              label: {
                Label("Quit Karabiner-Elements", systemImage: "xmark.circle.fill")
              })

            Spacer()
          }
        }
        .padding()
      }

      Spacer()
    }
    .padding()
  }
}

struct ActionView_Previews: PreviewProvider {
  static var previews: some View {
    ActionView()
  }
}
