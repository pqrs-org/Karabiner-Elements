import SwiftUI

struct UIView: View {
  @ObservedObject private var settings = LibKrbn.Settings.shared
  @ObservedObject private var appIcons = AppIcons.shared

  var body: some View {
    ScrollView {
      VStack(alignment: .leading, spacing: 24.0) {
        GroupBox(label: Text("Menu bar")) {
          VStack(alignment: .leading, spacing: 12.0) {
            HStack {
              Toggle(isOn: $settings.showIconInMenuBar) {
                Text("Show icon in menu bar (Default: on)")
              }
              .switchToggleStyle()

              Spacer()
            }

            HStack {
              Toggle(isOn: $settings.showProfileNameInMenuBar) {
                Text("Show profile name in menu bar (Default: off)")
              }
              .switchToggleStyle()

              Spacer()
            }

            HStack {
              Toggle(isOn: $settings.askForConfirmationBeforeQuitting) {
                Text("Ask for confirmation when quitting (Default: on)")
              }
              .switchToggleStyle()

              Spacer()
            }

          }
          .padding()
        }

        GroupBox(label: Text("Karabiner Notification Window")) {
          VStack(alignment: .leading, spacing: 12.0) {
            HStack {
              Toggle(isOn: $settings.enableNotificationWindow) {
                Text("Enable Karabiner Notification Window (Default: on)")
              }
              .switchToggleStyle()

              Spacer()
            }

            HStack {
              Toggle(isOn: $settings.virtualHIDKeyboardIndicateStickyModifierKeysState) {
                Text("Indicate sticky modifier keys state (Default: on)")
              }
              .switchToggleStyle()

              Spacer()
            }

            VStack(alignment: .leading, spacing: 12.0) {
              Label(
                "What is the Karabiner Notification Window?",
                systemImage: "lightbulb"
              )
              VStack(alignment: .leading, spacing: 0.0) {
                Text(
                  "Karabiner Notification Window is a window that displays messages, located at the bottom right of the screen. "
                )
                Text(
                  "It is used for temporary alerts, displaying the status of sticky modifiers, and showing messages for some complex modifications."
                )
              }

              Image(decorative: "notification-window")
                .resizable()
                .aspectRatio(contentMode: .fit)
                .frame(height: 150)

            }
            .padding()
            .foregroundColor(Color.warningForeground)
            .background(Color.warningBackground)
          }
          .padding()
        }

        GroupBox(label: Text("App icon")) {
          VStack(alignment: .leading, spacing: 12.0) {
            VStack {
              Label(
                "It takes a few seconds for changes to the application icon to take effect.\nAnd to update the Dock icon, you need to close and reopen the application.",
                systemImage: "lightbulb"
              )
              .padding()
              .foregroundColor(Color.warningForeground)
              .background(Color.warningBackground)
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

                  Spacer()
                }
                .padding(.vertical, 5.0)
                .overlay(
                  RoundedRectangle(cornerRadius: 8)
                    .stroke(
                      Color(NSColor.selectedControlColor),
                      lineWidth: appIcons.selectedAppIconNumber == appIcon.id ? 3 : 0
                    )
                )

                .tag(appIcon.id)
              }
            }.pickerStyle(RadioGroupPickerStyle())

            Divider()
          }
          .padding()
          .background(Color(NSColor.textBackgroundColor))
        }
      }
    }
    .padding()
  }
}

struct UIView_Previews: PreviewProvider {
  static var previews: some View {
    UIView()
  }
}
