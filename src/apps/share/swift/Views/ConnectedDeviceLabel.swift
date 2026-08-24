import SwiftUI

struct ConnectedDeviceLabel: View {
  let title: String
  let isKeyboard: Bool
  let isPointingDevice: Bool
  let isGamePad: Bool
  let isConsumer: Bool
  var fallbackSystemImageName: String? = nil

  var body: some View {
    Label {
      Text(title)
        .lineLimit(nil)
        .fixedSize(horizontal: false, vertical: true)
    } icon: {
      if let fallbackSystemImageName {
        Image(systemName: fallbackSystemImageName)
      } else {
        VStack {
          if isKeyboard { Image(systemName: "keyboard") }
          if isPointingDevice { Image(systemName: "computermouse") }
          if isGamePad { Image(systemName: "gamecontroller") }
          if isConsumer { Image(systemName: "headphones") }
        }
        .frame(width: 20)
      }
    }
    .frame(maxWidth: .infinity, alignment: .leading)
    .padding(.vertical, 8)
    .listRowSeparator(.visible, edges: .bottom)
  }
}
