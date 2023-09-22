import SwiftUI

struct DevicesView: View {
  @ObservedObject private var settings = LibKrbn.Settings.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      List {
        VStack(alignment: .leading, spacing: 0.0) {
          ForEach($settings.connectedDeviceSettings) { $connectedDeviceSetting in
            VStack(alignment: .leading, spacing: 8.0) {
              HStack(alignment: .center, spacing: 0) {
                HStack(spacing: 4.0) {
                  Spacer()
                  if connectedDeviceSetting.connectedDevice.isKeyboard {
                    Image(systemName: "keyboard")
                  }
                  if connectedDeviceSetting.connectedDevice.isPointingDevice {
                    Image(systemName: "capsule.portrait")
                  }
                  if connectedDeviceSetting.connectedDevice.isGamePad {
                    Image(systemName: "gamecontroller")
                  }
                }
                .frame(width: 50.0)

                Text(
                  "\(connectedDeviceSetting.connectedDevice.productName) (\(connectedDeviceSetting.connectedDevice.manufacturerName))"
                )
                .padding(.leading, 12.0)

                Spacer()
              }

              HStack(alignment: .top, spacing: 0.0) {
                VStack(alignment: .leading, spacing: 0.0) {
                  if connectedDeviceSetting.connectedDevice.isAppleDevice,
                    !connectedDeviceSetting.connectedDevice.isKeyboard,
                    !settings.unsafeUI
                  {
                    Text("Apple pointing devices are not supported")
                      .foregroundColor(Color(NSColor.placeholderTextColor))
                    Spacer()
                  } else {
                    VStack(alignment: .leading, spacing: 4.0) {
                      HStack {
                        Toggle(isOn: $connectedDeviceSetting.modifyEvents) {
                          Text("Modify events")
                        }
                        .switchToggleStyle()
                        .frame(width: 140.0)

                        Spacer()
                      }
                      .padding(.bottom, 10)

                      VStack(alignment: .leading, spacing: 2.0) {
                        if connectedDeviceSetting.connectedDevice.isKeyboard {
                          if connectedDeviceSetting.modifyEvents {
                            Toggle(isOn: $connectedDeviceSetting.manipulateCapsLockLed) {
                              Text("Manipulate caps lock LED")

                              Spacer()
                            }
                            .switchToggleStyle(controlSize: .mini, font: .callout)
                          }

                          if !connectedDeviceSetting.connectedDevice.isBuiltInKeyboard
                            && !connectedDeviceSetting.disableBuiltInKeyboardIfExists
                          {
                            Toggle(isOn: $connectedDeviceSetting.treatAsBuiltInKeyboard) {
                              Text("Treat as a built-in keyboard")

                              Spacer()
                            }
                            .switchToggleStyle(controlSize: .mini, font: .callout)
                          }

                          if !connectedDeviceSetting.connectedDevice.isBuiltInKeyboard
                            && !connectedDeviceSetting.treatAsBuiltInKeyboard
                          {
                            Toggle(isOn: $connectedDeviceSetting.disableBuiltInKeyboardIfExists) {
                              Text("Disable the built-in keyboard while this device is connected")

                              Spacer()
                            }
                            .switchToggleStyle(controlSize: .mini, font: .callout)
                          }
                        }
                      }.padding(.leading, 20)
                    }
                    .frame(width: 400.0)
                  }
                }

                Spacer()

                if connectedDeviceSetting.connectedDevice.transport != "FIFO" {
                  VStack(alignment: .trailing, spacing: 4.0) {
                    if connectedDeviceSetting.connectedDevice.vendorId != 0 {
                      HStack(alignment: .firstTextBaseline, spacing: 0) {
                        Spacer()

                        Text("Vendor ID: ")

                        Text(
                          String(
                            format: "%5d (0x%04x)",
                            connectedDeviceSetting.connectedDevice.vendorId,
                            connectedDeviceSetting.connectedDevice.vendorId)
                        )
                      }
                    }

                    if connectedDeviceSetting.connectedDevice.productId != 0 {
                      HStack(alignment: .center, spacing: 0) {
                        Spacer()

                        Text("Product ID: ")

                        Text(
                          String(
                            format: "%5d (0x%04x)",
                            connectedDeviceSetting.connectedDevice.productId,
                            connectedDeviceSetting.connectedDevice.productId)
                        )
                      }
                    }

                    if !connectedDeviceSetting.connectedDevice.deviceAddress.isEmpty {
                      HStack(alignment: .center, spacing: 0) {
                        Spacer()

                        Text("Device Address: ")

                        Text(connectedDeviceSetting.connectedDevice.deviceAddress)
                      }
                    }
                  }
                  .font(.custom("Menlo", size: 12.0))
                }
              }
              .padding(.leading, 62.0)
              .padding(.top, 4.0)

              if connectedDeviceSetting.connectedDevice.isPointingDevice
                || connectedDeviceSetting.connectedDevice.isGamePad
              {
                if connectedDeviceSetting.modifyEvents {
                  HStack(alignment: .top, spacing: 100.0) {
                    VStack(alignment: .leading, spacing: 0.0) {
                      Toggle(isOn: $connectedDeviceSetting.mouseFlipX) {
                        Text("Flip mouse X")

                        Spacer()
                      }
                      .switchToggleStyle(controlSize: .mini, font: .callout)

                      Toggle(isOn: $connectedDeviceSetting.mouseFlipY) {
                        Text("Flip mouse Y")

                        Spacer()
                      }
                      .switchToggleStyle(controlSize: .mini, font: .callout)

                      Toggle(isOn: $connectedDeviceSetting.mouseFlipVerticalWheel) {
                        Text("Flip mouse vertical wheel")

                        Spacer()
                      }
                      .switchToggleStyle(controlSize: .mini, font: .callout)

                      Toggle(isOn: $connectedDeviceSetting.mouseFlipHorizontalWheel) {
                        Text("Flip mouse horizontal wheel")

                        Spacer()
                      }
                      .switchToggleStyle(controlSize: .mini, font: .callout)

                      Spacer()
                    }
                    .frame(width: 200.0)

                    VStack(alignment: .leading, spacing: 0.0) {
                      Toggle(isOn: $connectedDeviceSetting.mouseSwapXY) {
                        Text("Swap mouse X and Y")

                        Spacer()
                      }
                      .switchToggleStyle(controlSize: .mini, font: .callout)

                      Toggle(isOn: $connectedDeviceSetting.mouseSwapWheel) {
                        Text("Swap mouse wheels")

                        Spacer()
                      }
                      .switchToggleStyle(controlSize: .mini, font: .callout)

                      Spacer()
                    }
                    .frame(width: 200.0)

                    Spacer()
                  }
                  .padding(.leading, 62.0 + 20.0)
                }
              }
            }
            .padding(.vertical, 12.0)
            .padding(.trailing, 12.0)
            .overlay(
              RoundedRectangle(cornerRadius: 8)
                .stroke(
                  Color(NSColor.selectedControlColor),
                  lineWidth: connectedDeviceSetting.modifyEvents ? 3 : 0
                )
                .padding(2)
            )

            Divider()
          }

          Spacer()
        }
      }
      .background(Color(NSColor.textBackgroundColor))
    }
    .padding()
  }
}

struct DevicesView_Previews: PreviewProvider {
  static var previews: some View {
    DevicesView()
  }
}
