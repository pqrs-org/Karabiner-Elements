import SwiftUI

struct DriverNotConnectedAlertView: View {
  @FocusState var focus: Bool

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .center) {
        Label(
          "Waiting to connect to Karabiner-VirtualHIDDevice-Daemon",
          systemImage: "hourglass"
        )
        .font(.system(size: 24))

        ProgressView()

        Label(
          "If the message remains visible, third-party security software may be blocking communication with Karabiner-VirtualHIDDevice-Daemon.",
          systemImage: "lightbulb"
        )
        .fixedSize(horizontal: false, vertical: true)
      }
      .padding()
      .frame(width: 850)

      SheetCloseButton {
        ContentViewStates.shared.showDriverNotConnectedAlert = false
      }
    }
    .onAppear {
      focus = true
    }
  }
}
