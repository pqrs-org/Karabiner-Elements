import SwiftUI

struct SimpleModificationPickerView: View {
  var categories: SimpleModificationDefinitionCategories
  var label: String
  var action: (_ json: String) -> Void

  var body: some View {
    Menu(label) {
      ForEach(categories.categories) { category in
        Menu(category.name) {
          ForEach(category.entries) { e in
            Button(
              action: {
                action(e.json)
              },
              label: {
                Text(e.label)
              })
          }
        }
      }
    }
  }
}
