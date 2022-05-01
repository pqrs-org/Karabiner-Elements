import SwiftUI

struct SimpleModificationsView: View {
  @ObservedObject private var settings = Settings.shared
  @State private var selectedDevice: ConnectedDevice? = nil

  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      HStack(alignment: .top, spacing: 12.0) {
        DeviceSelectorView(selectedDevice: $selectedDevice)

        VStack {
          SimpleModificationView(selectedDevice: selectedDevice)

          Spacer()
        }

        Spacer()
      }
    }
    .padding()
  }

  struct SimpleModificationView: View {
    private let selectedDevice: ConnectedDevice?
    private let simpleModifications: [SimpleModification]

    init(selectedDevice: ConnectedDevice?) {
      self.selectedDevice = selectedDevice
      self.simpleModifications =
        selectedDevice == nil
        ? Settings.shared.simpleModifications
        : Settings.shared.findConnectedDeviceSetting(selectedDevice!)?.simpleModifications ?? []
    }

    var body: some View {
      VStack(alignment: .leading, spacing: 6.0) {
        ForEach(simpleModifications) { simpleModification in
          HStack {
            SimpleModificationPickerView(
              categories: SimpleModificationDefinitions.shared.fromCategories,
              label: simpleModification.fromEntry.label,
              action: { json in
                Settings.shared.updateSimpleModification(
                  index: simpleModification.index,
                  fromJson: json,
                  toJson: simpleModification.toEntry.json,
                  device: selectedDevice)
              }
            )

            SimpleModificationPickerView(
              categories: SimpleModificationDefinitions.shared.toCategories,
              label: simpleModification.toEntry.label,
              action: { json in
                Settings.shared.updateSimpleModification(
                  index: simpleModification.index,
                  fromJson: simpleModification.fromEntry.json,
                  toJson: json,
                  device: selectedDevice)
              }
            )
          }

          Divider()
        }
      }
      .background(Color(NSColor.textBackgroundColor))
    }
  }
}

struct SimpleModificationsView_Previews: PreviewProvider {
  static var previews: some View {
    SimpleModificationsView()
  }
}
