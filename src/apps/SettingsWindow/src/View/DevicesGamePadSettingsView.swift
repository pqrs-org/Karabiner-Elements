import SwiftUI

struct DevicesGamePadSettingsView: View {
  @ObservedObject var connectedDeviceSetting: ConnectedDeviceSetting
  @Binding var showing: Bool

  @ObservedObject private var settings = Settings.shared

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .leading, spacing: 12.0) {
        Text(
          "\(connectedDeviceSetting.connectedDevice.productName) (\(connectedDeviceSetting.connectedDevice.manufacturerName))"
        )
        .padding(.leading, 40)
        .padding(.top, 20)

        TabView {
          XYStickTabView(
            connectedDeviceSetting: connectedDeviceSetting,
            defaults: settings.deviceDefaults)
            .padding()
            .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .top)
            .tabItem {
              Text("XY stick")
            }

          WheelsStickTabView(
            connectedDeviceSetting: connectedDeviceSetting,
            defaults: settings.deviceDefaults)
            .padding()
            .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .top)
            .tabItem {
              Text("Wheels stick")
            }

          OthersTabView(connectedDeviceSetting: connectedDeviceSetting)
            .padding()
            .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .top)
            .tabItem {
              Text("Others")
            }
        }
      }

      SheetCloseButton {
        showing = false
      }
    }
    .padding()
    .frame(width: 1000, height: 600, alignment: .top)
  }

  struct XYStickTabView: View {
    @ObservedObject var connectedDeviceSetting: ConnectedDeviceSetting
    let defaults: SettingsConfigurationSnapshot.DeviceDefaults?

    var body: some View {
      VStack(alignment: .leading) {
        StickParametersView(
          deadzone: $connectedDeviceSetting.gamePadXYStickDeadzone,
          deadzoneDefaultValue: defaults?.gamePadXyStickDeadzone ?? 0.0,

          deltaMagnitudeDetectionThreshold: $connectedDeviceSetting
            .gamePadXYStickDeltaMagnitudeDetectionThreshold,
          deltaMagnitudeDetectionThresholdDefaultValue:
            defaults?.gamePadXyStickDeltaMagnitudeDetectionThreshold ?? 0.0,

          continuedMovementAbsoluteMagnitudeThreshold: $connectedDeviceSetting
            .gamePadXYStickContinuedMovementAbsoluteMagnitudeThreshold,
          continuedMovementAbsoluteMagnitudeThresholdDefaultValue:
            defaults?.gamePadXyStickContinuedMovementAbsoluteMagnitudeThreshold ?? 0.0,

          continuedMovementIntervalMilliseconds: $connectedDeviceSetting
            .gamePadXYStickContinuedMovementIntervalMilliseconds,
          continuedMovementIntervalMillisecondsDefaultValue:
            defaults?.gamePadXyStickContinuedMovementIntervalMilliseconds ?? 0
        )

        HStack(spacing: 20.0) {
          FormulaView(
            name: "X formula",
            value: $connectedDeviceSetting.gamePadStickXFormula,
            error: $connectedDeviceSetting.gamePadStickXFormulaError,
            resetFunction: {
              connectedDeviceSetting.resetGamePadStickXFormula()
            }
          )

          FormulaView(
            name: "Y formula",
            value: $connectedDeviceSetting.gamePadStickYFormula,
            error: $connectedDeviceSetting.gamePadStickYFormulaError,
            resetFunction: {
              connectedDeviceSetting.resetGamePadStickYFormula()
            }
          )
        }
        .padding(.top, 20.0)
      }
    }
  }

  struct WheelsStickTabView: View {
    @ObservedObject var connectedDeviceSetting: ConnectedDeviceSetting
    let defaults: SettingsConfigurationSnapshot.DeviceDefaults?

    var body: some View {
      VStack(alignment: .leading) {
        StickParametersView(
          deadzone: $connectedDeviceSetting.gamePadWheelsStickDeadzone,
          deadzoneDefaultValue: defaults?.gamePadWheelsStickDeadzone ?? 0.0,

          deltaMagnitudeDetectionThreshold: $connectedDeviceSetting
            .gamePadWheelsStickDeltaMagnitudeDetectionThreshold,
          deltaMagnitudeDetectionThresholdDefaultValue:
            defaults?.gamePadWheelsStickDeltaMagnitudeDetectionThreshold ?? 0.0,

          continuedMovementAbsoluteMagnitudeThreshold: $connectedDeviceSetting
            .gamePadWheelsStickContinuedMovementAbsoluteMagnitudeThreshold,
          continuedMovementAbsoluteMagnitudeThresholdDefaultValue:
            defaults?.gamePadWheelsStickContinuedMovementAbsoluteMagnitudeThreshold ?? 0.0,

          continuedMovementIntervalMilliseconds: $connectedDeviceSetting
            .gamePadWheelsStickContinuedMovementIntervalMilliseconds,
          continuedMovementIntervalMillisecondsDefaultValue:
            defaults?.gamePadWheelsStickContinuedMovementIntervalMilliseconds ?? 0
        )

        HStack(spacing: 20.0) {
          FormulaView(
            name: "vertical wheel formula",
            value: $connectedDeviceSetting.gamePadStickVerticalWheelFormula,
            error: $connectedDeviceSetting.gamePadStickVerticalWheelFormulaError,
            resetFunction: {
              connectedDeviceSetting.resetGamePadStickVerticalWheelFormula()
            }
          )

          FormulaView(
            name: "horizontal wheel formula",
            value: $connectedDeviceSetting.gamePadStickHorizontalWheelFormula,
            error: $connectedDeviceSetting.gamePadStickHorizontalWheelFormulaError,
            resetFunction: {
              connectedDeviceSetting.resetGamePadStickHorizontalWheelFormula()
            }
          )
        }
        .padding(.top, 20.0)
      }
    }
  }

  struct OthersTabView: View {
    @ObservedObject var connectedDeviceSetting: ConnectedDeviceSetting

    var body: some View {
      VStack(alignment: .leading, spacing: 40.0) {
        Toggle(isOn: $connectedDeviceSetting.gamePadSwapSticks) {
          Text("Swap gamepad XY and wheels sticks")
        }
        .switchToggleStyle(controlSize: .mini, font: .callout)

        DevicesMouseFlagsView(connectedDeviceSetting: connectedDeviceSetting)
      }
    }
  }

  struct StickParametersView: View {
    @Binding var deadzone: Double
    let deadzoneDefaultValue: Double

    @Binding var deltaMagnitudeDetectionThreshold: Double
    let deltaMagnitudeDetectionThresholdDefaultValue: Double

    @Binding var continuedMovementAbsoluteMagnitudeThreshold: Double
    let continuedMovementAbsoluteMagnitudeThresholdDefaultValue: Double

    @Binding var continuedMovementIntervalMilliseconds: Int
    let continuedMovementIntervalMillisecondsDefaultValue: Int

    var body: some View {
      Grid(alignment: .leadingFirstTextBaseline) {
        GridRow {
          Text("deadzone:")
            .gridColumnAlignment(.trailing)

          DoubleTextField(
            value: $deadzone,
            range: 0...1,
            step: 0.01,
            maximumFractionDigits: 2,
            width: 60)

          Text(
            "(Default: \(String(format: "%.2f", deadzoneDefaultValue)))"
          )
        }

        GridRow {
          Text("Delta magnitude detection threshold:")

          DoubleTextField(
            value: $deltaMagnitudeDetectionThreshold,
            range: 0...1,
            step: 0.01,
            maximumFractionDigits: 2,
            width: 60)

          Text(
            "(Default: \(String(format: "%.2f", deltaMagnitudeDetectionThresholdDefaultValue)))"
          )
        }

        GridRow {
          Text("Continued movement absolute magnitude threshold:")

          DoubleTextField(
            value: $continuedMovementAbsoluteMagnitudeThreshold,
            range: 0...1,
            step: 0.1,
            maximumFractionDigits: 2,
            width: 60)

          Text(
            "(Default: \(String(format: "%.2f", continuedMovementAbsoluteMagnitudeThresholdDefaultValue)))"
          )
        }

        GridRow {
          Text("Continued movement interval milliseconds:")

          IntTextField(
            value: $continuedMovementIntervalMilliseconds,
            range: 0...1000,
            step: 1,
            width: 60)

          Text("(Default: \(continuedMovementIntervalMillisecondsDefaultValue))")
        }
      }
    }
  }

  struct FormulaView: View {
    let name: String
    @Binding var value: String
    @Binding var error: Bool
    let resetFunction: () -> Void

    var body: some View {
      VStack {
        HStack {
          Text(name)

          if error {
            Label(
              "Invalid formula",
              systemImage: ErrorBorder.icon
            )
            .modifier(ErrorBorder(padding: 4.0))
          }

          Spacer()

          Button(
            role: .destructive,
            action: {
              resetFunction()
            },
            label: {
              Label("Reset to the default formula", systemImage: "trash")
                .buttonLabelStyle()
            }
          )
          .deleteButtonStyle()
        }

        TextEditor(text: $value)
          .frame(height: 250.0)
      }
    }
  }
}
