import SwiftUI

struct DevicesMouseFlagsView: View {
  @Binding var deviceConfiguration: SettingsConfiguration.Device

  var body: some View {
    GroupBox(label: Text("Mouse Flags")) {
      HStack(alignment: .top, spacing: 60.0) {
        VStack(alignment: .leading, spacing: 6.0) {
          Toggle(isOn: $deviceConfiguration.mouseFlipX) {
            Text("Flip mouse X")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          Toggle(isOn: $deviceConfiguration.mouseFlipY) {
            Text("Flip mouse Y")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          Toggle(isOn: $deviceConfiguration.mouseFlipVerticalWheel) {
            Text("Flip mouse vertical wheel")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          Toggle(isOn: $deviceConfiguration.mouseFlipHorizontalWheel) {
            Text("Flip mouse horizontal wheel")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)
        }
        .frame(width: 200.0)

        VStack(alignment: .leading, spacing: 6.0) {
          Toggle(isOn: $deviceConfiguration.mouseDiscardX) {
            Text("Discard mouse X")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          Toggle(isOn: $deviceConfiguration.mouseDiscardY) {
            Text("Discard mouse Y")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          Toggle(isOn: $deviceConfiguration.mouseDiscardVerticalWheel) {
            Text("Discard mouse vertical wheel")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          Toggle(isOn: $deviceConfiguration.mouseDiscardHorizontalWheel) {
            Text("Discard mouse horizontal wheel")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)
        }
        .frame(width: 220.0)

        VStack(alignment: .leading, spacing: 6.0) {
          Toggle(isOn: $deviceConfiguration.mouseSwapXy) {
            Text("Swap mouse X and Y")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          Toggle(isOn: $deviceConfiguration.mouseSwapWheels) {
            Text("Swap mouse wheels")
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
