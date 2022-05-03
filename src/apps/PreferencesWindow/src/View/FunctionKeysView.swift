import SwiftUI

struct FunctionKeysView: View {
  @ObservedObject private var settings = LibKrbn.Settings.shared
  @ObservedObject private var systemPreferences = SystemPreferences.shared
  @ObservedObject private var contentViewStates = ContentViewStates.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      HStack(alignment: .top, spacing: 12.0) {
        DeviceSelectorView(selectedDevice: $contentViewStates.functionKeysViewSelectedDevice)

        VStack {
          FnFunctionKeysView(selectedDevice: contentViewStates.functionKeysViewSelectedDevice)

          Spacer()
        }
      }

      HStack {
        Toggle(isOn: $systemPreferences.useFkeysAsStandardFunctionKeys) {
          Text("Use all F1, F2, etc. keys as standard function keys")
        }

        Spacer()
      }
    }
    .padding()
  }

  struct FnFunctionKeysView: View {
      private let selectedDevice: LibKrbn.ConnectedDevice?
      private let fnFunctionKeys: [LibKrbn.SimpleModification]

      init(selectedDevice: LibKrbn.ConnectedDevice?) {
      self.selectedDevice = selectedDevice
      self.fnFunctionKeys =
        selectedDevice == nil
          ? LibKrbn.Settings.shared.fnFunctionKeys
          : LibKrbn.Settings.shared.findConnectedDeviceSetting(selectedDevice!)?.fnFunctionKeys ?? []
    }

    var body: some View {
      VStack(alignment: .leading, spacing: 6.0) {
        ForEach(fnFunctionKeys) { fnFunctionKey in
          HStack {
            Text(fnFunctionKey.fromEntry.label)
              .frame(width: 40)

            SimpleModificationPickerView(
              categories: selectedDevice == nil
              ? LibKrbn.SimpleModificationDefinitions.shared.toCategories
              : LibKrbn.SimpleModificationDefinitions.shared.toCategoriesWithInheritBase,
              label: fnFunctionKey.toEntry.label,
              action: { json in
                  LibKrbn.Settings.shared.updateFnFunctionKey(
                  fromJsonString: fnFunctionKey.fromEntry.json,
                  toJsonString: json,
                  device: selectedDevice)
              }
            )
          }

          Divider()
        }
        Spacer()
      }
      .padding(10)
      .background(Color(NSColor.textBackgroundColor))
    }
  }
}

struct FunctionKeysView_Previews: PreviewProvider {
  static var previews: some View {
    FunctionKeysView()
  }
}
