import SwiftUI

struct DriverNotLoadedAlertView: View {
    var body: some View {
        VStack(alignment: .center) {
            HStack {
                Image(decorative: "baseline_warning_black_48pt")
                    .resizable()
                    .frame(width: 48.0, height: 48.0)
                Text("Driver Alert")
            }

            Text("Hello!")
        }
        .padding()
    }

    func openSystemPreferencesSecurity() {
        NSApplication.shared.miniaturizeAll(self)

        let url = URL(string: "x-apple.systempreferences:com.apple.preference.security?General")!
        NSWorkspace.shared.open(url)
    }
}

struct DriverNotLoadedAlertView_Previews: PreviewProvider {
    static var previews: some View {
        Group {
            DriverNotLoadedAlertView()
                .previewLayout(.sizeThatFits)
        }
    }
}
