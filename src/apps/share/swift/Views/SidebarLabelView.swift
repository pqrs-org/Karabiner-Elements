import SwiftUI

struct SidebarLabelView: View {
  private(set) var text: String
  private(set) var systemImage: String

  var body: some View {
    HStack {
      HStack {
        Spacer()
        Image(systemName: systemImage)
        Spacer()
      }
      .frame(width: 24.0)

      Text(text)

      Spacer()
    }
    .padding(.vertical, 6.0)
    .padding(.leading, 12.0)
    .sidebarButtonLabelStyle()
  }
}
