import SwiftUI

struct ComplexModificationsFileImportView: View {
  @ObservedObject private var settings = Settings.shared
  @State private var moveDisabled: Bool = true
  @State private var showingSheet = false

  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      Text("Hello")
    }
  }
}

struct ComplexModificationsFileImportView_Previews: PreviewProvider {
  static var previews: some View {
    ComplexModificationsFileImportView()
  }
}
