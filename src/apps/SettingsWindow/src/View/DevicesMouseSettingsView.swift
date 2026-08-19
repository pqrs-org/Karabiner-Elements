import SwiftUI

struct DevicesMouseSettingsView: View {
  let connectedDevice: ConnectedDevice
  @Binding var deviceConfiguration: SettingsConfiguration.Device
  @Binding var showing: Bool

  @ObservedObject private var settings = Settings.shared

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .leading, spacing: 40.0) {
        Text(
          "\(connectedDevice.productName) (\(connectedDevice.manufacturerName))"
        )
        .font(.title)
        .padding(.leading, 40)
        .padding(.top, 20)

        GroupBox(label: Text("Multiplier")) {
          Grid(alignment: .leadingFirstTextBaseline) {
            GridRow {
              Text("XY movement multiplier:")

              DoubleTextField(
                value: $deviceConfiguration.pointingMotionXyMultiplier,
                range: 0...10000,
                step: 0.1,
                maximumFractionDigits: 1,
                width: 60)

              Text(
                "(Default: \(String(format: "%.01f)", settings.configuration.deviceDefaults.pointingMotionXyMultiplier))"
              )
            }

            GridRow {
              Text("Wheels multiplier:")

              DoubleTextField(
                value: $deviceConfiguration.pointingMotionWheelsMultiplier,
                range: 0...10000,
                step: 0.1,
                maximumFractionDigits: 1,
                width: 60)

              Text(
                "(Default: \(String(format: "%.01f)", settings.configuration.deviceDefaults.pointingMotionWheelsMultiplier))"
              )
            }
          }
          .padding()
        }

        DevicesMouseFlagsView(deviceConfiguration: $deviceConfiguration)
      }

      SheetCloseButton {
        showing = false
      }
    }
    .padding()
    .frame(height: 600, alignment: .top)
  }
}
