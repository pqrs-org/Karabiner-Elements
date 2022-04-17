import SwiftUI

struct SimpleModificationPickerView: View {
  @State var categories: [SimpleModificationDefinitionCategory]
  @State var entry: SimpleModificationDefinitionEntry

  var body: some View {
      Menu(entry.label) {
      ForEach($categories) { $category in
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
