import SwiftUI

struct PreferencesView: View {
    @ObservedObject var userSettings = UserSettings.shared
    
    var body: some View {
        VStack(alignment: .leading, spacing: 12.0) {
            GroupBox(label: Text("Window behavior")) {
                VStack(alignment: .leading, spacing: 12.0) {
                    Toggle(isOn: $userSettings.forceStayTop) {
                        Text("Force EventViewer to stay on top of other windows (Default: off)")
                        Spacer()
                    }
                    
                    Toggle(isOn: $userSettings.showInAllSpaces) {
                        Text("Show EventViewer in all spaces (Default: off)")
                        Spacer()
                    }
                }
                .padding(6.0)
            }
            
            GroupBox(label: Text("Others")) {
                VStack(alignment: .leading, spacing: 12.0) {
                    Toggle(isOn: $userSettings.showHex) {
                        Text("Show HID usage in hexadecimal format (Default: off)")
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

struct PreferencesView_Previews: PreviewProvider {
    static var previews: some View {
        PreferencesView()
    }
}
