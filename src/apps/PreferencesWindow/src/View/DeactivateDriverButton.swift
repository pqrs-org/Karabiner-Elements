import SwiftUI

struct DeactivateDriverButton: View {
    var body: some View {
        Button(action: { VirtualHIDDeviceManager.shared.deactivateDriver() }) {
            Image(decorative: "ic_star_rate_18pt")
                .resizable()
                .frame(width: GUISize.buttonIconWidth, height: GUISize.buttonIconHeight)
            Text("Deactivate driver")
        }
    }
}
