import SwiftUI

struct InputMonitoringAlertView: View {
    var body: some View {
        VStack(alignment: .center, spacing: 20.0) {
            Label("EventViewer Alert", systemImage: "exclamationmark.triangle.fill").font(.system(size: 24))

            Text("EventViewer requires Input Monitoring permission.\nYou have to allow Karabiner-EventViewer on Security & Privacy System Preferences.")
                .multilineTextAlignment(.center)
                .fixedSize(horizontal: false, vertical: true)

            Button(action: { openSystemPreferencesSecurity() }) {
                Label("Open Security & Privacy System Preferences...", systemImage: "arrow.forward.circle.fill")
            }

            Image(decorative: "input_monitoring")
                .resizable()
                .aspectRatio(contentMode: .fit)
                .frame(height: 200.0)
                .border(Color.gray, width: 1)
        }
        .frame(width: 600)
        .padding()
    }

    func openSystemPreferencesSecurity() {
        NSApplication.shared.miniaturizeAll(self)

        let url = URL(string: "x-apple.systempreferences:com.apple.preference.security?Privacy_ListenEvent")!
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
