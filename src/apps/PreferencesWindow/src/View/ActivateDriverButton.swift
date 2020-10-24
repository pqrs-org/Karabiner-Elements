import SwiftUI

struct ActivateDriverButton: View {
    @State private var showingResult: Bool = false
    @State private var status: Int32 = 0

    var body: some View {
        VStack(alignment: .leading) {
            Button(action: {
                VirtualHIDDeviceManager.shared.activateDriver(completion: { status in
                    DispatchQueue.main.async {
                        self.status = status
                        self.showingResult = true
                    }
                })
            }) {
                Image(decorative: "ic_star_rate_18pt")
                    .resizable()
                    .frame(width: GUISize.buttonIconWidth, height: GUISize.buttonIconHeight)
                Text("Activate driver")
            }
            if self.showingResult {
                VStack(alignment: .leading) {
                    if self.status == 0 {
                        Text("Driver was successfully activated.")
                            .bold()
                    } else {
                        Text("Activation was failed. (error: \(self.status))")
                            .bold()
                    }
                }.padding(.bottom, 20)
            }
        }
    }
}
