import SwiftUI

struct FunctionKeysView: View {
  @ObservedObject private var settings = Settings.shared
  @ObservedObject private var settingsCoreServiceDaemonClient = SettingsCoreServiceDaemonClient
    .shared
  @ObservedObject private var contentViewStates = ContentViewStates.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 0.0) {
      VStack(alignment: .leading) {
        // When using Apple's Vendor ID and Product ID with the virtual keyboard,
        // useFkeysAsStandardFunctionKeys needs to be changed through the System Settings; otherwise,
        // the setting will not be applied correctly.
        // Therefore, instead of changing it directly here, providing a button to open the System Settings.

        HStack {
          Text("Use all F1, F2, etc. keys as standard function keys:")

          if settingsCoreServiceDaemonClient.useFkeysAsStandardFunctionKeys {
            Text("On").foregroundColor(.accentColor).bold()
          } else {
            Text("Off")
          }
        }

        OpenSystemSettingsButton(
          url: "x-apple.systempreferences:com.apple.Keyboard-Settings.extension?FunctionKeys",
          label: {
            Label(
              "Open System Settings > Function Keys...",
              systemImage: "arrow.up.forward.app")
          }
        )
      }
      .padding()

      Divider()

      HSplitView {
        DeviceSelectorView(selectedDevice: $contentViewStates.functionKeysViewSelectedDevice)
          .frame(minWidth: 250, maxWidth: 250)

        FnFunctionKeysView(selectedDevice: contentViewStates.functionKeysViewSelectedDevice)
      }
    }
  }

  struct FnFunctionKeysView: View {
    @ObservedObject private var settings = Settings.shared
    @ObservedObject private var settingsCoreServiceDaemonClient = SettingsCoreServiceDaemonClient
      .shared

    private let selectedDevice: ConnectedDevice?
    private let fnFunctionKeys: [SettingsConfiguration.SimpleModification]

    init(selectedDevice: ConnectedDevice?) {
      self.selectedDevice = selectedDevice
      self.fnFunctionKeys =
        Settings.shared.fnFunctionKeys(connectedDevice: selectedDevice)
    }

    var body: some View {
      ScrollView {
        VStack(alignment: .leading, spacing: 4.0) {
          ForEach(fnFunctionKeys) { fnFunctionKey in
            let fromEntry = fnFunctionKey.fromEntry
            let toEntry = fnFunctionKey.toEntry(
              categories: selectedDevice == nil
                ? SimpleModificationDefinitions.shared.toCategories
                : SimpleModificationDefinitions.shared.toCategoriesWithInheritBase)

            HStack {
              Text(
                settingsCoreServiceDaemonClient.useFkeysAsStandardFunctionKeys
                  ? "fn + \(fromEntry.label)"
                  : fromEntry.label
              )
              .monospaced()
              .frame(width: 80, alignment: .trailing)

              Image(systemName: "arrow.forward")
                .padding(.horizontal, 6.0)

              SimpleModificationPickerView(
                categories: selectedDevice == nil
                  ? SimpleModificationDefinitions.shared.toCategories
                  : SimpleModificationDefinitions.shared.toCategoriesWithInheritBase,
                label: toEntry.label,
                action: { json in
                  Settings.shared.updateFnFunctionKey(
                    fromJsonString: fromEntry.json,
                    toJsonString: json,
                    device: selectedDevice)
                },
                showUnsafe: settings.configuration.globalConfiguration.unsafeUi
                  || (selectedDevice?.isGamePad ?? false)
              )
            }

            Divider()
          }
        }
        .padding()
        .background(Color(NSColor.textBackgroundColor))
      }
    }
  }
}
