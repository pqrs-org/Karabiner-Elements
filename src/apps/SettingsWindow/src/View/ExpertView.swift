import SwiftUI

struct ExpertView: View {
  @ObservedObject private var settings = Settings.shared

  var body: some View {
    ScrollView {
      VStack(alignment: .leading, spacing: 24.0) {
        GroupBox(label: Text("Expert mode")) {
          VStack(alignment: .leading, spacing: 4.0) {
            Toggle(isOn: $settings.configuration.globalConfiguration.unsafeUi) {
              HStack {
                AppLocalizedText("setting.unsafe_ui")
                Text("(Default: off)")
              }
            }
            .switchToggleStyle()

            Label(
              "Unsafe configuration disables the safeguard feature on the configuration UI.\n"
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

        GroupBox(label: Text("Pointing devices")) {
          Toggle(
            isOn: $settings.configuration.selectedProfile
              .modifyPointingDeviceEventsByDefault
          ) {
            Text("Modify events for pointing devices by default (Default: off)")
          }
          .switchToggleStyle()
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }

        GroupBox(label: Text("CGEventTap fallback")) {
          VStack(alignment: .leading, spacing: 20.0) {
            VStack(alignment: .leading, spacing: 4.0) {
              Toggle(isOn: $settings.configuration.globalConfiguration.enableCgeventtapFallback) {
                HStack {
                  AppLocalizedText("setting.enable_cgeventtap_fallback")
                  Text("(Default: off)")
                }
              }
              .switchToggleStyle()

              Label(
                "If this setting is enabled, Karabiner-Core-Service uses CGEventTap as a fallback for devices that cannot be handled via HID input capture.",
                systemImage: InfoBorder.icon
              )
              .modifier(InfoBorder())
              .fixedSize(horizontal: false, vertical: true)
            }
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }

        GroupBox(label: Text("Options")) {
          VStack(alignment: .leading, spacing: 20.0) {
            VStack(alignment: .leading, spacing: 4.0) {
              Toggle(
                isOn: $settings.configuration.globalConfiguration
                  .filterUselessEventsFromSpecificDevices
              ) {
                HStack {
                  AppLocalizedText("setting.filter_useless_events_from_specific_devices")
                  Text("(Default: on)")
                }
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
              Toggle(
                isOn: $settings.configuration.globalConfiguration
                  .reorderSameTimestampInputEventsToPrioritizeModifiers
              ) {
                HStack {
                  AppLocalizedText(
                    "setting.reorder_same_timestamp_input_events_to_prioritize_modifiers")
                  Text("(Default: on)")
                }
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

        GroupBox(label: AppLocalizedText("setting.delay_milliseconds_before_open_device")) {
          VStack(alignment: .leading, spacing: 4.0) {
            HStack {
              IntTextField(
                value: $settings.configuration.selectedProfile.parameters
                  .delayMillisecondsBeforeOpenDevice,
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

        GroupBox(label: AppLocalizedText("setting.delay_milliseconds_before_sleep_shortcut")) {
          VStack(alignment: .leading, spacing: 4.0) {
            HStack {
              IntTextField(
                value: $settings.configuration.globalConfiguration
                  .delayMillisecondsBeforeSleepShortcut,
                range: 0...10000,
                step: 100,
                width: 50)

              Text("milliseconds (Default value is 500; 0 disables delay)")
            }

            Label(
              "Karabiner-Elements delays the key-down event of the macOS sleep shortcut (command+option+power, command+option+eject, or escape at the login window). This prevents the shortcut's release events from waking the Mac immediately after sleep.",
              systemImage: InfoBorder.icon
            )
            .modifier(InfoBorder())
            .fixedSize(horizontal: false, vertical: true)
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }
      }
      .padding()
    }
  }
}
