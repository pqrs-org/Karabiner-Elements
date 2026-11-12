import SwiftUI

struct ConnectedDeviceIcons: View {
  let isKeyboard: Bool
  let isPointingDevice: Bool
  let isGamePad: Bool
  let isConsumer: Bool
  var fallbackSystemImageName: String?

  var body: some View {
    VStack(spacing: 4.0) {
      if let fallbackSystemImageName {
        Image(systemName: fallbackSystemImageName)
      } else {
        if isKeyboard { Image(systemName: "keyboard") }
        if isPointingDevice { Image(systemName: "computermouse") }
        if isGamePad { Image(systemName: "gamecontroller") }
        if isConsumer { Image(systemName: "headphones") }
      }
    }
    .frame(width: 20.0, alignment: .center)
  }
}

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
      ConnectedDeviceIcons(
        isKeyboard: isKeyboard,
        isPointingDevice: isPointingDevice,
        isGamePad: isGamePad,
        isConsumer: isConsumer,
        fallbackSystemImageName: fallbackSystemImageName
      )
    }
    .frame(maxWidth: .infinity, alignment: .leading)
    .padding(.vertical, 8)
    .listRowSeparator(.visible, edges: .bottom)
  }
}
