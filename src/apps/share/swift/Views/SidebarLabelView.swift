import SwiftUI

struct SidebarLabelView: View {
  private(set) var text: String
  private(set) var systemImage: String
  private(set) var padding = 6.0

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
    .padding(.vertical, padding)
    .padding(.leading, 12.0)
    .sidebarButtonLabelStyle()
  }
}
