import SwiftUI

struct UIView: View {
  @ObservedObject private var settings = Settings.shared
  @ObservedObject private var appIcons = AppIcons.shared

  var body: some View {
    ScrollView {
      VStack(alignment: .leading, spacing: 24.0) {
        GroupBox(label: Text("Menu bar")) {
          VStack(alignment: .leading, spacing: 12.0) {
            Toggle(isOn: $settings.configuration.globalConfiguration.showInMenuBar) {
              Text("Show icon in menu bar (Default: on)")
            }
            .switchToggleStyle()

            Toggle(isOn: $settings.configuration.globalConfiguration.showProfileNameInMenuBar) {
              Text("Show profile name in menu bar (Default: off)")
            }
            .switchToggleStyle()

            Toggle(isOn: $settings.configuration.globalConfiguration.showAdditionalMenuItems) {
              Text("Show additional menu items (Default: off)")
            }
            .switchToggleStyle()
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }

        GroupBox(label: Text("Karabiner Notification Window")) {
          VStack(alignment: .leading, spacing: 12.0) {
            Toggle(isOn: $settings.configuration.globalConfiguration.enableNotificationWindow) {
              Text("Enable Karabiner Notification Window (Default: on)")
            }
            .switchToggleStyle()

            Toggle(
              isOn: $settings.configuration.selectedProfile.virtualHidKeyboard
                .indicateStickyModifierKeysState
            ) {
              Text("Indicate sticky modifier keys state (Default: on)")
            }
            .switchToggleStyle()

            VStack(alignment: .leading, spacing: 12.0) {
              Label(
                "What is the Karabiner Notification Window?\n\n"
                  + "Karabiner Notification Window is a window that displays messages, located at the bottom right of the screen."
                  + "It is used for temporary alerts, displaying the status of sticky modifiers, and showing messages for some complex modifications.",
                systemImage: InfoBorder.icon
              )

              Image(decorative: "notification-window")
                .resizable()
                .aspectRatio(contentMode: .fit)
                .frame(height: 100)
                .background(
                  RoundedRectangle(cornerRadius: 9)
                    .stroke(
                      Color(NSColor.separatorColor),
                      lineWidth: 4
                    )
                )
            }
            .modifier(InfoBorder())
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }

        GroupBox(label: Text("App icon")) {
          VStack(alignment: .leading, spacing: 12.0) {
            VStack {
              Label(
                "It takes a few seconds for changes to the application icon to take effect.\nAnd to update the Dock icon, you need to close and reopen the application.",
                systemImage: InfoBorder.icon
              )
              .modifier(InfoBorder())
            }

            Picker(selection: $appIcons.selectedAppIconNumber, label: Text("")) {
              ForEach($appIcons.icons) { $appIcon in
                HStack {
                  if let image = appIcon.karabinerElementsThumbnailImage {
                    Image(nsImage: image)
                      .resizable()
                      .frame(width: 64.0, height: 64.0)
                  }

                  if let image = appIcon.eventViewerThumbnailImage {
                    Image(nsImage: image)
                      .resizable()
                      .frame(width: 64.0, height: 64.0)
                  }

                  if let image = appIcon.multitouchExtensionThumbnailImage {
                    Image(nsImage: image)
                      .resizable()
                      .frame(width: 64.0, height: 64.0)
                  }
                }
                .padding(5.0)
                .overlay(
                  RoundedRectangle(cornerRadius: 8)
                    .inset(by: -4)
                    .stroke(
                      Color(NSColor.selectedControlColor),
                      lineWidth: appIcons.selectedAppIconNumber == appIcon.id ? 3 : 0
                    )
                )

                .tag(appIcon.id)
              }
            }.pickerStyle(RadioGroupPickerStyle())
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }
      }
      .padding()
    }
  }
}
