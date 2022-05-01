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

            Button(action: {
              Settings.shared.removeSimpleModification(
                index: simpleModification.index,
                device: selectedDevice)
            }) {
              Image(systemName: "trash.fill")
                .buttonLabelStyle()
            }
            .deleteButtonStyle()
          }

          Divider()
        }

        HStack {
          Button(action: {
            Settings.shared.appendSimpleModification(device: selectedDevice)
          }) {
            Label("Add item", systemImage: "plus.circle.fill")
          }

          Spacer()
        }
        .if(simpleModifications.count > 0) {
          $0.padding(.top, 20.0)
        }

        Spacer()
      }
      .padding(10)
      .background(Color(NSColor.textBackgroundColor))
    }
  }
}

struct SimpleModificationsView_Previews: PreviewProvider {
  static var previews: some View {
    SimpleModificationsView()
  }
}
