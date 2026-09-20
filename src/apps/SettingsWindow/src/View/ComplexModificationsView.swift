import AppKit
import SwiftUI

enum ComplexModificationsSheetView: String {
  case assets
  case fileImport
}

struct ComplexModificationsView: View {
  @AppLocalizationContext private var localized
  @ObservedObject private var contentViewStates = ContentViewStates.shared
  @ObservedObject private var settings = Settings.shared
  @State private var moveDisabled: Bool = true
  @State private var showingEditSheet = false
  @State private var hoverRuleIndex: Int?
  @State private var editingRule: SettingsConfiguration.ComplexModificationsRule?
  @State private var filterKeyword = ""

  private var normalizedFilterKeyword: String {
    filterKeyword.trimmingCharacters(in: .whitespacesAndNewlines)
  }

  private var rules: [SettingsConfiguration.ComplexModificationsRule] {
    settings.configuration.selectedProfile.complexModifications.rules
  }

  private func matchesFilter(_ rule: SettingsConfiguration.ComplexModificationsRule) -> Bool {
    let keyword = normalizedFilterKeyword
    if keyword.isEmpty {
      return true
    }

    return rule.searchText.localizedCaseInsensitiveContains(keyword)
  }

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
            AccentColorIconLabel(
              title: localized("settings.complex_modifications.add_predefined"),
              systemImage: "plus.circle.fill")
          })

        Button(
          action: {
            var buffer = [Int8](repeating: 0, count: 32 * 1024)
            krbn_core_configuration_get_new_complex_modifications_rule_json_string(
              &buffer, buffer.count)

            editingRule = SettingsConfiguration.ComplexModificationsRule(
              index: -1,
              description: localized("settings.complex_modifications.editor.new_rule_hint"),
              descriptionNotes: [],
              enabled: true,
              codeString: String(utf8String: buffer) ?? "",
              searchText: "",
              codeType: .json
            )
            showingEditSheet = true
          },
          label: {
            AccentColorIconLabel(
              title: localized("settings.complex_modifications.add_rule"), systemImage: "sparkles")
          })

        Spacer()

        AppLocalizedLabel("settings.complex_modifications.for_experts", systemImage: "flame")

        Button(
          action: {
            var buffer = [Int8](repeating: 0, count: 32 * 1024)
            krbn_core_configuration_get_new_complex_modifications_rule_eval_js_string(
              &buffer, buffer.count)

            editingRule = SettingsConfiguration.ComplexModificationsRule(
              index: -1,
              description: localized("settings.complex_modifications.editor.new_script_hint"),
              descriptionNotes: [],
              enabled: true,
              codeString: String(utf8String: buffer) ?? "",
              searchText: "",
              codeType: .javascript
            )
            showingEditSheet = true
          },
          label: {
            AccentColorIconLabel(
              title: localized("settings.complex_modifications.add_script"),
              systemImage: "wand.and.rays")
          }
        )
      }
      .padding()
      .frame(maxWidth: .infinity, alignment: .leading)

      Divider()

      HStack {
        if normalizedFilterKeyword.isEmpty && rules.count > 1 {
          AppLocalizedLabel(
            "settings.general.shared.reorder_hint",
            systemImage: "arrow.up.arrow.down.square.fill"
          )
        }

        Spacer()

        SearchField(
          text: $filterKeyword, placeholderKey: "settings.complex_modifications.filter"
        )
        .frame(width: 300)
      }
      .padding(.horizontal)
      .padding(.top)
      .padding(.bottom, 4.0)

      List {
        ForEach(rules) { complexModificationRule in
          if matchesFilter(complexModificationRule) {
            // Store the ruleIndex here to prevent referencing a deleted complexModificationRule in onHover when a rule is removed.
            let ruleIndex = complexModificationRule.index

            HStack(alignment: .center, spacing: 0) {
              if normalizedFilterKeyword.isEmpty && rules.count > 1 {
                Image(systemName: "arrow.up.arrow.down.square.fill")
                  .resizable(resizingMode: .stretch)
                  .frame(width: 16.0, height: 16.0)
                  .padding(.trailing, 6.0)
                  .onHover { hovering in
                    if hovering {
                      moveDisabled = false
                    } else if NSEvent.pressedMouseButtons == 0 {
                      moveDisabled = true
                    }
                  }
                  .contextMenu {
                    Section(
                      header: AppLocalizedConstrainedText("settings.complex_modifications.position")
                    ) {
                      Button {
                        settings.moveComplexModificationsRule(complexModificationRule.index, 0)
                      } label: {
                        AppLocalizedConstrainedText("settings.complex_modifications.move_to_top")
                      }

                      Button {
                        settings.moveComplexModificationsRule(
                          complexModificationRule.index, rules.count)
                      } label: {
                        AppLocalizedConstrainedText("settings.complex_modifications.move_to_bottom")
                      }
                    }
                  }
              }

              HStack {
                VStack(alignment: .leading, spacing: 2.0) {
                  Text(complexModificationRule.description)

                  ForEach(complexModificationRule.descriptionNotes.indices, id: \.self) { index in
                    Text(complexModificationRule.descriptionNotes[index])
                      .font(.caption)
                      .foregroundColor(.secondary)
                  }
                }
                .frame(maxWidth: .infinity, alignment: .leading)
                .if(!complexModificationRule.enabled) {
                  $0.foregroundColor(.gray)
                }

                if !complexModificationRule.enabled {
                  AppLocalizedText("settings.complex_modifications.disabled")
                    .foregroundColor(.gray)
                }
              }
              .if(hoverRuleIndex == ruleIndex) {
                $0.overlay(
                  RoundedRectangle(cornerRadius: 2)
                    .inset(by: -4)
                    .stroke(
                      Color.accentColor,
                      lineWidth: 2
                    )
                )
              }

              HStack(alignment: .center, spacing: 10) {
                Toggle(
                  isOn: Binding(
                    get: { complexModificationRule.enabled },
                    set: {
                      settings.setComplexModificationsRuleEnabled(
                        index: complexModificationRule.index,
                        enabled: $0)
                    })
                ) {
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
                    AppLocalizedConstrainedLabel(
                      "shared.action.edit", systemImage: "pencil.circle.fill")
                  })

                Button(
                  role: .destructive,
                  action: {
                    settings.removeComplexModificationsRule(index: complexModificationRule.index)
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
            .moveDisabled(moveDisabled || !normalizedFilterKeyword.isEmpty)
          }
        }
        .onMove { indices, destination in
          if normalizedFilterKeyword.isEmpty, let first = indices.first {
            settings.moveComplexModificationsRule(first, destination)
          }
        }
        .listRowSeparator(.hidden)
      }
      .background(Color(NSColor.textBackgroundColor))
    }
    .sheet(isPresented: $contentViewStates.complexModificationsViewSheetPresented) {
      Group {
        if let sheetView = contentViewStates.complexModificationsViewSheetView {
          switch sheetView {
          case ComplexModificationsSheetView.assets:
            ComplexModificationsAssetsView()
          case ComplexModificationsSheetView.fileImport:
            ComplexModificationsFileImportView()
          }
        }
      }
      .modifier(SettingsLanguage())
    }
    .sheet(isPresented: $showingEditSheet) {
      ComplexModificationsEditView(
        rule: $editingRule,
        showing: $showingEditSheet,
        onEditingCancelledByExternalChange: {
          contentViewStates.showToast(
            localized("settings.complex_modifications.editor.rules_changed")
          )
        }
      )
      .modifier(SettingsLanguage())
    }
  }
}
