import SwiftUI

struct SimpleModificationPickerView: View {
  @AppLocalizationContext private var localized
  private(set) var categories: SimpleModificationDefinitionCategories
  private(set) var label: String
  private(set) var action: (_ json: String) -> Void
  private(set) var showUnsafe: Bool

  var body: some View {
    Menu(localizedLabel(label)) {
      ForEach(categories.categories) { category in
        Menu {
          ForEach(category.entries) { e in
            if !e.unsafe || showUnsafe {
              Button(
                action: {
                  action(e.json)
                },
                label: {
                  Text(verbatim: selectionLabel(e.label, selected: e.label == label))
                })
            }
          }
        } label: {
          Text(verbatim: selectionLabel(category.name, selected: category.include(label: label)))
        }
      }
    }
    .frame(maxWidth: .infinity, alignment: .leading)
  }

  // Definitions contain either an explicit localization key or a literal key name.
  // Unknown strings (including custom JSON) are displayed verbatim by the lookup.
  private func localizedLabel(_ label: String) -> String {
    localized(label)
  }

  // On macOS 27, icons provided by Label or Image are not displayed in these menus.
  // Use monochrome Unicode symbols in the label text to keep selection markers visible.
  private func selectionLabel(_ label: String, selected: Bool) -> String {
    (selected ? "◉ " : "○ ") + localizedLabel(label)
  }
}
