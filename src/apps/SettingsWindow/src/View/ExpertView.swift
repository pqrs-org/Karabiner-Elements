import SwiftUI

struct ExpertView: View {
  @ObservedObject private var settings = LibKrbn.Settings.shared

  var body: some View {
    ScrollView {
      VStack(alignment: .leading, spacing: 24.0) {
        GroupBox(label: Text("Expert mode")) {
          VStack(alignment: .leading, spacing: 4.0) {
            Toggle(isOn: $settings.unsafeUI) {
              Text("Enable unsafe configuration (Default: off)")
            }
            .switchToggleStyle()

            Label(
              "Unsafe configuration disables the foolproof feature on the configuration UI.\n"
                + "You should not enable unsafe configuration unless you are ready to stop Karabiner-Elements from remote machine. (e.g., using Screen Sharing)\n"
                + "\n" + "Unsafe configuration allows the following items:\n"
                + "- Allow you to enable Apple pointing devices in the Devices tab.\n"
                + "- Allow you to change left-click in Simple Modifications tab.",
              systemImage: WarningBorder.icon
            )
            .modifier(WarningBorder())
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }

        GroupBox(label: Text("Options")) {
          VStack(alignment: .leading, spacing: 20.0) {
            VStack(alignment: .leading, spacing: 4.0) {
              Toggle(isOn: $settings.filterUselessEventsFromSpecificDevices) {
                Text("Filter useless events from specific devices (Default: on)")
              }
              .switchToggleStyle()

              Label(
                "If this setting is enabled, the following events will be ignored:\n"
                  + "- Nintendo's Pro Controller (USB connected):\n"
                  + "    - Buttons since on/off events are continuously sent at high frequency even when nothing is pressed.\n"
                  + "    - Sticks since tilt events in random directions are continuously sent even when the stick is not moved at all.",
                systemImage: InfoBorder.icon
              )
              .modifier(InfoBorder())
              .fixedSize(horizontal: false, vertical: true)
            }

            VStack(alignment: .leading, spacing: 4.0) {
              Toggle(isOn: $settings.reorderSameTimestampInputEventsToPrioritizeModifiers) {
                Text("Reorder same timestamp input events to prioritize modifiers (Default: on)")
              }
              .switchToggleStyle()

              Label(
                "If your keyboard supports hardware macros and sends multiple keys at once, and you notice that modifier key order is changing, try turning this setting off.",
                systemImage: InfoBorder.icon
              )
              .modifier(InfoBorder())
            }
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }

        GroupBox(label: Text("Delay to grab device")) {
          VStack(alignment: .leading, spacing: 4.0) {
            HStack {
              IntTextField(
                value: $settings.delayMillisecondsBeforeOpenDevice,
                range: 0...10000,
                step: 100,
                width: 50)

              Text("milliseconds (Default value is 1000)")
            }

            Label(
              "Setting insufficient delay (e.g., 0) will result in a device becoming unusable after Karabiner-Elements is quit.\n"
                + "(This is a macOS problem and can be solved by unplugging the device and plugging it again.)",
              systemImage: WarningBorder.icon
            )
            .modifier(WarningBorder())
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }
      }
      .padding()
    }
  }
}
