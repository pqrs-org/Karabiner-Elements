import SwiftUI

struct SimpleModificationsView: View {
  @ObservedObject private var settings = Settings.shared
  @ObservedObject private var contentViewStates = ContentViewStates.shared

  var body: some View {
    HSplitView {
      DeviceSelectorView(selectedDevice: $contentViewStates.simpleModificationsViewSelectedDevice)
        .frame(minWidth: 250, maxWidth: 250)

      SimpleModificationView(
        selectedDevice: contentViewStates.simpleModificationsViewSelectedDevice)
    }
    .onAppear {
      settings.appendSimpleModificationIfEmpty(
        device: contentViewStates.simpleModificationsViewSelectedDevice)
    }
    .onChange(of: contentViewStates.simpleModificationsViewSelectedDevice) { newDevice in
      settings.appendSimpleModificationIfEmpty(device: newDevice)
    }
  }

  struct SimpleModificationView: View {
    @ObservedObject private var settings = Settings.shared

    private let selectedDevice: ConnectedDevice?
    private let simpleModifications: [SimpleModification]

    init(selectedDevice: ConnectedDevice?) {
      self.selectedDevice = selectedDevice
      self.simpleModifications = Settings.shared.simpleModifications(
        connectedDevice: selectedDevice)
    }

    var body: some View {
      // Use `ScrollView` instead of `List` to avoid `AttributeGraph: cycle detected through attribute` error.
      ScrollView {
        VStack(alignment: .leading, spacing: 4.0) {
          ForEach(simpleModifications) { simpleModification in
            HStack {
              SimpleModificationPickerView(
                categories: SimpleModificationDefinitions.shared.fromCategories,
                label: simpleModification.fromEntry.label,
                action: { json in
                  Settings.shared.updateSimpleModification(
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
                categories: SimpleModificationDefinitions.shared.toCategories,
                label: simpleModification.toEntry.label,
                action: { json in
                  Settings.shared.updateSimpleModification(
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
                  Settings.shared.removeSimpleModification(
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
            .frame(maxWidth: .infinity, alignment: .leading)

            Divider()
          }

          Button(
            action: {
              Settings.shared.appendSimpleModification(device: selectedDevice)
            },
            label: {
              // Use `Image` and `Text` instead of `Label` to set icon color like `Button` in `List`.
              Image(systemName: "plus.circle.fill").foregroundColor(.blue)
              Text("Add item")
            }
          )
          .buttonStyle(.automatic)
          .if(simpleModifications.count > 0) {
            $0.padding(.top)
          }
        }
        .padding()
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(Color(NSColor.textBackgroundColor))
      }
    }
  }
}
