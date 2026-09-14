import SwiftUI

struct DevicesMouseFlagsView: View {
  @Binding var deviceConfiguration: SettingsConfiguration.Device

  var body: some View {
    GroupBox(label: AppLocalizedText("settings.mouse.flags")) {
      HStack(alignment: .top, spacing: 60.0) {
        VStack(alignment: .leading, spacing: 6.0) {
          Toggle(isOn: $deviceConfiguration.mouseFlipX) {
            AppLocalizedText("settings.mouse.flip_x")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          Toggle(isOn: $deviceConfiguration.mouseFlipY) {
            AppLocalizedText("settings.mouse.flip_y")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          Toggle(isOn: $deviceConfiguration.mouseFlipVerticalWheel) {
            AppLocalizedText("settings.mouse.flip_vertical_wheel")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          Toggle(isOn: $deviceConfiguration.mouseFlipHorizontalWheel) {
            AppLocalizedText("settings.mouse.flip_horizontal_wheel")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)
        }
        .frame(width: 200.0)

        VStack(alignment: .leading, spacing: 6.0) {
          Toggle(isOn: $deviceConfiguration.mouseDiscardX) {
            AppLocalizedText("settings.mouse.discard_x")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          Toggle(isOn: $deviceConfiguration.mouseDiscardY) {
            AppLocalizedText("settings.mouse.discard_y")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          Toggle(isOn: $deviceConfiguration.mouseDiscardVerticalWheel) {
            AppLocalizedText("settings.mouse.discard_vertical_wheel")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          Toggle(isOn: $deviceConfiguration.mouseDiscardHorizontalWheel) {
            AppLocalizedText("settings.mouse.discard_horizontal_wheel")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)
        }
        .frame(width: 220.0)

        VStack(alignment: .leading, spacing: 6.0) {
          Toggle(isOn: $deviceConfiguration.mouseSwapXy) {
            AppLocalizedText("settings.mouse.swap_xy")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          Toggle(isOn: $deviceConfiguration.mouseSwapWheels) {
            AppLocalizedText("settings.mouse.swap_wheels")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)
        }
        .frame(width: 160.0)
      }
      .padding()
    }
  }
}
