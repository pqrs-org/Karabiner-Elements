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
            Text("Note:")
            Text("Unsafe configuration disables the foolproof feature on the configuration UI.")
            Text(
              "You should not enable unsafe configuration unless you are ready to stop Karabiner-Elements from remote machine such as Screen Sharing."
            )
            Text("")
            Text("Unsafe configuration allows the following items:")
            Text("- Allow you to enable Apple pointing devices in the Devices tab.")
            Text("- Allow you to change left-click in Simple Modifications tab.")
          }
          .padding()
          .foregroundColor(Color.warningForeground)
          .background(Color.warningBackground)
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
