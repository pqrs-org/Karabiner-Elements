import SwiftUI

struct DevicesBuiltInErrorMessageView: View {
  @Binding var connectedDeviceSetting: LibKrbn.ConnectedDeviceSetting

  var body: some View {
    if connectedDeviceSetting.treatAsBuiltInKeyboard
      && connectedDeviceSetting.disableBuiltInKeyboardIfExists
    {
      VStack {
        Label(
          "Cannot use both \"Treat as a built-in keyboard\" and \"Disable the built-in keyboard\" together for the same device.",
          systemImage: "exclamationmark.circle.fill"
        )
        .padding()
      }
      .foregroundColor(Color.errorForeground)
      .background(Color.errorBackground)
    }
  }
}
