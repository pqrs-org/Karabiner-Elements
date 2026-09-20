import SwiftUI

struct SidebarLabel: View {
  let title: String
  let systemImage: String

  var body: some View {
    HStack(spacing: 8.0) {
      Image(systemName: systemImage)
        .frame(width: 18.0)

      AppLocalizedConstrainedText(title)
    }
    .padding(.vertical, 2.0)
  }
}

struct SidebarLayoutResetButton: View {
  @Binding var request: UUID

  var body: some View {
    Button {
      request = UUID()
    } label: {
      SidebarLabel(
        title: "settings.sidebar.reset_layout", systemImage: "arrow.counterclockwise"
      )
      .frame(maxWidth: .infinity, alignment: .leading)
    }
  }
}

struct SidebarStyle: ViewModifier {
  let resetRequest: UUID
  let defaultContentSize: NSSize

  private let defaultWidth: CGFloat = 250

  func body(content: Content) -> some View {
    content
      // Allow resizing for longer localized sidebar titles.
      .navigationSplitViewColumnWidth(min: 200, ideal: defaultWidth, max: 500)
      .background {
        LayoutResetAnchor(
          request: resetRequest,
          sidebarWidth: defaultWidth,
          contentSize: defaultContentSize
        )
        .frame(width: 0, height: 0)
      }
      .listStyle(.sidebar)
      .modifier(FocusOnClick())
  }
}
