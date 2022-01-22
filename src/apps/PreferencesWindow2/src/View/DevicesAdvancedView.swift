import SwiftUI

struct DevicesAdvancedView: View {
    @ObservedObject private var settings = Settings.shared

    var body: some View {
        VStack(alignment: .leading, spacing: 12.0) {
            GroupBox(label: Text("Delay before open device")) {
                VStack(alignment: .leading, spacing: 12.0) {
                    HStack {
                        IntTextField(value: $settings.delayMillisecondsBeforeOpenDevice,
                                     range: 0 ... 10000,
                                     step: 100,
                                     width: 50)

                        Text("milliseconds (Default value is 1000)")
                        Spacer()
                    }

                    HStack(alignment: .center, spacing: 12.0) {
                        Image(systemName: "exclamationmark.triangle")

                        VStack(alignment: .leading, spacing: 0.0) {
                            Text("Setting insufficient delay (e.g., 0) will result in a device becoming unusable after Karabiner-Elements is quit.")
                            Text("(This is a macOS problem and can be solved by unplugging the device and plugging it again.)")
                        }
                    }
                }
                .padding(6.0)
            }

            Spacer()
                .frame(height: 24.0)

            GroupBox(label: Text("Disable the built-in keyboard while one of the following selected devices is connected")) {
                ScrollView {
                    VStack(alignment: .leading, spacing: 0.0) {
                        // swiftformat:disable:next unusedArguments
                        ForEach($settings.connectedDeviceSettings) { $connectedDeviceSetting in
                            HStack(alignment: .center, spacing: 0) {
                                Toggle(isOn: $connectedDeviceSetting.disableBuiltInKeyboardIfExists) {
                                    Text("\(connectedDeviceSetting.connectedDevice.productName) (\(connectedDeviceSetting.connectedDevice.manufacturerName)) [\(String(connectedDeviceSetting.connectedDevice.vendorId)),\(String(connectedDeviceSetting.connectedDevice.productId))]")
                                }.disabled(
                                    connectedDeviceSetting.connectedDevice.isBuiltInKeyboard ||
                                        connectedDeviceSetting.connectedDevice.isBuiltInTrackpad ||
                                        connectedDeviceSetting.connectedDevice.isBuiltInTouchBar
                                )

                                Spacer()

                                if connectedDeviceSetting.connectedDevice.isKeyboard {
                                    Image(systemName: "keyboard")
                                }
                                if connectedDeviceSetting.connectedDevice.isPointingDevice {
                                    Image(systemName: "capsule.portrait")
                                }
                            }
                            .padding(12.0)

                            Divider()
                        }

                        Spacer()
                    }
                }
                .background(Color(NSColor.textBackgroundColor))
            }

            Spacer()
        }
        .padding()
    }
}

struct DevicesAdvancediew_Previews: PreviewProvider {
    static var previews: some View {
        DevicesAdvancedView()
    }
}
