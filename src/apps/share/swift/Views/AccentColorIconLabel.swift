import SwiftUI

struct AccentColorIconLabel: View {
  private(set) var title: String
  private(set) var systemImage: String

  var body: some View {
    Label(
      title: {
        Text(title)
      },
      icon: {
        Image(systemName: systemImage)
          .renderingMode(.template)
          .foregroundColor(Color(NSColor.controlAccentColor))
      }
    )
  }
}
