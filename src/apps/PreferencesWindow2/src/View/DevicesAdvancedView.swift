import SwiftUI

struct DevicesAdvancedView: View {
    @ObservedObject private var settings = Settings.shared

    var body: some View {
        VStack(alignment: .leading, spacing: 12.0) {
            GroupBox(label: Text("Delay before open device")) {
                VStack(alignment: .leading, spacing: 12.0) {
                    HStack {
                        IntTextField(value: $settings.delayMillisecondsBeforeOpenDevice,
                                     range: 0 ... 10000,
                                     step: 100,
                                     width: 50)

                        Text("milliseconds (Default value is 1000)")
                        Spacer()
                    }

                    HStack(alignment: .center, spacing: 12.0) {
                        Image(systemName: "exclamationmark.triangle")

                        VStack(alignment: .leading, spacing: 0.0) {
                            Text("Setting insufficient delay (e.g., 0) will result in a device becoming unusable after Karabiner-Elements is quit.")
                            Text("(This is a macOS problem and can be solved by unplugging the device and plugging it again.)")
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

struct DevicesAdvancediew_Previews: PreviewProvider {
    static var previews: some View {
        DevicesAdvancedView()
    }
}
