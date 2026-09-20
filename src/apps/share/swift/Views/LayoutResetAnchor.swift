import SwiftUI

struct LayoutResetAnchor: NSViewRepresentable {
  let request: UUID
  let sidebarWidth: CGFloat
  let contentSize: NSSize

  func makeNSView(context: Context) -> AnchorView {
    AnchorView(request: request)
  }

  func updateNSView(_ view: AnchorView, context: Context) {
    guard view.request != request else { return }
    view.request = request
    // Resize outside SwiftUI's update pass to avoid recursive split-view layout updates.
    Task { @MainActor [weak view] in
      view?.resetLayout(sidebarWidth: sidebarWidth, contentSize: contentSize)
    }
  }

  final class AnchorView: NSView {
    var request: UUID

    init(request: UUID) {
      self.request = request
      super.init(frame: .zero)
    }

    required init?(coder: NSCoder) {
      fatalError("init(coder:) has not been implemented")
    }

    func resetLayout(sidebarWidth: CGFloat, contentSize: NSSize) {
      guard let window else { return }
      // The anchor lives in the sidebar. Find its enclosing split view rather than
      // searching the whole window, which could target a split view in the detail pane.
      // The ideal column width is only a preference, so reset the actual divider position.
      // Updating the divider preserves selection, focus, and the mounted detail view.
      var ancestor = superview
      while let view = ancestor {
        if let splitView = view as? NSSplitView,
          splitView.isVertical, splitView.arrangedSubviews.count >= 2
        {
          // Reset the sidebar before shrinking the window so its old width does not
          // unnecessarily constrain the detail pane. Reapply after the window relayout.
          splitView.setPosition(sidebarWidth, ofDividerAt: 0)
          window.setContentSize(contentSize)
          window.contentView?.layoutSubtreeIfNeeded()
          splitView.setPosition(sidebarWidth, ofDividerAt: 0)
          return
        }
        ancestor = view.superview
      }
    }
  }
}
