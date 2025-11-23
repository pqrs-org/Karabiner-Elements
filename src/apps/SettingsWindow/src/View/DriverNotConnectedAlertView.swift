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
