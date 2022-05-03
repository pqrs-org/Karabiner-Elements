import SwiftUI

struct SimpleModificationPickerView: View {
    var categories: LibKrbn.SimpleModificationDefinitionCategories
  var label: String
  var action: (_ json: String) -> Void

  var body: some View {
    Menu(label) {
      ForEach(categories.categories) { category in
        Menu {
          ForEach(category.entries) { e in
            Button(
              action: {
                action(e.json)
              },
              label: {
                // We have to use `Image` and `Text` in menu instead of `Label` in order to show image.
                if e.label == label {
                  Image(systemName: "checkmark.square.fill")
                } else {
                  Image(systemName: "square")
                }

                Text(e.label)
              })
          }
        } label: {
          // We have to use `Image` and `Text` in menu instead of `Label` in order to show image.
          if category.include(label: label) {
            Image(systemName: "checkmark.square.fill")
          } else {
            Image(systemName: "square")
          }

          Text(category.name)
        }
      }
    }
  }
}
