import SwiftUI

struct ProView: View {
  @ObservedObject private var settings = LibKrbn.Settings.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 24.0) {
      GroupBox(label: Text("Pro mode")) {
        VStack(alignment: .leading, spacing: 12.0) {
          HStack {
            Toggle(isOn: $settings.unsafeUI) {
              Text("Enable unsafe configuration (Default: off)")
            }
            .switchToggleStyle()

            Spacer()
          }

          VStack(alignment: .leading, spacing: 0.0) {
            Text("Warning:")
            Text("Unsafe configuration disables the foolproof feature on the configuration UI.")
            Text(
              "You should not enable unsafe configuration unless you are ready to stop Karabiner-Elements from remote machine. (e.g., using Screen Sharing)"
            )
            Text("")
            Text("Unsafe configuration allows the following items:")
            Text("- Allow you to enable Apple pointing devices in the Devices tab.")
            Text("- Allow you to change left-click in Simple Modifications tab.")
          }
          .padding()
          .foregroundColor(Color.errorForeground)
          .background(Color.errorBackground)
        }
        .padding(6.0)
      }

      GroupBox(label: Text("Delay to grab device")) {
        VStack(alignment: .leading, spacing: 12.0) {
          HStack {
            IntTextField(
              value: $settings.delayMillisecondsBeforeOpenDevice,
              range: 0...10000,
              step: 100,
              width: 50)

            Text("milliseconds (Default value is 1000)")
            Spacer()
          }

          HStack(alignment: .center, spacing: 12.0) {
            Image(systemName: "exclamationmark.triangle")

            VStack(alignment: .leading, spacing: 0.0) {
              Text(
                "Setting insufficient delay (e.g., 0) will result in a device becoming unusable after Karabiner-Elements is quit."
              )
              Text(
                "(This is a macOS problem and can be solved by unplugging the device and plugging it again.)"
              )
            }
          }
        }
        .padding(6.0)
      }

      Spacer()
    }
    .padding()
  }
}

struct ProView_Previews: PreviewProvider {
  static var previews: some View {
    ProView()
  }
}
