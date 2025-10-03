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
  @State private var hoverRuleIndex: Int?
  @State private var editingRule: LibKrbn.ComplexModificationsRule?

  var body: some View {
    VStack(alignment: .leading, spacing: 0.0) {
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

            editingRule = LibKrbn.ComplexModificationsRule(
              index: -1,
              description: "Edit the following setting and press the Save button.",
              enabled: true,
              jsonString: String(utf8String: buffer) ?? ""
            )
            showingEditSheet = true
          },
          label: {
            AccentColorIconLabel(title: "Add your own rule", systemImage: "sparkles")
          })

        if settings.complexModificationsRules.count > 1 {
          Spacer()

          HStack {
            Text("You can reorder list by dragging")
            Image(systemName: "arrow.up.arrow.down.square.fill")
              .resizable(resizingMode: .stretch)
              .frame(width: 16.0, height: 16.0)
            Text("icon")
          }
        }
      }
      .padding()
      .frame(maxWidth: .infinity, alignment: .leading)

      List {
        ForEach($settings.complexModificationsRules) { $complexModificationRule in
          // Store the ruleIndex here to prevent referencing a deleted complexModificationRule in onHover when a rule is removed.
          let ruleIndex = complexModificationRule.index

          HStack(alignment: .center, spacing: 0) {
            if settings.complexModificationsRules.count > 1 {
              Image(systemName: "arrow.up.arrow.down.square.fill")
                .resizable(resizingMode: .stretch)
                .frame(width: 16.0, height: 16.0)
                .padding(.trailing, 6.0)
                .onHover { hovering in
                  moveDisabled = !hovering
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
            }

            HStack {
              Text(complexModificationRule.description)
                .frame(maxWidth: .infinity, alignment: .leading)
                .if(!complexModificationRule.enabled) {
                  $0.foregroundColor(.gray)
                }

              if !complexModificationRule.enabled {
                Text("disabled")
                  .foregroundColor(.gray)
              }
            }
            .if(hoverRuleIndex == ruleIndex) {
              $0.overlay(
                RoundedRectangle(cornerRadius: 2)
                  .inset(by: -4)
                  .stroke(
                    Color(NSColor(Color.accentColor)),
                    lineWidth: 2
                  )
              )
            }

            HStack(alignment: .center, spacing: 10) {
              Toggle(isOn: $complexModificationRule.enabled) {
                Text("")
              }
              .switchToggleStyle()
              .padding(.trailing, 10.0)
              .frame(width: 60)

              Button(
                action: {
                  editingRule = complexModificationRule
                  showingEditSheet = true
                },
                label: {
                  Label("Edit", systemImage: "pencil.circle.fill")
                })

              Button(
                role: .destructive,
                action: {
                  settings.removeComplexModificationsRule(complexModificationRule)
                },
                label: {
                  Image(systemName: "trash")
                    .buttonLabelStyle()
                }
              )
              .deleteButtonStyle()
              .frame(width: 60)
            }
            .onHover { hovering in
              if hovering {
                hoverRuleIndex = ruleIndex
              } else {
                if hoverRuleIndex == ruleIndex {
                  hoverRuleIndex = nil
                }
              }
            }
          }
          .listOverlayDivider()
          .moveDisabled(moveDisabled)
        }
        .onMove { indices, destination in
          if let first = indices.first {
            settings.moveComplexModificationsRule(first, destination)
          }
        }
        .listRowSeparator(.hidden)
      }
      .background(Color(NSColor.textBackgroundColor))
    }
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
