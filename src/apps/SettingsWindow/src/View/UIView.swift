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

            if settings.configuration.globalConfiguration.enableNotificationWindow {
              Toggle(
                isOn: $settings.configuration.selectedProfile.virtualHidKeyboard
                  .indicateStickyModifierKeysState
              ) {
                Text("Indicate sticky modifier keys state (Default: on)")
              }
              .switchToggleStyle()

              GroupBox(label: Text("Appearance")) {
                VStack(alignment: .leading, spacing: 12.0) {
                  HStack(spacing: 24.0) {
                    Picker(
                      selection: $settings.configuration.globalConfiguration
                        .notificationWindowPosition,
                      label: Text("Position")
                    ) {
                      Text("Top left").tag(
                        SettingsConfiguration.GlobalConfiguration.NotificationWindowPosition.topLeft
                      )
                      Text("Top right").tag(
                        SettingsConfiguration.GlobalConfiguration.NotificationWindowPosition
                          .topRight)
                      Text("Bottom left").tag(
                        SettingsConfiguration.GlobalConfiguration.NotificationWindowPosition
                          .bottomLeft)
                      Text("Bottom right").tag(
                        SettingsConfiguration.GlobalConfiguration.NotificationWindowPosition
                          .bottomRight
                      )
                    }
                    .pickerStyle(.menu)
                    .fixedSize()

                    Toggle(
                      isOn: $settings.configuration.globalConfiguration
                        .notificationWindowRespectScreenVisibleFrame
                    ) {
                      Text("Keep the window outside the Dock area (Default: on)")
                    }
                    .switchToggleStyle()
                  }

                  Toggle(
                    isOn: $settings.configuration.globalConfiguration.notificationWindowShowIcon
                  ) {
                    Text("Show application icon (Default: on)")
                  }
                  .switchToggleStyle()

                  HStack {
                    Text("Font size:")
                    IntTextField(
                      value: $settings.configuration.globalConfiguration.notificationWindowFontSize,
                      range: 8...64,
                      step: 1,
                      width: 40)
                    Text("pt (Default: 13)")
                  }

                  Grid(alignment: .leading, horizontalSpacing: 12.0, verticalSpacing: 12.0) {
                    notificationWindowColorSettings(
                      title: "Light appearance",
                      background: $settings.configuration.globalConfiguration
                        .notificationWindowColors
                        .light.backgroundColor,
                      text: $settings.configuration.globalConfiguration.notificationWindowColors
                        .light
                        .textColor,
                      backgroundSystemColor: resolvedSystemColor(
                        .windowBackgroundColor,
                        appearance: .aqua),
                      textSystemColor: resolvedSystemColor(.labelColor, appearance: .aqua))
                    notificationWindowColorSettings(
                      title: "Dark appearance",
                      background: $settings.configuration.globalConfiguration
                        .notificationWindowColors
                        .dark.backgroundColor,
                      text: $settings.configuration.globalConfiguration.notificationWindowColors
                        .dark
                        .textColor,
                      backgroundSystemColor: resolvedSystemColor(
                        .windowBackgroundColor,
                        appearance: .darkAqua),
                      textSystemColor: resolvedSystemColor(.labelColor, appearance: .darkAqua))
                  }
                }
                .padding()
                .frame(maxWidth: .infinity, alignment: .leading)
              }
            }

            VStack(alignment: .leading, spacing: 12.0) {
              Label(
                "What is the Karabiner Notification Window?\n\n"
                  + "The Karabiner Notification Window displays the status of sticky modifiers and messages from Complex Modifications.",
                systemImage: InfoBorder.icon
              )

              Image(decorative: "notification-window")
                .resizable()
                .scaledToFit()
                .frame(height: 50)
                .clipShape(RoundedRectangle(cornerRadius: 12))
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
                      .scaledToFit()
                      .frame(width: 64.0, height: 64.0)
                  }

                  if let image = appIcon.eventViewerThumbnailImage {
                    Image(nsImage: image)
                      .resizable()
                      .scaledToFit()
                      .frame(width: 64.0, height: 64.0)
                  }

                  if let image = appIcon.multitouchExtensionThumbnailImage {
                    Image(nsImage: image)
                      .resizable()
                      .scaledToFit()
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

  private func notificationWindowColorSettings(
    title: String,
    background: Binding<String>,
    text: Binding<String>,
    backgroundSystemColor: NSColor,
    textSystemColor: NSColor
  ) -> some View {
    GridRow(alignment: .center) {
      Text(title)
        .bold()
        .fixedSize(horizontal: true, vertical: false)
      notificationWindowColorPicker(
        title: "Background color",
        value: background,
        systemColor: backgroundSystemColor)
      notificationWindowColorPicker(
        title: "Text color",
        value: text,
        systemColor: textSystemColor)
    }
  }

  private func resolvedSystemColor(
    _ color: NSColor,
    appearance name: NSAppearance.Name
  ) -> NSColor {
    guard let appearance = NSAppearance(named: name) else {
      return color
    }

    var resolvedColor = color
    appearance.performAsCurrentDrawingAppearance {
      resolvedColor = color.usingColorSpace(.sRGB) ?? color
    }
    return resolvedColor
  }

  private func notificationWindowColorPicker(
    title: String,
    value: Binding<String>,
    systemColor: NSColor
  ) -> some View {
    HStack {
      ColorPicker(
        title,
        selection: Binding(
          get: {
            value.wrappedValue == "system"
              ? Color(nsColor: systemColor)
              : Color(colorString: value.wrappedValue)
          },
          set: { color in
            value.wrappedValue = NSColor(color).notificationWindowColorString ?? "system"
          }),
        supportsOpacity: true)

      Button {
        value.wrappedValue = "system"
      } label: {
        Image(systemName: "arrow.counterclockwise")
      }
      .buttonStyle(.borderless)
      .disabled(value.wrappedValue == "system")
      .help("Use the system color")
    }
  }
}

extension NSColor {
  fileprivate var notificationWindowColorString: String? {
    guard let color = usingColorSpace(.sRGB) else { return nil }

    let red = Int((color.redComponent * 255.0).rounded())
    let green = Int((color.greenComponent * 255.0).rounded())
    let blue = Int((color.blueComponent * 255.0).rounded())
    let alpha = Int((color.alphaComponent * 255.0).rounded())

    return String(format: "#%02x%02x%02x%02x", red, green, blue, alpha)
  }
}
