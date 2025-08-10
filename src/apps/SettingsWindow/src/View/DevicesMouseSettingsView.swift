import SwiftUI

struct DevicesMouseSettingsView: View {
  @ObservedObject var connectedDeviceSetting: LibKrbn.ConnectedDeviceSetting
  @Binding var showing: Bool

  @ObservedObject private var settings = LibKrbn.Settings.shared

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .leading, spacing: 40.0) {
        Text(
          "\(connectedDeviceSetting.connectedDevice.productName) (\(connectedDeviceSetting.connectedDevice.manufacturerName))"
        )
        .font(.title)
        .padding(.leading, 40)
        .padding(.top, 20)

        GroupBox(label: Text("Multiplier")) {
          Grid(alignment: .leadingFirstTextBaseline) {
            GridRow {
              Text("XY movement multiplier:")

              DoubleTextField(
                value: $connectedDeviceSetting.pointingMotionXYMultiplier,
                range: 0...10000,
                step: 0.1,
                maximumFractionDigits: 1,
                width: 60)

              Text(
                "(Default: \(String(format: "%.01f)", libkrbn_core_configuration_pointing_motion_xy_multiplier_default_value()))"
              )
            }

            GridRow {
              Text("Wheels multiplier:")

              DoubleTextField(
                value: $connectedDeviceSetting.pointingMotionWheelsMultiplier,
                range: 0...10000,
                step: 0.1,
                maximumFractionDigits: 1,
                width: 60)

              Text(
                "(Default: \(String(format: "%.01f)", libkrbn_core_configuration_pointing_motion_wheels_multiplier_default_value()))"
              )
            }
          }
          .padding()
        }

        DevicesMouseFlagsView(connectedDeviceSetting: connectedDeviceSetting)
      }

      SheetCloseButton {
        showing = false
      }
    }
    .padding()
    .frame(width: 1000, height: 600, alignment: .top)
  }
}
