import SwiftUI

struct SimpleModificationsView: View {
  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      Text("Simple Modifications")

      Button(action: {
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
