import SwiftUI

struct DevicesMouseFlagsView: View {
  @ObservedObject var connectedDeviceSetting: LibKrbn.ConnectedDeviceSetting

  var body: some View {
    GroupBox(label: Text("Mouse Flags")) {
      HStack(alignment: .top, spacing: 60.0) {
        VStack(alignment: .leading, spacing: 6.0) {
          Toggle(isOn: $connectedDeviceSetting.mouseFlipX) {
            Text("Flip mouse X")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          Toggle(isOn: $connectedDeviceSetting.mouseFlipY) {
            Text("Flip mouse Y")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          Toggle(isOn: $connectedDeviceSetting.mouseFlipVerticalWheel) {
            Text("Flip mouse vertical wheel")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          Toggle(isOn: $connectedDeviceSetting.mouseFlipHorizontalWheel) {
            Text("Flip mouse horizontal wheel")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)
        }
        .frame(width: 200.0)

        VStack(alignment: .leading, spacing: 6.0) {
          Toggle(isOn: $connectedDeviceSetting.mouseDiscardX) {
            Text("Discard mouse X")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          Toggle(isOn: $connectedDeviceSetting.mouseDiscardY) {
            Text("Discard mouse Y")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          Toggle(isOn: $connectedDeviceSetting.mouseDiscardVerticalWheel) {
            Text("Discard mouse vertical wheel")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          Toggle(isOn: $connectedDeviceSetting.mouseDiscardHorizontalWheel) {
            Text("Discard mouse horizontal wheel")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)
        }
        .frame(width: 220.0)

        VStack(alignment: .leading, spacing: 6.0) {
          Toggle(isOn: $connectedDeviceSetting.mouseSwapXY) {
            Text("Swap mouse X and Y")
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)

          Toggle(isOn: $connectedDeviceSetting.mouseSwapWheels) {
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
