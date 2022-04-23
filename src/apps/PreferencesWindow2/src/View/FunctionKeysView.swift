import SwiftUI

struct FunctionKeysView: View {
  @ObservedObject private var settings = Settings.shared
  @State private var selectedDevice: ConnectedDevice? = nil

  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      HStack(alignment: .top, spacing: 12.0) {
        List {
          VStack(alignment: .leading, spacing: 0) {
            Button(action: {
              selectedDevice = nil
            }) {
              HStack {
                Text("For all devices")

                Spacer()
              }
              .sidebarButtonLabelStyle()
            }
            .sidebarButtonStyle(selected: selectedDevice == nil)

            Divider()

            ForEach($settings.connectedDeviceSettings) { $connectedDeviceSetting in
              Button(action: {
                selectedDevice = connectedDeviceSetting.connectedDevice
              }) {
                HStack {
                  Text(
                    "\(connectedDeviceSetting.connectedDevice.productName) (\(connectedDeviceSetting.connectedDevice.manufacturerName)) [\(String(connectedDeviceSetting.connectedDevice.vendorId)),\(String(connectedDeviceSetting.connectedDevice.productId))]"
                  )

                  Spacer()

                  VStack {
                    if connectedDeviceSetting.connectedDevice.isKeyboard {
                      Image(systemName: "keyboard")
                    }
                    if connectedDeviceSetting.connectedDevice.isPointingDevice {
                      Image(systemName: "capsule.portrait")
                    }
                  }
                }
                .sidebarButtonLabelStyle()
              }
              .sidebarButtonStyle(
                selected: selectedDevice?.id == connectedDeviceSetting.connectedDevice.id)

              Divider()
            }

            Spacer()
          }
          .background(Color(NSColor.textBackgroundColor))
        }
        .frame(width: 250)

        VStack {
          FnFunctionKeysView(
            fnFunctionKeys: selectedDevice == nil
              ? settings.fnFunctionKeys
              : settings.findConnectedDeviceSetting(selectedDevice!)?.fnFunctionKeys ?? []
          )

          Spacer()
        }
      }
    }
    .padding()
  }

  struct FnFunctionKeysView: View {
    @State var fnFunctionKeys: [SimpleModification]

    var body: some View {
      List {
        VStack(alignment: .leading, spacing: 0) {
          ForEach($fnFunctionKeys) { $fnFunctionKey in
            HStack {
              Text(fnFunctionKey.fromEntry.label)
                .frame(width: 40)

              SimpleModificationPickerView(
                categories: SimpleModificationDefinitions.shared.toCategories,
                entry: fnFunctionKey.toEntry
              )
            }
          }
        }
      }
    }
  }
}

struct FunctionKeysView_Previews: PreviewProvider {
  static var previews: some View {
    FunctionKeysView()
  }
}
