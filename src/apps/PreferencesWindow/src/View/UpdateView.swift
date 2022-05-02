import SwiftUI

struct UpdateView: View {
  @ObservedObject var settings = Settings.shared
  let version = Bundle.main.object(forInfoDictionaryKey: "CFBundleVersion") as! String

  var body: some View {
    VStack(alignment: .leading, spacing: 24.0) {
      GroupBox(label: Text("Update")) {
        VStack(alignment: .leading, spacing: 12.0) {
          HStack {
            Text("Karabiner-Elements version \(version)")
            Spacer()
          }

          HStack {
            Toggle(isOn: $settings.checkForUpdatesOnStartup) {
              Text("Check for updates on startup (Default: on)")
            }
            Spacer()
          }

          HStack {
            Button(action: {
              Updater.shared.checkForUpdatesStableOnly()
            }) {
              Label("Check for updates", systemImage: "star")
            }

            Spacer()

            Button(action: {
              Updater.shared.checkForUpdatesWithBetaVersion()
            }) {
              Label("Check for beta updates", systemImage: "star.circle")
            }
          }
        }
        .padding(6.0)
      }

      GroupBox(label: Text("Web sites")) {
        VStack(alignment: .leading, spacing: 12.0) {
          HStack {
            Button(action: {
              NSWorkspace.shared.open(URL(string: "https://karabiner-elements.pqrs.org/")!)
            }) {
              Label("Open official website", systemImage: "house")
            }
            Button(action: {
              NSWorkspace.shared.open(
                URL(string: "https://github.com/pqrs-org/Karabiner-Elements")!)
            }) {
              Label("Open GitHub (source code)", systemImage: "network")
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

struct UpdateView_Previews: PreviewProvider {
  static var previews: some View {
    UpdateView()
  }
}
