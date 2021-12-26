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
