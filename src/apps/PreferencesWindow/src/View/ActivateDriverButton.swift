import SwiftUI

struct ActivateDriverButton: View {
    var body: some View {
        Button(action: { VirtualHIDDeviceManager.shared.activateDriver() }) {
            Image(decorative: "ic_star_rate_18pt")
                .resizable()
                .frame(width: GUISize.buttonIconWidth, height: GUISize.buttonIconHeight)
            Text("Activate driver")
        }
    }
}
