import SwiftUI

struct DevicesGamePadSettingsView: View {
  @Binding var connectedDeviceSetting: LibKrbn.ConnectedDeviceSetting
  @Binding var showing: Bool

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .leading, spacing: 12.0) {
        Text(
          "\(connectedDeviceSetting.connectedDevice.productName) (\(connectedDeviceSetting.connectedDevice.manufacturerName))"
        )
        .padding(.leading, 40)
        .padding(.top, 20)

        GroupBox(label: Text("Deadzone")) {
          VStack(alignment: .leading) {
            HStack {
              Toggle(isOn: $connectedDeviceSetting.gamePadOverwriteXYStickDeadzone) {
                Text("Overwrite XY stick deadzone: ")
                  .frame(maxWidth: .infinity, alignment: .leading)
              }
              .switchToggleStyle(controlSize: .mini, font: .callout)
              .frame(width: 250.0)

              HStack(alignment: .center, spacing: 8.0) {
                Text("Deadzone:")

                DoubleTextField(
                  value: $connectedDeviceSetting.gamePadXYStickDeadzone,
                  range: 0.05...1,
                  step: 0.01,
                  maximumFractionDigits: 3,
                  width: 60)

                Text("(Default: 0.1)")
              }
              .padding(.leading, 20)
              .disabled(!connectedDeviceSetting.gamePadOverwriteXYStickDeadzone)

              Spacer()
            }

            HStack {
              Toggle(isOn: $connectedDeviceSetting.gamePadOverwriteWheelsStickDeadzone) {
                Text("Overwrite Wheels stick deadzone: ")
                  .frame(maxWidth: .infinity, alignment: .leading)
              }
              .switchToggleStyle(controlSize: .mini, font: .callout)
              .frame(width: 250.0)

              HStack(alignment: .center, spacing: 8.0) {
                Text("Deadzone:")

                DoubleTextField(
                  value: $connectedDeviceSetting.gamePadWheelsStickDeadzone,
                  range: 0.05...1,
                  step: 0.01,
                  maximumFractionDigits: 3,
                  width: 60)

                Text("(Default: 0.1)")
              }
              .padding(.leading, 20)
              .disabled(!connectedDeviceSetting.gamePadOverwriteWheelsStickDeadzone)
            }
          }.padding()
        }

        GroupBox(label: Text("Stick parameters")) {
          VStack(alignment: .leading) {
            HStack {
              Toggle(
                isOn: $connectedDeviceSetting
                  .gamePadOverwriteStickStrokeAccelerationTransitionDurationMilliseconds
              ) {
                Text("Overwrite stick stroke acceleration transition duration: ")
                  .frame(maxWidth: .infinity, alignment: .leading)
              }
              .switchToggleStyle(controlSize: .mini, font: .callout)
              .frame(width: 250.0)

              HStack(alignment: .center, spacing: 8.0) {
                Text("Duration:")

                IntTextField(
                  value: $connectedDeviceSetting
                    .gamePadStickStrokeAccelerationTransitionDurationMilliseconds,
                  range: 0...10000,
                  step: 100,
                  width: 60)

                Text("milliseconds (Default: 500)")
              }
              .padding(.leading, 20)
              .disabled(
                !connectedDeviceSetting
                  .gamePadOverwriteStickStrokeAccelerationTransitionDurationMilliseconds)

              Spacer()
            }

            Toggle(isOn: $connectedDeviceSetting.gamePadSwapSticks) {
              Text("Swap gamepad XY and wheels sticks")
            }
            .switchToggleStyle(controlSize: .mini, font: .callout)
          }
        }

        Spacer()
      }

      SheetCloseButton {
        showing = false
      }
    }
    .padding()
    .frame(width: 1000, height: 600)
  }
}

struct DevicesGamePadSettingsView_Previews: PreviewProvider {
  @State static var connectedDeviceSetting = LibKrbn.ConnectedDeviceSetting(
    LibKrbn.ConnectedDevice(
      index: 0,
      manufacturerName: "",
      productName: "",
      transport: "",
      vendorId: 0,
      productId: 0,
      deviceAddress: nil,
      isKeyboard: false,
      isPointingDevice: false,
      isGamePad: true,
      isBuiltInKeyboard: false,
      isBuiltInTrackpad: false,
      isBuiltInTouchBar: false,
      isAppleDevice: false
    )
  )
  @State static var showing = true

  static var previews: some View {
    DevicesGamePadSettingsView(
      connectedDeviceSetting: $connectedDeviceSetting,
      showing: $showing)
  }
}
