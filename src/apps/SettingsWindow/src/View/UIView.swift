import SwiftUI

struct UIView: View {
  @ObservedObject private var appIcons = AppIcons.shared
  @ObservedObject private var settings = LibKrbn.Settings.shared

  var body: some View {
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
        .padding(6.0)
      }

      GroupBox(label: Text("Sticky modifier keys")) {
        VStack(alignment: .leading, spacing: 12.0) {
          HStack {
            Toggle(isOn: $settings.virtualHIDKeyboardIndicateStickyModifierKeysState) {
              Text("Indicate sticky modifier keys state (Default: on)")
            }
            .switchToggleStyle()

            Spacer()
          }
        }
        .padding(6.0)
      }

      GroupBox(label: Text("App icon")) {
        VStack(alignment: .leading, spacing: 12.0) {
          VStack {
            Label(
              "The app icon changes may take several seconds or require reopening the application.",
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
        .padding(6.0)
        .background(Color(NSColor.textBackgroundColor))
      }

      Spacer()
    }
    .padding()
  }
}

struct UIView_Previews: PreviewProvider {
  static var previews: some View {
    UIView()
  }
}
