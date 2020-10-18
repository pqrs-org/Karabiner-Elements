import SwiftUI

struct DriverNotLoadedAlertView: View {
    var body: some View {
        VStack(alignment: .leading) {
            HStack {
                Image(decorative: "baseline_warning_black_48pt")
                    .resizable()
                    .frame(width: 48.0, height: 48.0)
                Text("Driver Alert")
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
                        }

                        Image(decorative: "dext-allow")
                            .resizable()
                            .aspectRatio(contentMode: .fit)
                            .frame(width: 200)
                            .border(Color.gray, width: 1)
                    }.padding()
                }.frame(width: 400)

                GroupBox(label: Text("Advanced")) {
                    VStack(alignment: .leading, spacing: 20.0) {
                        Text("If macOS failed to load the driver in the early stage, the allow button might be not shown on Security & Privacy System Preferences.\nIn this case, you need to reinstall the driver in order for the button to appear.")
                            .fixedSize(horizontal: false, vertical: true)

                        Text("How to reinstall driver:")

                        VStack(alignment: .leading, spacing: 10.0) {
                            Text("1. Press the following button to deactivate driver.")

                            Button(action: { VirtualHIDDeviceManager.shared.deactivateDriver() }) {
                                Image(decorative: "ic_star_rate_18pt")
                                    .resizable()
                                    .frame(width: GUISize.buttonIconWidth, height: GUISize.buttonIconHeight)
                                Text("Deactivate driver")
                            }

                            Text("2. Enter the administrator password to complete.")

                            Text("3. Restart macOS.")
                                .fontWeight(.bold)

                            Text("4. Press the following button to activate driver.")

                            Button(action: { VirtualHIDDeviceManager.shared.activateDriver() }) {
                                Image(decorative: "ic_star_rate_18pt")
                                    .resizable()
                                    .frame(width: GUISize.buttonIconWidth, height: GUISize.buttonIconHeight)
                                Text("Activate driver")
                            }

                            Text("5. \"System Extension Blocked\" alert is shown.")

                            Text("6. Open Security Preferences and press the allow button.")
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
