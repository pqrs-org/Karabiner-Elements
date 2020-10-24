import SwiftUI

struct DeactivateDriverButton: View {
    @State private var showingAlert: Bool = false
    @State private var status: Int32 = 0

    var body: some View {
        Button(action: {
            VirtualHIDDeviceManager.shared.deactivateDriver(completion: { status in
                DispatchQueue.main.async {
                    self.status = status
                    self.showingAlert = true
                }
            })
        }) {
            Image(decorative: "ic_star_rate_18pt")
                .resizable()
                .frame(width: GUISize.buttonIconWidth, height: GUISize.buttonIconHeight)
            Text("Deactivate driver")
        }
        .alert(isPresented: $showingAlert) {
            Alert(title: Text(
                self.status == 0
                    ? "Deactivation Result"
                    : "Deactivation Error"
            ),
            message: Text(
                self.status == 0
                    ? "Driver was successfully deactivated."
                    : "Deactivation was failed. (error: \(self.status))"
            ),
            dismissButton: .default(Text("Close")))
        }
    }
}
