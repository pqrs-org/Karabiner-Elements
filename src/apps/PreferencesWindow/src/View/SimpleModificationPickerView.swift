import SwiftUI

struct SimpleModificationPickerView: View {
  @ObservedObject private var settings = LibKrbn.Settings.shared

  private(set) var categories: LibKrbn.SimpleModificationDefinitionCategories
  private(set) var label: String
  private(set) var action: (_ json: String) -> Void

  var body: some View {
    Menu(label) {
      ForEach(categories.categories) { category in
        Menu {
          ForEach(category.entries) { e in
            if !e.unsafe || settings.unsafeUI {
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

                  Text(e.label)
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

          Text(category.name)
        }
      }
    }
  }
}
