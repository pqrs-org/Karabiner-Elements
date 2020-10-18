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

            Button(action: { openSystemPreferencesSecurity() }) {
                Image(decorative: "ic_forward_18pt")
                Text("Open Security & Privacy System Preferences")
            }
            .padding(.top, 20)

            Image(decorative: "input_monitoring")
                .resizable()
                .aspectRatio(contentMode: .fit)
                .frame(height: 200.0)
                .border(Color.gray, width: /*@START_MENU_TOKEN@*/1/*@END_MENU_TOKEN@*/)
                .padding(.top, 40)
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
