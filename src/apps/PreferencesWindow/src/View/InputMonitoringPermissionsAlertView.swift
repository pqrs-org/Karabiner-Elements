import SwiftUI

struct InputMonitoringPermissionsAlertView: View {
    var body: some View {
        VStack(spacing: 20.0) {
            Label("Please allow Karabiner apps to monitor input events",
                  systemImage: "lightbulb")
                .font(.system(size: 24))

            Text("karabiner_grabber and karabiner_observer require Input Monitoring permission.\nPlease allow them on Security & Privacy System Preferences.")
                .fixedSize(horizontal: false, vertical: true)

            Button(action: { openSystemPreferencesSecurity() }) {
                Image(decorative: "ic_forward_18pt")
                    .resizable()
                    .frame(width: GUISize.buttonIconWidth, height: GUISize.buttonIconHeight)
                Text("Open Security & Privacy System Preferences")
            }

            Image(decorative: "input-monitoring")
                .resizable()
                .aspectRatio(contentMode: .fit)
                .border(Color.gray, width: 1)
                .frame(height: 300)
        }
        .padding()
        .frame(width: 700)
    }

    private func openSystemPreferencesSecurity() {
        NSApplication.shared.miniaturizeAll(self)

        let url = URL(string: "x-apple.systempreferences:com.apple.preference.security?Privacy_ListenEvent")!
        NSWorkspace.shared.open(url)
    }
}

struct InputMonitoringPermissionsAlertView_Previews: PreviewProvider {
    static var previews: some View {
        Group {
            InputMonitoringPermissionsAlertView()
                .previewLayout(.sizeThatFits)
        }
    }
}
