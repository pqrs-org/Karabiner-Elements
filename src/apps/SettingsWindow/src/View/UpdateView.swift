import SwiftUI

struct UpdateView: View {
  @ObservedObject private var settings = LibKrbn.Settings.shared
  let version =
    Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as? String ?? ""

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
            .switchToggleStyle()

            Spacer()
          }

          HStack {
            Button(
              action: {
                libkrbn_updater_check_for_updates_stable_only()
              },
              label: {
                Label("Check for updates", systemImage: "star")
              }
            )

            Spacer()

            Button(
              action: {
                libkrbn_updater_check_for_updates_with_beta_version()
              },
              label: {
                Label("Check for beta updates", systemImage: "star.circle")
              }
            )
          }
        }
        .padding()
      }

      GroupBox(label: Text("Web sites")) {
        VStack(alignment: .leading, spacing: 12.0) {
          HStack {
            Button(
              action: {
                NSWorkspace.shared.open(URL(string: "https://karabiner-elements.pqrs.org/")!)
              },
              label: {
                Label("Open official website", systemImage: "house")
              })
            Button(
              action: {
                NSWorkspace.shared.open(
                  URL(string: "https://github.com/pqrs-org/Karabiner-Elements")!)
              },
              label: {
                Label("Open GitHub (source code)", systemImage: "network")
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
