import SwiftUI

struct SimpleModificationPickerView: View {
  @State var categories: SimpleModificationDefinitionCategories
  @State var entry: SimpleModificationDefinitionEntry

  var body: some View {
    Menu(entry.label) {
      ForEach($categories.categories) { $category in
        Menu(category.name) {
          ForEach($category.entries) { $e in
            Button(
              action: {
                entry = e
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
