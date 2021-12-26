import SwiftUI

struct MiscView: View {
    @ObservedObject var settings = Settings.shared

    var body: some View {
        VStack(alignment: .leading, spacing: 12.0) {
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

            GroupBox(label: Text("Extra tool")) {
                VStack(alignment: .leading, spacing: 12.0) {
                    HStack {
                        Button(action: {
                            libkrbn_launch_multitouch_extension()
                        }) {
                            Label("Open Karabiner-MultitouchExtension app", systemImage: "arrow.up.forward.app")
                        }

                        Spacer()
                    }
                }
                .padding(6.0)
            }

            GroupBox(label: Text("Web sites")) {
                VStack(alignment: .leading, spacing: 12.0) {
                    HStack {
                        Button(action: { NSWorkspace.shared.open(URL(string: "https://karabiner-elements.pqrs.org/")!) }) {
                            Label("Open official website", systemImage: "house")
                        }
                        Button(action: { NSWorkspace.shared.open(URL(string: "https://github.com/pqrs-org/Karabiner-Elements")!) }) {
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

struct MiscView_Previews: PreviewProvider {
    static var previews: some View {
        MiscView()
    }
}
