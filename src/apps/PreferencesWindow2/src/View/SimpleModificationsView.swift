import SwiftUI

struct SimpleModificationsView: View {
  @ObservedObject private var contentViewStates = ContentViewStates.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      Text("Simple Modifications")

      Button(action: {
        contentViewStates.navigationSelection = NavigationTag.complexModifications.rawValue
      }) {
        Label("Debug", systemImage: "hammer.fill")
      }
    }
    .padding()
  }
}

struct SimpleModificationsView_Previews: PreviewProvider {
  static var previews: some View {
    SimpleModificationsView()
  }
}
