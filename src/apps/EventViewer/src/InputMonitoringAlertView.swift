import SwiftUI

struct InputMonitoringAlertView: View {
    var body: some View {
        VStack(alignment: .center) {
            HStack {
                Image(decorative: "baseline_warning_black_48pt")
                    .resizable()
                    .frame(width: 48.0, height: 48.0)
                Text("EventViewer Alert")
            }

            Text("EventViewer requires Input Monitoring permission.\nYou have to allow Karabiner-EventViewer on Security & Privacy System Preferences.")
                .multilineTextAlignment(.center)
                .fixedSize()

            Spacer(minLength: 20.0)

            Button(action: { openSystemPreferencesSecurity() }) {
                Image(decorative: "ic_forward_18pt")
                Text("Open Security & Privacy System Preferences")
            }

            Spacer(minLength: 40.0)

            Image(decorative: "input_monitoring")
                .resizable()
                .aspectRatio(contentMode: .fill)
                .frame(width: 250.0, height: 210.0)

            Spacer()
        }
        .padding()
    }

    func openSystemPreferencesSecurity() {
        NSApplication.shared.miniaturizeAll(self)

        let url = URL(string: "x-apple.systempreferences:com.apple.preference.security?Privacy")!
        NSWorkspace.shared.open(url)
    }
}

struct InputMonitoringAlertView_Previews: PreviewProvider {
    static var previews: some View {
        Group {
            InputMonitoringAlertView()
                .previewLayout(.sizeThatFits)
        }
    }
}
