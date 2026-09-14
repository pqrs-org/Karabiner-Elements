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
                  // We have to use `Image` and `Text` in menu instead of `Label` in order to show image.
                  if e.label == label {
                    Image(systemName: "circle.circle.fill")
                  } else {
                    Image(systemName: "circle")
                  }

                  Text(verbatim: localizedLabel(e.label))
                })
            }
          }
        } label: {
          // We have to use `Image` and `Text` in menu instead of `Label` in order to show image.
          if category.include(label: label) {
            Image(systemName: "circle.circle.fill")
          } else {
            Image(systemName: "circle")
          }

          Text(verbatim: localizedLabel(category.name))
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
}
