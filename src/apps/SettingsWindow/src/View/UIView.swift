import SwiftUI

struct UIView: View {
  @AppLocalizationContext private var localized
  @ObservedObject private var settings = Settings.shared
  @ObservedObject private var appIcons = AppIcons.shared

  private var defaults: SettingsConfiguration.Defaults {
    settings.configuration.defaultConfiguration
  }

  var body: some View {
    ScrollView {
      VStack(alignment: .leading, spacing: 24.0) {
        GroupBox(label: AppLocalizedText("settings.ui.menu_bar")) {
          VStack(alignment: .leading, spacing: 12.0) {
            Toggle(isOn: $settings.configuration.globalConfiguration.showInMenuBar) {
              AppLocalizedText([
                "settings.ui.show_in_menu_bar",
                " ",
                .init(
                  "settings.general.defaults.value",
                  arguments: [
                    "value": localized(
                      defaults.globalConfiguration.showInMenuBar
                        ? "shared.value.on" : "shared.value.off")
                  ]),
              ])
            }
            .switchToggleStyle()

            Toggle(isOn: $settings.configuration.globalConfiguration.showProfileNameInMenuBar) {
              AppLocalizedText([
                "settings.ui.show_profile_name_in_menu_bar",
                " ",
                .init(
                  "settings.general.defaults.value",
                  arguments: [
                    "value": localized(
                      defaults.globalConfiguration.showProfileNameInMenuBar
                        ? "shared.value.on" : "shared.value.off")
                  ]),
              ])
            }
            .switchToggleStyle()

            Toggle(isOn: $settings.configuration.globalConfiguration.showAdditionalMenuItems) {
              AppLocalizedText([
                "settings.ui.show_additional_menu_items",
                " ",
                .init(
                  "settings.general.defaults.value",
                  arguments: [
                    "value": localized(
                      defaults.globalConfiguration.showAdditionalMenuItems
                        ? "shared.value.on" : "shared.value.off")
                  ]),
              ])
            }
            .switchToggleStyle()

            Toggle(isOn: $settings.configuration.globalConfiguration.showQuitConfirmationMenu) {
              AppLocalizedText([
                "settings.ui.show_quit_confirmation_menu",
                " ",
                .init(
                  "settings.general.defaults.value",
                  arguments: [
                    "value": localized(
                      defaults.globalConfiguration.showQuitConfirmationMenu
                        ? "shared.value.on" : "shared.value.off")
                  ]),
              ])
            }
            .switchToggleStyle()
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }

        GroupBox(label: AppLocalizedText("settings.ui.notification_window")) {
          VStack(alignment: .leading, spacing: 12.0) {
            Toggle(isOn: $settings.configuration.globalConfiguration.enableNotificationWindow) {
              AppLocalizedText([
                "settings.ui.enable_notification_window",
                " ",
                .init(
                  "settings.general.defaults.value",
                  arguments: [
                    "value": localized(
                      defaults.globalConfiguration.enableNotificationWindow
                        ? "shared.value.on" : "shared.value.off")
                  ]),
              ])
            }
            .switchToggleStyle()

            if settings.configuration.globalConfiguration.enableNotificationWindow {
              Toggle(
                isOn: $settings.configuration.selectedProfile.virtualHidKeyboard
                  .indicateStickyModifierKeysState
              ) {
                AppLocalizedText([
                  "settings.ui.indicate_sticky_modifier_keys_state",
                  " ",
                  .init(
                    "settings.general.defaults.value",
                    arguments: [
                      "value": localized(
                        defaults.selectedProfile.virtualHidKeyboard.indicateStickyModifierKeysState
                          ? "shared.value.on" : "shared.value.off")
                    ]),
                ])
              }
              .switchToggleStyle()

              GroupBox(label: AppLocalizedText("settings.ui.appearance")) {
                VStack(alignment: .leading, spacing: 12.0) {
                  HStack(spacing: 24.0) {
                    Picker(
                      selection: $settings.configuration.globalConfiguration
                        .notificationWindowPosition,
                      label: AppLocalizedConstrainedText("settings.ui.notification_window_position")
                    ) {
                      AppLocalizedConstrainedText("settings.ui.position.top_left").tag(
                        SettingsConfiguration.GlobalConfiguration.NotificationWindowPosition.topLeft
                      )
                      AppLocalizedConstrainedText("settings.ui.position.top_right").tag(
                        SettingsConfiguration.GlobalConfiguration.NotificationWindowPosition
                          .topRight)
                      AppLocalizedConstrainedText("settings.ui.position.bottom_left").tag(
                        SettingsConfiguration.GlobalConfiguration.NotificationWindowPosition
                          .bottomLeft)
                      AppLocalizedConstrainedText("settings.ui.position.bottom_right").tag(
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
                      AppLocalizedText([
                        "settings.ui.notification_window_respect_screen_visible_frame",
                        " ",
                        .init(
                          "settings.general.defaults.value",
                          arguments: [
                            "value": localized(
                              defaults.globalConfiguration
                                .notificationWindowRespectScreenVisibleFrame
                                ? "shared.value.on" : "shared.value.off")
                          ]),
                      ])
                    }
                    .switchToggleStyle()
                  }

                  Toggle(
                    isOn: $settings.configuration.globalConfiguration.notificationWindowShowIcon
                  ) {
                    AppLocalizedText([
                      "settings.ui.notification_window_show_icon",
                      " ",
                      .init(
                        "settings.general.defaults.value",
                        arguments: [
                          "value": localized(
                            defaults.globalConfiguration.notificationWindowShowIcon
                              ? "shared.value.on" : "shared.value.off")
                        ]),
                    ])
                  }
                  .switchToggleStyle()

                  HStack {
                    AppLocalizedText("settings.ui.notification_window_font_size")
                    IntTextField(
                      value: $settings.configuration.globalConfiguration.notificationWindowFontSize,
                      range: 8...64,
                      step: 1,
                      width: 40)
                    AppLocalizedText("settings.general.units.points")
                    AppLocalizedText(
                      "settings.general.defaults.value",
                      arguments: [
                        "value": String(defaults.globalConfiguration.notificationWindowFontSize)
                      ])
                  }

                  Grid(alignment: .leading, horizontalSpacing: 12.0, verticalSpacing: 12.0) {
                    notificationWindowColorSettings(
                      title: "settings.ui.section.light",
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
                      title: "settings.ui.section.dark",
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
              AppLocalizedLabel(
                "settings.ui.notification_description",
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

        GroupBox(label: AppLocalizedText("settings.ui.app_icon")) {
          VStack(alignment: .leading, spacing: 12.0) {
            VStack {
              AppLocalizedLabel(
                "settings.ui.app_icon_hint",
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
      AppLocalizedText(title)
        .bold()
        .fixedSize(horizontal: true, vertical: false)
      notificationWindowColorPicker(
        title: "settings.ui.background_color",
        value: background,
        systemColor: backgroundSystemColor)
      notificationWindowColorPicker(
        title: "settings.ui.text_color",
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
        selection: Binding(
          get: {
            value.wrappedValue == "system"
              ? Color(nsColor: systemColor)
              : Color(colorString: value.wrappedValue)
          },
          set: { color in
            value.wrappedValue = NSColor(color).notificationWindowColorString ?? "system"
          }),
        supportsOpacity: true
      ) {
        AppLocalizedConstrainedText(title)
      }

      Button {
        value.wrappedValue = "system"
      } label: {
        Image(systemName: "arrow.counterclockwise")
      }
      .buttonStyle(.borderless)
      .disabled(value.wrappedValue == "system")
      .help(localized("settings.ui.use_system_color"))
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
