import SwiftUI

struct DeactivateDriverButton: View {
    @State private var showingProgress: Bool = false
    @State private var showingResult: Bool = false
    @State private var status: Int32 = 0

    var body: some View {
        VStack(alignment: .leading) {
            Button(action: {
                self.showingProgress = true
                self.showingResult = false

                VirtualHIDDeviceManager.shared.deactivateDriver(completion: { status in
                    DispatchQueue.main.async {
                        self.status = status
                        self.showingProgress = false
                        self.showingResult = true
                    }
                })
            }) {
                Image(decorative: "ic_star_rate_18pt")
                    .resizable()
                    .frame(width: GUISize.buttonIconWidth, height: GUISize.buttonIconHeight)
                Text("Deactivate driver")
            }

            if self.showingProgress {
                Text("Deactivating ...")
                    .padding(.bottom, 20)
            }

            if self.showingResult {
                VStack(alignment: .leading) {
                    if self.status == 0 {
                        Text("Driver was successfully deactivated.")
                            .bold()
                    } else {
                        Text("Deactivation was failed. (error: \(self.status))")
                            .bold()
                    }
                }.padding(.bottom, 20)
            }
        }
    }
}
