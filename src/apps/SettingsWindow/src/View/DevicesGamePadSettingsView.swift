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

          HStack {
            VStack(alignment: .leading, spacing: 0.0) {
              Text("Note:")
              Text(
                "These parameters are still under development and may change in the next release."
              )
            }

            Spacer()
          }
          .padding()
          .foregroundColor(Color.warningForeground)
          .background(Color.warningBackground)

          GroupBox(label: Text("XY Stick")) {
            VStack(alignment: .leading) {
              StickContinuedMovementAbsoluteMagnitudeThresholdView(
                stickName: "XY",
                value: binding.gamePadXYStickContinuedMovementAbsoluteMagnitudeThreshold)

              StickContinuedMovementIntervalMillisecondsView(
                stickName: "XY",
                defaultValue: Int(
                  libkrbn_core_configuration_game_pad_xy_stick_continued_movement_interval_milliseconds_default_value()
                ),
                value: binding.gamePadXYStickContinuedMovementIntervalMilliseconds
              )

              StickFlickingInputWindowMillisecondsView(
                stickName: "XY",
                defaultValue: Int(
                  libkrbn_core_configuration_game_pad_xy_stick_flicking_input_window_milliseconds_default_value()
                ),
                value: binding.gamePadXYStickFlickingInputWindowMilliseconds
              )

              FormulaView(
                name: "X formula",
                defaultValue: String(
                  cString: libkrbn_core_configuration_game_pad_stick_x_formula_default_value()
                ),
                value: binding.gamePadStickXFormula
              )
            }.padding()
          }

          GroupBox(label: Text("Wheels Stick")) {
            VStack(alignment: .leading) {
              StickContinuedMovementAbsoluteMagnitudeThresholdView(
                stickName: "wheels",
                value: binding.gamePadWheelsStickContinuedMovementAbsoluteMagnitudeThreshold)

              StickContinuedMovementIntervalMillisecondsView(
                stickName: "wheels",
                defaultValue: Int(
                  libkrbn_core_configuration_game_pad_wheels_stick_continued_movement_interval_milliseconds_default_value()
                ),
                value: binding.gamePadWheelsStickContinuedMovementIntervalMilliseconds
              )

              StickFlickingInputWindowMillisecondsView(
                stickName: "wheels",
                defaultValue: Int(
                  libkrbn_core_configuration_game_pad_wheels_stick_flicking_input_window_milliseconds_default_value()
                ),
                value: binding.gamePadWheelsStickFlickingInputWindowMilliseconds
              )
            }.padding()
          }

          GroupBox(label: Text("Others")) {
            VStack(alignment: .leading) {
              Toggle(isOn: binding.gamePadSwapSticks) {
                Text("Swap gamepad XY and wheels sticks")
              }
              .switchToggleStyle(controlSize: .mini, font: .callout)
            }.padding()
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

  struct StickContinuedMovementAbsoluteMagnitudeThresholdView: View {
    let stickName: String
    @Binding var value: LibKrbn.OptionalSettingValue<Double>

    var body: some View {
      HStack {
        Toggle(
          isOn: $value.overwrite
        ) {
          Text("Overwrite \(stickName) stick continued movement absolute magnitude threshold:")
            .frame(maxWidth: .infinity, alignment: .leading)
        }
        .switchToggleStyle(controlSize: .mini, font: .callout)
        .frame(width: 480.0)

        HStack(alignment: .center, spacing: 8.0) {
          Text("Threshold:")
            .frame(width: 150.0, alignment: .trailing)

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
    let stickName: String
    let defaultValue: Int
    @Binding var value: LibKrbn.OptionalSettingValue<Int>

    var body: some View {
      HStack {
        Toggle(
          isOn: $value.overwrite
        ) {
          Text("Overwrite \(stickName) stick continued movement interval milliseconds:")
            .frame(maxWidth: .infinity, alignment: .leading)
        }
        .switchToggleStyle(controlSize: .mini, font: .callout)
        .frame(width: 480.0)

        HStack(alignment: .center, spacing: 8.0) {
          Text("Interval milliseconds:")
            .frame(width: 150.0, alignment: .trailing)

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
    let stickName: String
    let defaultValue: Int
    @Binding var value: LibKrbn.OptionalSettingValue<Int>

    var body: some View {
      HStack {
        Toggle(
          isOn: $value.overwrite
        ) {
          Text("Overwrite \(stickName) stick flicking input window milliseconds:")
            .frame(maxWidth: .infinity, alignment: .leading)
        }
        .switchToggleStyle(controlSize: .mini, font: .callout)
        .frame(width: 480.0)

        HStack(alignment: .center, spacing: 8.0) {
          Text("Milliseconds:")
            .frame(width: 150.0, alignment: .trailing)

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
    let defaultValue: String
    @Binding var value: LibKrbn.OptionalSettingValue<String>
    @State private var text = ""
    @State private var error = false

    init(
      name: String,
      defaultValue: String,
      value: Binding<LibKrbn.OptionalSettingValue<String>>
    ) {
      self.name = name
      self.defaultValue = defaultValue
      _value = value
      _text = State(initialValue: value.value.wrappedValue)
    }

    var body: some View {
      HStack {
        Toggle(
          isOn: $value.overwrite
        ) {
          Text("Overwrite \(name) formula:")
            .frame(maxWidth: .infinity, alignment: .leading)
        }
        .switchToggleStyle(controlSize: .mini, font: .callout)
        .frame(width: 480.0)

        VStack(alignment: .leading) {
          if error {
            Text("invalid formula")
              .foregroundColor(Color.errorForeground)
              .background(Color.errorBackground)
          }

          TextEditor(text: $text)
            .if(!value.overwrite) {
              $0
                .disabled(true)
                .foregroundColor(.gray)
            }
        }
        .padding(.leading, 20)
        .disabled(!value.overwrite)
      }
      .onChange(of: text) { newText in
        update(byText: newText)
      }
    }

    private func update(byText newText: String) {
      if libkrbn_core_configuration_game_pad_validate_stick_formula(newText.cString(using: .utf8)) {
        error = false

        Task { @MainActor in
          value.value = newText
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
    deviceAddress: nil,
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
