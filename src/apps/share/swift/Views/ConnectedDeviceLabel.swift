import SwiftUI

func connectedDeviceLabelTitle(
  productName: String,
  manufacturerName: String,
  vendorId: UInt64,
  productId: UInt64,
  deviceAddress: String
) -> String {
  var title = productName
  if !manufacturerName.isEmpty {
    title += " (\(manufacturerName))"
  }

  if vendorId != 0 || productId != 0 {
    title += "\n  [VID: \(vendorId), PID: \(productId)]"
  } else if !deviceAddress.isEmpty {
    title += "\n  \(deviceAddress)"
  }

  return title
}

struct ConnectedDeviceLabel: View {
  let title: String
  let isKeyboard: Bool
  let isPointingDevice: Bool
  let isGamePad: Bool
  let isConsumer: Bool
  var fallbackSystemImageName: String?

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
