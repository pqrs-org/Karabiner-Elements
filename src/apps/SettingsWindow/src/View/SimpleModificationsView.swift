import SwiftUI

struct SimpleModificationsView: View {
  @ObservedObject private var settings = LibKrbn.Settings.shared
  @ObservedObject private var contentViewStates = ContentViewStates.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      HSplitView {
        DeviceSelectorView(selectedDevice: $contentViewStates.simpleModificationsViewSelectedDevice)
          .frame(minWidth: 250, maxWidth: 250)

        SimpleModificationView(
          selectedDevice: contentViewStates.simpleModificationsViewSelectedDevice)
      }
    }
    .padding()
    .onAppear {
      settings.appendSimpleModificationIfEmpty(
        device: contentViewStates.simpleModificationsViewSelectedDevice)
    }
    .onChange(of: contentViewStates.simpleModificationsViewSelectedDevice) { newDevice in
      settings.appendSimpleModificationIfEmpty(device: newDevice)
    }
  }

  struct SimpleModificationView: View {
    @ObservedObject private var settings = LibKrbn.Settings.shared

    private let selectedDevice: LibKrbn.ConnectedDevice?
    private let simpleModifications: [LibKrbn.SimpleModification]

    init(selectedDevice: LibKrbn.ConnectedDevice?) {
      self.selectedDevice = selectedDevice
      self.simpleModifications = LibKrbn.Settings.shared.simpleModifications(
        connectedDevice: selectedDevice)
    }

    var body: some View {
      // Use `ScrollView` instead of `List` to avoid `AttributeGraph: cycle detected through attribute` error.
      ScrollView {
        VStack(alignment: .leading, spacing: 6.0) {
          ForEach(simpleModifications) { simpleModification in
            HStack {
              SimpleModificationPickerView(
                categories: LibKrbn.SimpleModificationDefinitions.shared.fromCategories,
                label: simpleModification.fromEntry.label,
                action: { json in
                  LibKrbn.Settings.shared.updateSimpleModification(
                    index: simpleModification.index,
                    fromJsonString: json,
                    toJsonString: simpleModification.toEntry.json,
                    device: selectedDevice)
                },
                showUnsafe: settings.unsafeUI || (selectedDevice?.isGamePad ?? false)
              )

              Image(systemName: "arrow.forward")
                .padding(.horizontal, 6.0)

              SimpleModificationPickerView(
                categories: LibKrbn.SimpleModificationDefinitions.shared.toCategories,
                label: simpleModification.toEntry.label,
                action: { json in
                  LibKrbn.Settings.shared.updateSimpleModification(
                    index: simpleModification.index,
                    fromJsonString: simpleModification.fromEntry.json,
                    toJsonString: json,
                    device: selectedDevice)
                },
                showUnsafe: settings.unsafeUI || (selectedDevice?.isGamePad ?? false)
              )
              .padding(.trailing, 24.0)

              Button(
                role: .destructive,
                action: {
                  LibKrbn.Settings.shared.removeSimpleModification(
                    index: simpleModification.index,
                    device: selectedDevice)
                },
                label: {
                  Image(systemName: "trash")
                    .buttonLabelStyle()
                }
              )
              .deleteButtonStyle()
            }

            Divider()
          }

          HStack {
            Button(
              action: {
                LibKrbn.Settings.shared.appendSimpleModification(device: selectedDevice)
              },
              label: {
                // Use `Image` and `Text` instead of `Label` to set icon color like `Button` in `List`.
                Image(systemName: "plus.circle.fill").foregroundColor(.blue)
                Text("Add item")
              }
            )
            .buttonStyle(.automatic)

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
}
