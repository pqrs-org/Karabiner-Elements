import SwiftUI

enum ComplexModificationsSheetView: String {
  case assets
  case fileImport
}

struct ComplexModificationsView: View {
  @ObservedObject private var contentViewStates = ContentViewStates.shared
  @ObservedObject private var settings = LibKrbn.Settings.shared
  @State private var moveDisabled: Bool = true
  @State private var showingEditSheet = false
  @State private var editingRule: LibKrbn.ComplexModificationsRule?

  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      HStack {
        Button(
          action: {
            contentViewStates.complexModificationsViewSheetView =
              ComplexModificationsSheetView.assets
            contentViewStates.complexModificationsViewSheetPresented = true
          },
          label: {
            AccentColorIconLabel(title: "Add predefined rule", systemImage: "plus.circle.fill")
          })

        Button(
          action: {
            var buffer = [Int8](repeating: 0, count: 32 * 1024)
            libkrbn_core_configuration_get_new_complex_modifications_rule_json_string(
              &buffer, buffer.count)

            editingRule = LibKrbn.ComplexModificationsRule(-1, "New rule", String(cString: buffer))
            showingEditSheet = true
          },
          label: {
            AccentColorIconLabel(title: "Add new rule", systemImage: "pencil.circle.fill")
          })

        Spacer()

        if settings.complexModificationsRules.count > 1 {
          HStack {
            Text("You can reorder list by dragging")
            Image(systemName: "arrow.up.arrow.down.square.fill")
              .resizable(resizingMode: .stretch)
              .frame(width: 16.0, height: 16.0)
            Text("icon")
          }
        }
      }.padding([.leading, .trailing], 16)

      List {
        ForEach($settings.complexModificationsRules) { $complexModificationRule in
          VStack {
            HStack(alignment: .center, spacing: 0) {
              if settings.complexModificationsRules.count > 1 {
                Image(systemName: "arrow.up.arrow.down.square.fill")
                  .resizable(resizingMode: .stretch)
                  .frame(width: 16.0, height: 16.0)
                  .onHover { hovering in
                    moveDisabled = !hovering
                  }
              }

              Text(complexModificationRule.description)
                .padding(.leading, 6.0)

              Spacer()

              Button(
                action: {
                  editingRule = complexModificationRule
                  showingEditSheet = true
                },
                label: {
                  Label("Edit", systemImage: "pencil.circle.fill")
                    .padding(.horizontal, 10.0)
                })

              Button(
                action: {
                  settings.removeComplexModificationsRule(complexModificationRule)
                },
                label: {
                  Image(systemName: "trash.fill")
                    .buttonLabelStyle()
                }
              )
              .deleteButtonStyle()
              .frame(width: 60)
            }
            .contextMenu {
              Section(header: Text("Position")) {
                Button {
                  settings.moveComplexModificationsRule(complexModificationRule.index, 0)
                } label: {
                  Label("Move item to top", systemImage: "arrow.up.to.line")
                }

                Button {
                  settings.moveComplexModificationsRule(
                    complexModificationRule.index, settings.complexModificationsRules.count)
                } label: {
                  Label("Move item to bottom", systemImage: "arrow.down.to.line")
                }
              }
            }

            Divider()
          }
          .moveDisabled(moveDisabled)
        }.onMove { indices, destination in
          if let first = indices.first {
            settings.moveComplexModificationsRule(first, destination)
          }
        }
      }
      .background(Color(NSColor.textBackgroundColor))
    }
    .padding()
    .sheet(isPresented: $contentViewStates.complexModificationsViewSheetPresented) {
      if let sheetView = contentViewStates.complexModificationsViewSheetView {
        switch sheetView {
        case ComplexModificationsSheetView.assets:
          ComplexModificationsAssetsView()
        case ComplexModificationsSheetView.fileImport:
          ComplexModificationsFileImportView()
        }
      }
    }
    .sheet(isPresented: $showingEditSheet) {
      ComplexModificationsEditView(rule: $editingRule, showing: $showingEditSheet)
    }
  }
}

struct ComplexModificationsView_Previews: PreviewProvider {
  static var previews: some View {
    ComplexModificationsView()
  }
}
