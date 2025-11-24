import SwiftUI

struct VirtualHidDeviceServiceClientNotConnectedAlertView: View {
  @FocusState var focus: Bool

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .center) {
        Label(
          "Waiting for a connection from Karabiner-Core-Service to Karabiner-VirtualHIDDevice-Daemon",
          systemImage: "hourglass"
        )
        .font(.system(size: 24))
        .textSelection(.enabled)

        ProgressView()

        Label(
          "If the message remains visible, third-party security software may be blocking communication with Karabiner-VirtualHIDDevice-Daemon.",
          systemImage: "lightbulb"
        )
        .fixedSize(horizontal: false, vertical: true)
        .textSelection(.enabled)
      }
      .padding()
      .frame(width: 850)

      SheetCloseButton {
        ContentViewStates.shared.showVirtualHidDeviceServiceClientNotConnectedAlert = false
      }
    }
    .onAppear {
      focus = true
    }
  }
}
