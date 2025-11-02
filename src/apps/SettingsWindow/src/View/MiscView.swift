import SwiftUI

struct MiscView: View {
  @ObservedObject private var settings = LibKrbn.Settings.shared

  var body: some View {
    ScrollView {
      VStack(alignment: .leading, spacing: 24.0) {
        GroupBox(label: Text("Extra tool: Multitouch Extension")) {
          VStack(alignment: .leading, spacing: 12.0) {
            Toggle(isOn: $settings.enableMultitouchExtension) {
              Text("Enable Multitouch Extension (Default: off)")
            }
            .switchToggleStyle()

            Label(
              "This setting is hardware-specific. "
                + "When you import Karabiner-Elements settings to another Mac, "
                + "the enabled state of the Multitouch Extension is not carried over.",
              systemImage: InfoBorder.icon
            )
            .modifier(InfoBorder())
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }

        GroupBox(label: Text("Export & Import")) {
          VStack(alignment: .leading, spacing: 12.0) {
            Button(
              action: {
                var buffer = [Int8](repeating: 0, count: 32 * 1024)
                libkrbn_get_user_configuration_directory(&buffer, buffer.count)
                guard let path = String(utf8String: buffer) else { return }

                let url = URL(fileURLWithPath: path, isDirectory: true)
                NSWorkspace.shared.open(url)
              },
              label: {
                Label(
                  "Open config folder", systemImage: "arrow.up.forward.app")
              })

            Label(
              "You can back up your settings or migrate them to another machine by copying karabiner.json. "
                + "There are also backups under the automatic_backups folder, so you can restore a previous state by overwriting karabiner.json with one of those backups.",
              systemImage: InfoBorder.icon
            )
            .modifier(InfoBorder())
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }

        GroupBox(label: Text("System default configuration")) {
          VStack(alignment: .leading, spacing: 12.0) {
            Button(
              action: {
                settings.installSystemDefaultProfile()
              },
              label: {
                Label(
                  "Copy the current configuration to the system default configuration",
                  systemImage: "square.and.arrow.down")
              })

            Label(
              "You can use Karabiner-Elements even before login by setting the system default configuration. "
                + "(This operation requires the administrator privilege.)",
              systemImage: InfoBorder.icon
            )
            .modifier(InfoBorder())

            if settings.systemDefaultProfileExists {
              Button(
                role: .destructive,
                action: {
                  settings.removeSystemDefaultProfile()
                },
                label: {
                  Label("Remove the system default configuration", systemImage: "trash")
                    .buttonLabelStyle()
                }
              )
              .deleteButtonStyle()
            } else {
              Text("System default configuration is not set.").foregroundColor(
                Color.primary.opacity(0.5))
            }
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }
      }
      .padding()
    }
    .padding(.leading, 2)  // Prevent the header underline from disappearing in NavigationSplitView.
  }
}
