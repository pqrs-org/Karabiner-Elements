import SwiftUI

struct ActionView: View {
  @ObservedObject private var settings = LibKrbn.Settings.shared
  private let version = Bundle.main.object(forInfoDictionaryKey: "CFBundleVersion") as! String

  var body: some View {
    VStack(alignment: .leading, spacing: 24.0) {
      GroupBox(label: Text("Action")) {
        VStack(alignment: .leading, spacing: 16) {
          HStack {
            Button(action: {
              libkrbn_launchctl_restart_console_user_server()
              KarabinerKit.relaunch()
            }) {
              Label("Restart Karabiner-Elements", systemImage: "arrow.clockwise")
            }

            Spacer()
          }

          HStack {
            Button(action: {
              KarabinerKit.quitKarabiner(settings.askForConfirmationBeforeQuitting)
            }) {
              Label("Quit Karabiner-Elements", systemImage: "xmark.circle.fill")
            }

            Spacer()
          }
        }
        .padding(6.0)
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
