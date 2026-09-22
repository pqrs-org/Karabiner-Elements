import SwiftUI

struct DevicesMouseFlagsView: View {
  @Binding var deviceConfiguration: SettingsConfiguration.Device

  var body: some View {
    GroupBox(label: AppLocalizedText("settings.devices.mouse.flags")) {
      HStack(alignment: .top, spacing: 60.0) {
        VStack(alignment: .leading, spacing: 6.0) {
          Toggle(isOn: $deviceConfiguration.mouseFlipX) {
            AppLocalizedText("settings.devices.mouse.flip_x")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          Toggle(isOn: $deviceConfiguration.mouseFlipY) {
            AppLocalizedText("settings.devices.mouse.flip_y")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          Toggle(isOn: $deviceConfiguration.mouseFlipVerticalWheel) {
            AppLocalizedText("settings.devices.mouse.flip_vertical_wheel")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          Toggle(isOn: $deviceConfiguration.mouseFlipHorizontalWheel) {
            AppLocalizedText("settings.devices.mouse.flip_horizontal_wheel")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)
        }
        .frame(width: 200.0)

        VStack(alignment: .leading, spacing: 6.0) {
          Toggle(isOn: $deviceConfiguration.mouseDiscardX) {
            AppLocalizedText("settings.devices.mouse.discard_x")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          Toggle(isOn: $deviceConfiguration.mouseDiscardY) {
            AppLocalizedText("settings.devices.mouse.discard_y")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          Toggle(isOn: $deviceConfiguration.mouseDiscardVerticalWheel) {
            AppLocalizedText("settings.devices.mouse.discard_vertical_wheel")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          Toggle(isOn: $deviceConfiguration.mouseDiscardHorizontalWheel) {
            AppLocalizedText("settings.devices.mouse.discard_horizontal_wheel")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)
        }
        .frame(width: 220.0)

        VStack(alignment: .leading, spacing: 6.0) {
          Toggle(isOn: $deviceConfiguration.mouseSwapXy) {
            AppLocalizedText("settings.devices.mouse.swap_xy")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          Toggle(isOn: $deviceConfiguration.mouseSwapWheels) {
            AppLocalizedText("settings.devices.mouse.swap_wheels")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          Toggle(isOn: $deviceConfiguration.mouseHorizontalWheelToButtons) {
            Text("Treat mouse horizontal wheel as buttons")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)
        }
        .frame(width: 220.0)
      }
      .padding()
    }
  }
}
