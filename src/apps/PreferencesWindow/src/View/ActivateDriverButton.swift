import SwiftUI

struct ActivateDriverButton: View {
    @State private var showingAlert: Bool = false
    @State private var status: Int32 = 0

    var body: some View {
        Button(action: {
            VirtualHIDDeviceManager.shared.activateDriver(completion: { status in
                DispatchQueue.main.async {
                    self.status = status
                    self.showingAlert = true
                }
            })
        }) {
            Image(decorative: "ic_star_rate_18pt")
                .resizable()
                .frame(width: GUISize.buttonIconWidth, height: GUISize.buttonIconHeight)
            Text("Activate driver")
        }
        .alert(isPresented: $showingAlert) {
            Alert(title: Text(
                self.status == 0
                    ? "Activation Result"
                    : "Activation Error"
            ),
            message: Text(
                self.status == 0
                    ? "Driver was successfully activated."
                    : "Activation was failed. (error: \(self.status))"
            ),
            dismissButton: .default(Text("Close")))
        }
    }
}
