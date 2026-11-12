import SwiftUI

struct DevicesView: View {
  @AppLocalizationContext private var localized
  @ObservedObject private var settings = Settings.shared
  @ObservedObject private var connectedDevices = ConnectedDevices.shared
  @State private var showEraseNotConnectedDeviceSettingsButton = false

  var body: some View {
    VStack(alignment: .leading, spacing: 0.0) {
      // Build all device rows eagerly so offscreen grids are laid out before scrolling.
      ScrollView {
        VStack(alignment: .leading, spacing: 0.0) {
          ForEach(
            Array(connectedDevices.connectedDevices.enumerated()),
            id: \.element.id
          ) { index, connectedDevice in
            if let deviceConfiguration = settings.deviceConfigurationBinding(connectedDevice) {
              HStack(alignment: .top, spacing: 8.0) {
                ConnectedDeviceIcons(
                  isKeyboard: connectedDevice.isKeyboard,
                  isPointingDevice: connectedDevice.isPointingDevice,
                  isGamePad: connectedDevice.isGamePad,
                  isConsumer: connectedDevice.isConsumer
                )

                VStack(alignment: .leading, spacing: 0.0) {
                  DeviceName(connectedDevice: connectedDevice)

                  if !connectedDevice.isVirtualDevice {
                    VStack(alignment: .leading, spacing: 0.0) {
                      ModifyEventsSetting(
                        connectedDevice: connectedDevice,
                        deviceConfiguration: deviceConfiguration)

                      VStack(alignment: .leading, spacing: 6.0) {
                        KeyboardSettings(
                          connectedDevice: connectedDevice,
                          deviceConfiguration: deviceConfiguration)

                        MouseSettings(
                          connectedDevice: connectedDevice,
                          deviceConfiguration: deviceConfiguration)

                        GamePadSettings(
                          connectedDevice: connectedDevice,
                          deviceConfiguration: deviceConfiguration)

                        ExtraSettings(
                          connectedDevice: connectedDevice,
                          deviceConfiguration: deviceConfiguration)
                      }
                      .padding(.leading, 20.0)
                      .padding(.top, 8.0)

                      LogitechHidppSettings(
                        connectedDevice: connectedDevice,
                        deviceConfiguration: deviceConfiguration
                      )
                      .padding(.leading, 20.0)
                      .padding(.top, 12.0)
                    }
                    .padding(.top, 8.0)
                  }
                }
                .frame(minWidth: 0.0, maxWidth: .infinity, alignment: .leading)

                DeviceIdentifiers(connectedDevice: connectedDevice)
              }
              .if(connectedDevice.isVirtualDevice) {
                $0.foregroundColor(Color(NSColor.placeholderTextColor))
              }
              .padding(.vertical, 12.0)
              .padding(.horizontal, 12.0)
              .frame(maxWidth: .infinity, alignment: .leading)
              .overlay(
                RoundedRectangle(cornerRadius: 8)
                  .stroke(
                    Color(NSColor.selectedControlColor),
                    lineWidth: deviceConfiguration.wrappedValue.modifyEvents
                      && !connectedDevice.isVirtualDevice
                      ? 3 : 0
                  )
                  .padding(2)
              )

              if index < connectedDevices.connectedDevices.count - 1 {
                Divider()
              }
            }
          }
        }
        .padding(8.0)
      }
      .background(Color(NSColor.textBackgroundColor))

      if connectedDevices.notConnectedConfiguredDevicesCount > 0 {
        HStack {
          Label(
            localized(
              "settings.devices.disconnected_count",
              arguments: ["count": String(connectedDevices.notConnectedConfiguredDevicesCount)]),
            systemImage: InfoBorder.icon
          )
          .frame(maxWidth: .infinity, alignment: .leading)

          if !showEraseNotConnectedDeviceSettingsButton {
            Button(
              action: {
                showEraseNotConnectedDeviceSettingsButton = true
              },
              label: {
                Image(systemName: "trash")
                  .buttonLabelStyle()
              }
            )
          } else {
            Button(
              role: .destructive,
              action: {
                settings.eraseNotConnectedDeviceSettings()
              },
              label: {
                Label(
                  localized(
                    "settings.devices.remove_disconnected",
                    arguments: [
                      "count": String(connectedDevices.notConnectedConfiguredDevicesCount)
                    ]),
                  systemImage: "trash"
                )
                .buttonLabelStyle()
              }
            )
            .deleteButtonStyle()
          }
        }
        .modifier(InfoBorder())
        .padding()
      }
    }
  }

  struct DeviceName: View {
    @AppLocalizationContext private var localized
    let connectedDevice: ConnectedDevice

    var body: some View {
      Text(
        "\(connectedDevice.localizedProductName(localized)) (\(connectedDevice.localizedManufacturerName(localized)))"
      )
      .frame(maxWidth: .infinity, alignment: .leading)
    }
  }

  struct DeviceIdentifiers: View {
    private enum Layout {
      static let tableWidth: CGFloat = 240.0
      static let cellPadding: CGFloat = 4.0
      static let labelWidth: CGFloat = 70.0
      static let separatorWidth: CGFloat = 1.0
      static let labelColumnWidth = labelWidth + cellPadding * 2.0
      static let valueWidth =
        tableWidth - labelColumnWidth - separatorWidth - cellPadding * 2.0
    }

    private struct Row {
      let label: String
      let value: String
    }

    @ObservedObject private var settings = Settings.shared
    let connectedDevice: ConnectedDevice

    private var rows: [Row] {
      var result: [Row] = []

      if connectedDevice.vendorId != 0 {
        result.append(
          Row(
            label: "settings.devices.vendor_id",
            value:
              "\(connectedDevice.vendorId) (\(String(format: "0x%04x", connectedDevice.vendorId)))"
          ))
      }

      if connectedDevice.productId != 0 {
        result.append(
          Row(
            label: "settings.devices.product_id",
            value:
              "\(connectedDevice.productId) (\(String(format: "0x%04x", connectedDevice.productId)))"
          ))
      }

      if !connectedDevice.deviceAddress.isEmpty {
        result.append(
          Row(
            label: "settings.devices.address",
            value: connectedDevice.deviceAddress))
      }

      if settings.configuration.globalConfiguration.additionalDeviceIdentifierFields.contains(
        "serial_number"), !connectedDevice.serialNumber.isEmpty
      {
        result.append(
          Row(
            label: "settings.devices.serial_number",
            value: connectedDevice.serialNumber))
      }

      if settings.configuration.globalConfiguration.additionalDeviceIdentifierFields.contains(
        "transport"), !connectedDevice.transport.isEmpty
      {
        result.append(
          Row(
            label: "settings.devices.transport",
            value: connectedDevice.transport))
      }

      return result
    }

    var body: some View {
      if connectedDevice.transport != "FIFO" {
        VStack(alignment: .leading, spacing: 0.0) {
          ForEach(Array(rows.enumerated()), id: \.element.label) { index, row in
            HStack(alignment: .top, spacing: 0.0) {
              AppLocalizedText(row.label)
                .frame(width: Layout.labelWidth, alignment: .topTrailing)
                .padding(Layout.cellPadding)

              Color.clear
                .frame(width: Layout.separatorWidth, height: 0.0)

              Text(row.value)
                .fixedSize(horizontal: false, vertical: true)
                .frame(width: Layout.valueWidth, alignment: .topLeading)
                .padding(Layout.cellPadding)
            }
            .overlay(alignment: .topLeading) {
              Rectangle()
                .fill(Color(NSColor.separatorColor))
                .frame(width: Layout.separatorWidth)
                .frame(maxHeight: .infinity)
                .offset(x: Layout.labelColumnWidth)
            }
            .overlay(alignment: .bottomLeading) {
              if index < rows.count - 1 {
                Divider()
                  .frame(width: Layout.tableWidth)
              }
            }
          }
        }
        .font(.caption)
        .monospaced()
        .overlay(
          RoundedRectangle(cornerRadius: 6.0)
            .stroke(Color(NSColor.separatorColor))
        )
      }
    }
  }

  struct ModifyEventsSetting: View {
    @AppLocalizationContext private var localized
    let connectedDevice: ConnectedDevice
    @Binding var deviceConfiguration: SettingsConfiguration.Device

    @ObservedObject private var settings = Settings.shared

    var body: some View {
      HStack(alignment: .top) {
        if connectedDevice.isAppleDevice,
          !connectedDevice.isKeyboard,
          connectedDevice.isPointingDevice,
          !settings.configuration.globalConfiguration.unsafeUi
        {
          AppLocalizedText("settings.devices.apple_pointing_unsupported")
            .foregroundColor(Color(NSColor.placeholderTextColor))
            .frame(maxWidth: .infinity, alignment: .leading)
        } else {
          VStack(alignment: .leading) {
            Toggle(isOn: $deviceConfiguration.modifyEvents) {
              AppLocalizedText("settings.devices.modify_events")
            }
            .switchToggleStyle()
            .fixedSize(horizontal: true, vertical: false)

            if settings.configuration.globalConfiguration.enableCgeventtapFallback
              && !deviceConfiguration.modifyEvents
              && connectedDevice.isKeyboard
            {
              Label(
                title: {
                  AppLocalizedText(
                    "settings.devices.fallback_hint"
                  )
                  .textSelection(.enabled)
                },
                icon: {
                  Image(systemName: InfoBorder.icon)
                }
              )
              .modifier(InfoBorder())
            }
          }
        }
      }
    }
  }

  struct KeyboardSettings: View {
    @AppLocalizationContext private var localized
    let connectedDevice: ConnectedDevice
    @Binding var deviceConfiguration: SettingsConfiguration.Device

    @ObservedObject private var settings = Settings.shared

    var body: some View {
      if connectedDevice.isKeyboard {
        if !connectedDevice.isBuiltInKeyboard
          && !deviceConfiguration.disableBuiltInKeyboardIfExists
        {
          DetailedSetting(
            title: "settings.devices.treat_as_built_in",
            isOn: $deviceConfiguration.treatAsBuiltInKeyboard)
        }

        if !connectedDevice.isBuiltInKeyboard
          && !deviceConfiguration.treatAsBuiltInKeyboard
        {
          DetailedSetting(
            title: "settings.devices.disable_built_in",
            isOn: $deviceConfiguration.disableBuiltInKeyboardIfExists)
        }

        if deviceConfiguration.modifyEvents {
          DetailedSetting(
            title: "settings.devices.caps_lock_led",
            isOn: $deviceConfiguration.manipulateCapsLockLed)

          DetailedSetting(
            title: "settings.devices.swap_iso_keys",
            isOn: $deviceConfiguration.swapGraveAccentAndNonUsBackslash)
        }
      }
    }
  }

  struct DetailedSetting: View {
    let title: String
    @Binding var isOn: Bool

    var body: some View {
      HStack(alignment: .firstTextBaseline, spacing: 8.0) {
        AppLocalizedText(title)
          .font(.callout)

        Toggle(isOn: $isOn) {
          AppLocalizedText(title)
        }
        .switchToggleStyle(controlSize: .mini, font: .callout)
        .labelsHidden()
      }
    }
  }

  struct MouseSettings: View {
    @AppLocalizationContext private var localized
    let connectedDevice: ConnectedDevice
    @Binding var deviceConfiguration: SettingsConfiguration.Device
    @State var showing = false

    var body: some View {
      if deviceConfiguration.modifyEvents
        && connectedDevice.isPointingDevice
      {
        Button(
          action: {
            showing = true
          },
          label: {
            AppLocalizedConstrainedLabel(
              "settings.devices.open_mouse_settings", systemImage: "computermouse"
            )
            .buttonLabelStyle()
          }
        )
        .sheet(isPresented: $showing) {
          DevicesMouseSettingsView(
            connectedDevice: connectedDevice,
            deviceConfiguration: $deviceConfiguration,
            showing: $showing
          )
          .modifier(SettingsLanguage())
        }
      } else {
        EmptyView()
      }
    }
  }

  struct GamePadSettings: View {
    @AppLocalizationContext private var localized
    let connectedDevice: ConnectedDevice
    @Binding var deviceConfiguration: SettingsConfiguration.Device
    @State var showing = false

    var body: some View {
      if deviceConfiguration.modifyEvents && connectedDevice.isGamePad {
        Button(
          action: {
            showing = true
          },
          label: {
            AppLocalizedConstrainedLabel(
              "settings.devices.open_gamepad_settings", systemImage: "gamecontroller"
            )
            .buttonLabelStyle()
          }
        )
        .sheet(isPresented: $showing) {
          DevicesGamePadSettingsView(
            connectedDevice: connectedDevice,
            deviceConfiguration: $deviceConfiguration,
            showing: $showing
          )
          .modifier(SettingsLanguage())
        }
      } else {
        EmptyView()
      }
    }
  }

  struct ExtraSettings: View {
    @AppLocalizationContext private var localized
    let connectedDevice: ConnectedDevice
    @Binding var deviceConfiguration: SettingsConfiguration.Device

    var body: some View {
      if deviceConfiguration.modifyEvents && !connectedDevice.isAppleDevice {
        DetailedSetting(
          title: "settings.devices.ignore_vendor_events",
          isOn: $deviceConfiguration.ignoreVendorEvents)

        AppLocalizedLabel(
          "settings.devices.vendor_events_hint",
          systemImage: "lightbulb"
        )
        .foregroundColor(Color(NSColor.textColor))
        .font(.caption)
        .frame(maxWidth: .infinity, alignment: .leading)
      }
    }
  }

  struct LogitechHidppSettings: View {
    let connectedDevice: ConnectedDevice
    @Binding var deviceConfiguration: SettingsConfiguration.Device

    @ObservedObject private var settings = Settings.shared

    private let columns = [
      GridItem(.adaptive(minimum: 240.0), spacing: 12.0, alignment: .leading)
    ]

    private var controlIds: [UInt16] {
      var result = Set(
        connectedDevice.capabilities.logitechHidpp?.divertableControlIds ?? [])
      for control in deviceConfiguration.logitechHidpp?.buttons ?? [] {
        result.insert(control.controlId)
      }
      return result.sorted()
    }

    private func control(_ controlId: UInt16)
      -> SettingsConfiguration.Device.LogitechHidppButton?
    {
      deviceConfiguration.logitechHidpp?.buttons.first {
        $0.controlId == controlId
      }
    }

    var body: some View {
      if deviceConfiguration.modifyEvents
        && !controlIds.isEmpty
      {
        GroupBox(
          label: AppLocalizedText("settings.devices.logitech_hidpp_button_assignments")
        ) {
          LazyVGrid(columns: columns, alignment: .leading, spacing: 6.0) {
            ForEach(controlIds, id: \.self) { controlId in
              let configuredControl = control(controlId)

              HStack(spacing: 8.0) {
                Toggle(
                  isOn: Binding(
                    get: {
                      control(controlId)?.enabled ?? false
                    },
                    set: { enabled in
                      settings.setLogitechHidppButtonEnabled(
                        controlId: controlId,
                        enabled: enabled,
                        connectedDevice: connectedDevice)
                    })
                ) {
                  Text("\(controlId) (\(String(format: "0x%04x", controlId)))")
                    .font(.callout)
                    .monospacedDigit()
                }
                .toggleStyle(.checkbox)
                .fixedSize(horizontal: true, vertical: false)

                SimpleModificationPickerView(
                  categories: SimpleModificationDefinitions.shared.toCategories,
                  label: SimpleModificationDefinitions.shared.toCategories.findLabel(
                    jsonString: configuredControl?.toJsonString ?? "[]"),
                  action: { json in
                    settings.setLogitechHidppButtonTo(
                      controlId: controlId,
                      toJsonString: json,
                      connectedDevice: connectedDevice)
                  },
                  showUnsafe: settings.configuration.globalConfiguration.unsafeUi
                    || connectedDevice.isGamePad
                )
                .disabled(configuredControl?.enabled != true)
              }
              .frame(maxWidth: .infinity, alignment: .leading)
            }
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
      }
    }
  }
}
