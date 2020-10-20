import SwiftUI

struct DriverNotLoadedAlertView: View {
    public static let title = "Driver Alert"

    var body: some View {
        VStack(alignment: .leading) {
            HStack {
                Image(decorative: "baseline_warning_black_48pt")
                    .resizable()
                    .frame(width: 48.0, height: 48.0)
                Text(DriverNotLoadedAlertView.title)
            }

            HStack(alignment: .top) {
                GroupBox {
                    VStack(alignment: .leading, spacing: 20.0) {
                        Text("The virtual keyboard and mouse driver is not loaded.\nPlease allow system software from \".Karabiner-VirtualHIDDevice-Manager\" on Security & Privacy System Preferences.")
                            .fixedSize(horizontal: false, vertical: true)

                        Button(action: { openSystemPreferencesSecurity() }) {
                            Image(decorative: "ic_forward_18pt")
                                .resizable()
                                .frame(width: GUISize.buttonIconWidth, height: GUISize.buttonIconHeight)
                            Text("Open Security & Privacy System Preferences")
                                .fixedSize(horizontal: false, vertical: true)
                        }

                        Image(decorative: "dext-allow")
                            .resizable()
                            .aspectRatio(contentMode: .fit)
                            .frame(height: 200)
                            .border(Color.gray, width: 1)
                    }.padding()
                }.frame(width: 400)

                GroupBox(label: Text("Advanced")) {
                    VStack(alignment: .leading, spacing: 20.0) {
                        Text("If macOS failed to load the driver in the early stage, the allow button might be not shown on Security & Privacy System Preferences.\nIn this case, you need to reinstall the driver in order for the button to appear.")
                            .fixedSize(horizontal: false, vertical: true)

                        Text("How to reinstall driver:")

                        VStack(alignment: .leading, spacing: 10.0) {
                            Text("1. Press the following button to deactivate driver.\n(The administrator password will be required.)")
                                .fixedSize(horizontal: false, vertical: true)

                            DeactivateDriverButton()

                            Text("2. Restart macOS.")
                                .fontWeight(.bold)
                                .fixedSize(horizontal: false, vertical: true)

                            Text("3. Press the following button to activate driver.")
                                .fixedSize(horizontal: false, vertical: true)

                            ActivateDriverButton()

                            Text("4. \"System Extension Blocked\" alert is shown.")
                                .fixedSize(horizontal: false, vertical: true)

                            Text("5. Open Security Preferences and press the allow button.")
                                .fixedSize(horizontal: false, vertical: true)
                        }
                    }.padding()
                }.frame(width: 400)
            }
        }
        .padding()
    }

    private func openSystemPreferencesSecurity() {
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
