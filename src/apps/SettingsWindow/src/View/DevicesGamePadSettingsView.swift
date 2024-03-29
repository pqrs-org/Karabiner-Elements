import SwiftUI

struct DevicesGamePadSettingsView: View {
  let connectedDevice: LibKrbn.ConnectedDevice
  @Binding var showing: Bool
  @ObservedObject private var settings = LibKrbn.Settings.shared
  @State var connectedDeviceSetting: LibKrbn.ConnectedDeviceSetting?

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .leading, spacing: 12.0) {
        if let s = connectedDeviceSetting {
          let binding = Binding {
            s
          } set: {
            connectedDeviceSetting = $0
          }
          Text("\(connectedDevice.productName) (\(connectedDevice.manufacturerName))")
            .padding(.leading, 40)
            .padding(.top, 20)

          TabView {
            XYStickTabView(connectedDeviceSetting: binding)
              .padding()
              .tabItem {
                Text("XY stick")
              }

            WheelsStickTabView(connectedDeviceSetting: binding)
              .padding()
              .tabItem {
                Text("Wheels stick")
              }

            OthersTabView(connectedDeviceSetting: binding)
              .padding()
              .tabItem {
                Text("Others")
              }
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
    .onAppear {
      setConnectedDeviceSetting()
    }
    .onChange(of: settings.connectedDeviceSettings) { _ in
      setConnectedDeviceSetting()
    }
  }

  private func setConnectedDeviceSetting() {
    connectedDeviceSetting = settings.findConnectedDeviceSetting(connectedDevice)
  }

  struct XYStickTabView: View {
    @Binding var connectedDeviceSetting: LibKrbn.ConnectedDeviceSetting

    var body: some View {
      VStack(alignment: .leading) {
        StickContinuedMovementAbsoluteMagnitudeThresholdView(
          value: $connectedDeviceSetting
            .gamePadXYStickContinuedMovementAbsoluteMagnitudeThreshold)

        StickContinuedMovementIntervalMillisecondsView(
          defaultValue: Int(
            libkrbn_core_configuration_game_pad_xy_stick_continued_movement_interval_milliseconds_default_value()
          ),
          value: $connectedDeviceSetting.gamePadXYStickContinuedMovementIntervalMilliseconds
        )

        StickFlickingInputWindowMillisecondsView(
          defaultValue: Int(
            libkrbn_core_configuration_game_pad_xy_stick_flicking_input_window_milliseconds_default_value()
          ),
          value: $connectedDeviceSetting.gamePadXYStickFlickingInputWindowMilliseconds
        )

        HStack(spacing: 20.0) {
          FormulaView(
            name: "X formula",
            value: $connectedDeviceSetting.gamePadStickXFormula
          )

          FormulaView(
            name: "Y formula",
            value: $connectedDeviceSetting.gamePadStickYFormula
          )
        }
        .padding(.top, 20.0)

        Spacer()
      }
    }
  }

  struct WheelsStickTabView: View {
    @Binding var connectedDeviceSetting: LibKrbn.ConnectedDeviceSetting

    var body: some View {
      VStack(alignment: .leading) {
        StickContinuedMovementAbsoluteMagnitudeThresholdView(
          value: $connectedDeviceSetting
            .gamePadWheelsStickContinuedMovementAbsoluteMagnitudeThreshold)

        StickContinuedMovementIntervalMillisecondsView(
          defaultValue: Int(
            libkrbn_core_configuration_game_pad_wheels_stick_continued_movement_interval_milliseconds_default_value()
          ),
          value: $connectedDeviceSetting.gamePadWheelsStickContinuedMovementIntervalMilliseconds
        )

        StickFlickingInputWindowMillisecondsView(
          defaultValue: Int(
            libkrbn_core_configuration_game_pad_wheels_stick_flicking_input_window_milliseconds_default_value()
          ),
          value: $connectedDeviceSetting.gamePadWheelsStickFlickingInputWindowMilliseconds
        )

        HStack(spacing: 20.0) {
          FormulaView(
            name: "vertical wheel formula",
            value: $connectedDeviceSetting.gamePadStickVerticalWheelFormula
          )

          FormulaView(
            name: "horizontal wheel formula",
            value: $connectedDeviceSetting.gamePadStickHorizontalWheelFormula
          )
        }
        .padding(.top, 20.0)

        Spacer()
      }
    }
  }

  struct OthersTabView: View {
    @Binding var connectedDeviceSetting: LibKrbn.ConnectedDeviceSetting

    var body: some View {
      VStack(alignment: .leading) {
        HStack {
          Toggle(isOn: $connectedDeviceSetting.gamePadSwapSticks) {
            Text("Swap gamepad XY and wheels sticks")
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          Spacer()
        }

        Spacer()
      }
    }
  }

  struct StickContinuedMovementAbsoluteMagnitudeThresholdView: View {
    @Binding var value: LibKrbn.OptionalSettingValue<Double>

    var body: some View {
      HStack {
        Toggle(
          isOn: $value.overwrite
        ) {
          Text("Overwrite continued movement absolute magnitude threshold:")
            .frame(maxWidth: .infinity, alignment: .leading)
        }
        .switchToggleStyle(controlSize: .mini, font: .callout)
        .frame(width: 400.0)

        HStack(alignment: .center, spacing: 8.0) {
          DoubleTextField(
            value: $value.value,
            range: 0...1,
            step: 0.1,
            maximumFractionDigits: 2,
            width: 60)

          Text("(Default: 1.0)")
        }
        .padding(.leading, 20)
        .disabled(!value.overwrite)
      }
    }
  }

  struct StickContinuedMovementIntervalMillisecondsView: View {
    let defaultValue: Int
    @Binding var value: LibKrbn.OptionalSettingValue<Int>

    var body: some View {
      HStack {
        Toggle(
          isOn: $value.overwrite
        ) {
          Text("Overwrite continued movement interval milliseconds:")
            .frame(maxWidth: .infinity, alignment: .leading)
        }
        .switchToggleStyle(controlSize: .mini, font: .callout)
        .frame(width: 400.0)

        HStack(alignment: .center, spacing: 8.0) {
          IntTextField(
            value: $value.value,
            range: 0...1000,
            step: 1,
            width: 60)

          Text(
            "(Default: \(defaultValue))"
          )
        }
        .padding(.leading, 20)
        .disabled(!value.overwrite)
      }
    }
  }

  struct StickFlickingInputWindowMillisecondsView: View {
    let defaultValue: Int
    @Binding var value: LibKrbn.OptionalSettingValue<Int>

    var body: some View {
      HStack {
        Toggle(
          isOn: $value.overwrite
        ) {
          Text("Overwrite flicking input window milliseconds:")
            .frame(maxWidth: .infinity, alignment: .leading)
        }
        .switchToggleStyle(controlSize: .mini, font: .callout)
        .frame(width: 400.0)

        HStack(alignment: .center, spacing: 8.0) {
          IntTextField(
            value: $value.value,
            range: 0...1000,
            step: 1,
            width: 60)

          Text(
            "(Default: \(defaultValue))"
          )
        }
        .padding(.leading, 20)
        .disabled(!value.overwrite)
      }
    }
  }

  struct FormulaView: View {
    let name: String
    @Binding var value: LibKrbn.OptionalSettingValue<String>
    @State private var text = ""
    @State private var error = false

    init(
      name: String,
      value: Binding<LibKrbn.OptionalSettingValue<String>>
    ) {
      self.name = name
      _value = value
      _text = State(initialValue: value.value.wrappedValue)
    }

    var body: some View {
      VStack {
        HStack {
          Toggle(
            isOn: $value.overwrite
          ) {
            Text("Overwrite \(name) formula:")
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          if error {
            Text("invalid formula")
              .foregroundColor(Color.errorForeground)
              .background(Color.errorBackground)
          }

          Spacer()
        }

        TextEditor(text: $text)
          .frame(height: 250.0)
          .disabled(!value.overwrite)
          .if(!value.overwrite) {
            $0.foregroundColor(.gray)
          }
      }
      .onChange(of: text) { newText in
        update(byText: newText)
      }
      .onChange(of: value) { newValue in
        update(byValue: newValue)
      }
    }

    private func update(byValue newValue: LibKrbn.OptionalSettingValue<String>) {
      error = false

      Task { @MainActor in
        if text != newValue.value {
          text = newValue.value
        }
      }
    }

    private func update(byText newText: String) {
      if libkrbn_core_configuration_game_pad_validate_stick_formula(newText.cString(using: .utf8)) {
        error = false

        Task { @MainActor in
          if value.value != newText {
            value.value = newText
          }
        }
      } else {
        error = true
      }
    }
  }
}

struct DevicesGamePadSettingsView_Previews: PreviewProvider {
  @State static var connectedDevice = LibKrbn.ConnectedDevice(
    index: 0,
    manufacturerName: "",
    productName: "",
    transport: "",
    vendorId: 0,
    productId: 0,
    deviceAddress: "",
    isKeyboard: false,
    isPointingDevice: false,
    isGamePad: true,
    isBuiltInKeyboard: false,
    isBuiltInTrackpad: false,
    isBuiltInTouchBar: false,
    isAppleDevice: false
  )
  @State static var showing = true

  static var previews: some View {
    DevicesGamePadSettingsView(
      connectedDevice: connectedDevice,
      showing: $showing)
  }
}
