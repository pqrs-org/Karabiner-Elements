import SwiftUI

struct MiscView: View {
    @ObservedObject var settings = Settings.shared
    let version = Bundle.main.object(forInfoDictionaryKey: "CFBundleVersion") as! String

    var body: some View {
        VStack(alignment: .leading, spacing: 12.0) {
            GroupBox(label: Text("Version")) {
                VStack(alignment: .leading, spacing: 12.0) {
                    HStack {
                        Text("Karabiner-Elements version \(version)")
                        Spacer()
                    }

                    Button(action: {
                        libkrbn_launchctl_restart_console_user_server()
                        KarabinerKit.relaunch()
                    }) {
                        Label("Restart Karabiner-Elements", systemImage: "arrow.clockwise")
                    }
                }
                .padding(6.0)
            }

            GroupBox(label: Text("Update")) {
                VStack(alignment: .leading, spacing: 12.0) {
                    Toggle(isOn: $settings.checkForUpdatesOnStartup) {
                        Text("Check for updates on startup (Default: on)")
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

            GroupBox(label: Text("Menu bar")) {
                VStack(alignment: .leading, spacing: 12.0) {
                    Toggle(isOn: $settings.showIconInMenuBar) {
                        Text("Show icon in menu bar (Default: on)")
                        Spacer()
                    }

                    Toggle(isOn: $settings.showProfileNameInMenuBar) {
                        Text("Show profile name in menu bar (Default: off)")
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

struct MiscView_Previews: PreviewProvider {
    static var previews: some View {
        MiscView()
    }
}
