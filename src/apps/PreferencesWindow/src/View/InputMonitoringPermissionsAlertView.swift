import SwiftUI

struct InputMonitoringPermissionsAlertView: View {
    public static let title = "Input Monitoring Permissions Alert"

    var body: some View {
        VStack(alignment: .leading, spacing: 20.0) {
            HStack {
                Image(decorative: "baseline_warning_black_48pt")
                    .resizable()
                    .frame(width: 48.0, height: 48.0)
                Text(InputMonitoringPermissionsAlertView.title)
            }

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
                .frame(height: 200)
        }
        .padding()
        .frame(width: 550)
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
