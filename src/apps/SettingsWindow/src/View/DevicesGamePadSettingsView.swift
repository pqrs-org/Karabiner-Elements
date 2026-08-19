import SwiftUI

struct DevicesGamePadSettingsView: View {
  let connectedDevice: ConnectedDevice
  @Binding var deviceConfiguration: SettingsConfiguration.Device
  @Binding var showing: Bool

  @ObservedObject private var settings = Settings.shared

  @State private var gamePadStickXFormula: String
  @State private var gamePadStickYFormula: String
  @State private var gamePadStickVerticalWheelFormula: String
  @State private var gamePadStickHorizontalWheelFormula: String
  @State private var gamePadStickXFormulaError = false
  @State private var gamePadStickYFormulaError = false
  @State private var gamePadStickVerticalWheelFormulaError = false
  @State private var gamePadStickHorizontalWheelFormulaError = false

  init(
    connectedDevice: ConnectedDevice,
    deviceConfiguration: Binding<SettingsConfiguration.Device>,
    showing: Binding<Bool>
  ) {
    self.connectedDevice = connectedDevice
    self._deviceConfiguration = deviceConfiguration
    self._showing = showing

    let device = deviceConfiguration.wrappedValue
    self._gamePadStickXFormula = State(initialValue: device.gamePadStickXFormula)
    self._gamePadStickYFormula = State(initialValue: device.gamePadStickYFormula)
    self._gamePadStickVerticalWheelFormula = State(
      initialValue: device.gamePadStickVerticalWheelFormula)
    self._gamePadStickHorizontalWheelFormula = State(
      initialValue: device.gamePadStickHorizontalWheelFormula)
  }

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .leading, spacing: 12.0) {
        Text(
          "\(connectedDevice.productName) (\(connectedDevice.manufacturerName))"
        )
        .padding(.leading, 40)
        .padding(.top, 20)

        TabView {
          XYStickTabView(
            deviceConfiguration: $deviceConfiguration,
            defaults: settings.configuration.deviceDefaults,
            xFormula: formulaBinding(
              .x, value: $gamePadStickXFormula, error: $gamePadStickXFormulaError),
            xFormulaError: $gamePadStickXFormulaError,
            resetXFormula: { resetFormula(.x) },
            yFormula: formulaBinding(
              .y, value: $gamePadStickYFormula, error: $gamePadStickYFormulaError),
            yFormulaError: $gamePadStickYFormulaError,
            resetYFormula: { resetFormula(.y) }
          )
          .padding()
          .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .top)
          .tabItem {
            Text("XY stick")
          }

          WheelsStickTabView(
            deviceConfiguration: $deviceConfiguration,
            defaults: settings.configuration.deviceDefaults,
            verticalWheelFormula: formulaBinding(
              .verticalWheel,
              value: $gamePadStickVerticalWheelFormula,
              error: $gamePadStickVerticalWheelFormulaError),
            verticalWheelFormulaError: $gamePadStickVerticalWheelFormulaError,
            resetVerticalWheelFormula: { resetFormula(.verticalWheel) },
            horizontalWheelFormula: formulaBinding(
              .horizontalWheel,
              value: $gamePadStickHorizontalWheelFormula,
              error: $gamePadStickHorizontalWheelFormulaError),
            horizontalWheelFormulaError: $gamePadStickHorizontalWheelFormulaError,
            resetHorizontalWheelFormula: { resetFormula(.horizontalWheel) }
          )
          .padding()
          .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .top)
          .tabItem {
            Text("Wheels stick")
          }

          OthersTabView(deviceConfiguration: $deviceConfiguration)
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
    // Keep valid formula fields synchronized with configuration reloads, including changes made
    // in an external editor. Preserve an invalid local draft so it can still be corrected.
    .onChange(of: deviceConfiguration.gamePadStickXFormula) { value in
      if !gamePadStickXFormulaError && gamePadStickXFormula != value {
        gamePadStickXFormula = value
      }
    }
    .onChange(of: deviceConfiguration.gamePadStickYFormula) { value in
      if !gamePadStickYFormulaError && gamePadStickYFormula != value {
        gamePadStickYFormula = value
      }
    }
    .onChange(of: deviceConfiguration.gamePadStickVerticalWheelFormula) { value in
      if !gamePadStickVerticalWheelFormulaError && gamePadStickVerticalWheelFormula != value {
        gamePadStickVerticalWheelFormula = value
      }
    }
    .onChange(of: deviceConfiguration.gamePadStickHorizontalWheelFormula) { value in
      if !gamePadStickHorizontalWheelFormulaError && gamePadStickHorizontalWheelFormula != value {
        gamePadStickHorizontalWheelFormula = value
      }
    }
  }

  private func formulaBinding(
    _ formula: Settings.GamePadStickFormula,
    value: Binding<String>,
    error: Binding<Bool>
  ) -> Binding<String> {
    Binding(
      get: { value.wrappedValue },
      set: { newValue in
        value.wrappedValue = newValue
        error.wrappedValue = !settings.setGamePadStickFormula(
          formula,
          value: newValue,
          connectedDevice: connectedDevice)
      })
  }

  private func resetFormula(_ formula: Settings.GamePadStickFormula) {
    settings.resetGamePadStickFormula(formula, connectedDevice: connectedDevice)

    guard let device = settings.deviceConfiguration(connectedDevice) else { return }
    switch formula {
    case .x:
      gamePadStickXFormula = device.gamePadStickXFormula
      gamePadStickXFormulaError = false
    case .y:
      gamePadStickYFormula = device.gamePadStickYFormula
      gamePadStickYFormulaError = false
    case .verticalWheel:
      gamePadStickVerticalWheelFormula = device.gamePadStickVerticalWheelFormula
      gamePadStickVerticalWheelFormulaError = false
    case .horizontalWheel:
      gamePadStickHorizontalWheelFormula = device.gamePadStickHorizontalWheelFormula
      gamePadStickHorizontalWheelFormulaError = false
    }
  }

  struct XYStickTabView: View {
    @Binding var deviceConfiguration: SettingsConfiguration.Device
    let defaults: SettingsConfiguration.DeviceDefaults
    @Binding var xFormula: String
    @Binding var xFormulaError: Bool
    let resetXFormula: () -> Void
    @Binding var yFormula: String
    @Binding var yFormulaError: Bool
    let resetYFormula: () -> Void

    var body: some View {
      VStack(alignment: .leading) {
        StickParametersView(
          deadzone: $deviceConfiguration.gamePadXyStickDeadzone,
          deadzoneDefaultValue: defaults.gamePadXyStickDeadzone,

          deltaMagnitudeDetectionThreshold: $deviceConfiguration
            .gamePadXyStickDeltaMagnitudeDetectionThreshold,
          deltaMagnitudeDetectionThresholdDefaultValue:
            defaults.gamePadXyStickDeltaMagnitudeDetectionThreshold,

          continuedMovementAbsoluteMagnitudeThreshold: $deviceConfiguration
            .gamePadXyStickContinuedMovementAbsoluteMagnitudeThreshold,
          continuedMovementAbsoluteMagnitudeThresholdDefaultValue:
            defaults.gamePadXyStickContinuedMovementAbsoluteMagnitudeThreshold,

          continuedMovementIntervalMilliseconds: $deviceConfiguration
            .gamePadXyStickContinuedMovementIntervalMilliseconds,
          continuedMovementIntervalMillisecondsDefaultValue:
            defaults.gamePadXyStickContinuedMovementIntervalMilliseconds
        )

        HStack(spacing: 20.0) {
          FormulaView(
            name: "X formula",
            value: $xFormula,
            error: $xFormulaError,
            resetFunction: resetXFormula
          )

          FormulaView(
            name: "Y formula",
            value: $yFormula,
            error: $yFormulaError,
            resetFunction: resetYFormula
          )
        }
        .padding(.top, 20.0)
      }
    }
  }

  struct WheelsStickTabView: View {
    @Binding var deviceConfiguration: SettingsConfiguration.Device
    let defaults: SettingsConfiguration.DeviceDefaults
    @Binding var verticalWheelFormula: String
    @Binding var verticalWheelFormulaError: Bool
    let resetVerticalWheelFormula: () -> Void
    @Binding var horizontalWheelFormula: String
    @Binding var horizontalWheelFormulaError: Bool
    let resetHorizontalWheelFormula: () -> Void

    var body: some View {
      VStack(alignment: .leading) {
        StickParametersView(
          deadzone: $deviceConfiguration.gamePadWheelsStickDeadzone,
          deadzoneDefaultValue: defaults.gamePadWheelsStickDeadzone,

          deltaMagnitudeDetectionThreshold: $deviceConfiguration
            .gamePadWheelsStickDeltaMagnitudeDetectionThreshold,
          deltaMagnitudeDetectionThresholdDefaultValue:
            defaults.gamePadWheelsStickDeltaMagnitudeDetectionThreshold,

          continuedMovementAbsoluteMagnitudeThreshold: $deviceConfiguration
            .gamePadWheelsStickContinuedMovementAbsoluteMagnitudeThreshold,
          continuedMovementAbsoluteMagnitudeThresholdDefaultValue:
            defaults.gamePadWheelsStickContinuedMovementAbsoluteMagnitudeThreshold,

          continuedMovementIntervalMilliseconds: $deviceConfiguration
            .gamePadWheelsStickContinuedMovementIntervalMilliseconds,
          continuedMovementIntervalMillisecondsDefaultValue:
            defaults.gamePadWheelsStickContinuedMovementIntervalMilliseconds
        )

        HStack(spacing: 20.0) {
          FormulaView(
            name: "vertical wheel formula",
            value: $verticalWheelFormula,
            error: $verticalWheelFormulaError,
            resetFunction: resetVerticalWheelFormula
          )

          FormulaView(
            name: "horizontal wheel formula",
            value: $horizontalWheelFormula,
            error: $horizontalWheelFormulaError,
            resetFunction: resetHorizontalWheelFormula
          )
        }
        .padding(.top, 20.0)
      }
    }
  }

  struct OthersTabView: View {
    @Binding var deviceConfiguration: SettingsConfiguration.Device

    var body: some View {
      VStack(alignment: .leading, spacing: 40.0) {
        Toggle(isOn: $deviceConfiguration.gamePadSwapSticks) {
          Text("Swap gamepad XY and wheels sticks")
        }
        .switchToggleStyle(controlSize: .mini, font: .callout)

        DevicesMouseFlagsView(deviceConfiguration: $deviceConfiguration)
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
