import SwiftUI

struct DevicesGamePadSettingsView: View {
  @AppLocalizationContext private var localized
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
          "\(connectedDevice.localizedProductName(localized)) (\(connectedDevice.localizedManufacturerName(localized)))"
        )
        .padding(.leading, 40.0)
        .padding(.top, 20.0)

        ScrollView {
          LazyVStack(alignment: .leading, spacing: 0, pinnedViews: [.sectionHeaders]) {
            Section {
              XYStickSettingsView(
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
              .padding(.top, 12.0)
              .padding(.bottom, 60.0)
            } header: {
              sectionHeader("settings.devices.gamepad.xy_stick")
            }

            Section {
              WheelsStickSettingsView(
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
              .padding(.top, 12.0)
              .padding(.bottom, 60.0)
            } header: {
              sectionHeader("settings.devices.gamepad.wheels_stick")
            }

            Section {
              OthersSettingsView(deviceConfiguration: $deviceConfiguration)
                .padding(.top, 12.0)
            } header: {
              sectionHeader("settings.devices.gamepad.others")
            }
          }
          .frame(maxWidth: .infinity, alignment: .leading)
          .padding()
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

  private func sectionHeader(_ key: String) -> some View {
    AppLocalizedText(key)
      .font(.title2)
      .sectionHeaderStyle()
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

  struct XYStickSettingsView: View {
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
            name: "settings.devices.gamepad.x_formula",
            value: $xFormula,
            defaultValue: defaults.gamePadStickXFormula,
            error: $xFormulaError,
            resetFunction: resetXFormula
          )

          FormulaView(
            name: "settings.devices.gamepad.y_formula",
            value: $yFormula,
            defaultValue: defaults.gamePadStickYFormula,
            error: $yFormulaError,
            resetFunction: resetYFormula
          )
        }
        .padding(.top, 20.0)
      }
    }
  }

  struct WheelsStickSettingsView: View {
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
            name: "settings.devices.gamepad.vertical_formula",
            value: $verticalWheelFormula,
            defaultValue: defaults.gamePadStickVerticalWheelFormula,
            error: $verticalWheelFormulaError,
            resetFunction: resetVerticalWheelFormula
          )

          FormulaView(
            name: "settings.devices.gamepad.horizontal_formula",
            value: $horizontalWheelFormula,
            defaultValue: defaults.gamePadStickHorizontalWheelFormula,
            error: $horizontalWheelFormulaError,
            resetFunction: resetHorizontalWheelFormula
          )
        }
        .padding(.top, 20.0)
      }
    }
  }

  struct OthersSettingsView: View {
    @Binding var deviceConfiguration: SettingsConfiguration.Device

    var body: some View {
      VStack(alignment: .leading, spacing: 12.0) {
        Toggle(isOn: $deviceConfiguration.gamePadSwapSticks) {
          AppLocalizedText("settings.devices.gamepad.swap_sticks")
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
          AppLocalizedText("settings.devices.gamepad.deadzone")
            .gridColumnAlignment(.trailing)

          DoubleTextField(
            value: $deadzone,
            range: 0...1,
            step: 0.01,
            maximumFractionDigits: 2,
            width: 60)

          AppLocalizedText(
            "settings.general.defaults.value",
            arguments: ["value": String(format: "%.2f", deadzoneDefaultValue)])
        }

        GridRow {
          AppLocalizedText("settings.devices.gamepad.delta_threshold")

          DoubleTextField(
            value: $deltaMagnitudeDetectionThreshold,
            range: 0...1,
            step: 0.01,
            maximumFractionDigits: 2,
            width: 60)

          AppLocalizedText(
            "settings.general.defaults.value",
            arguments: [
              "value": String(format: "%.2f", deltaMagnitudeDetectionThresholdDefaultValue)
            ])
        }

        GridRow {
          AppLocalizedText("settings.devices.gamepad.continued_threshold")

          DoubleTextField(
            value: $continuedMovementAbsoluteMagnitudeThreshold,
            range: 0...1,
            step: 0.1,
            maximumFractionDigits: 2,
            width: 60)

          AppLocalizedText(
            "settings.general.defaults.value",
            arguments: [
              "value": String(
                format: "%.2f", continuedMovementAbsoluteMagnitudeThresholdDefaultValue)
            ])
        }

        GridRow {
          AppLocalizedText("settings.devices.gamepad.continued_interval")

          IntTextField(
            value: $continuedMovementIntervalMilliseconds,
            range: 0...1000,
            step: 1,
            width: 60)

          AppLocalizedText(
            "settings.general.defaults.value",
            arguments: ["value": String(continuedMovementIntervalMillisecondsDefaultValue)])
        }
      }
    }
  }

  struct FormulaView: View {
    let name: String
    @Binding var value: String
    let defaultValue: String
    @Binding var error: Bool
    let resetFunction: () -> Void

    var body: some View {
      VStack {
        HStack {
          AppLocalizedText(name)

          if error {
            AppLocalizedLabel(
              "settings.devices.gamepad.invalid_formula",
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
              AppLocalizedConstrainedLabel(
                "settings.devices.gamepad.reset_formula", systemImage: "trash"
              )
              .buttonLabelStyle()
            }
          )
          .deleteButtonStyle()
          .disabled(value == defaultValue)
        }

        TextEditor(text: $value)
          .padding(8)
          .frame(height: 200.0)
          .background(Color(NSColor.textBackgroundColor))
          .border(Color(NSColor.separatorColor), width: 2)
      }
    }
  }
}
