import AppKit
import SwiftUI

// A selectable text view that keeps its selection and scroll position across content updates.
struct LiveSelectableTextView: View {
  let attributedString: NSAttributedString

  // This is primarily used to preserve selections in rolling log views that discard the oldest
  // entries as new entries are appended. It maps stable content IDs to their UTF-16 locations in
  // attributedString. When content is inserted or removed before a selection, anchors present in
  // both the old and new content are used to shift the selection so that it continues to refer to
  // the same content. If no anchors are provided, the selection keeps its absolute character
  // locations and is only clamped to the new string length.
  var selectionAnchorLocations: [String: Int] = [:]

  // When enabled, content updates keep the view at the end unless text is selected. While text is
  // selected, the current scroll position is preserved so that updates do not interrupt the
  // selection.
  var followsTail = false
  var textContainerInset = NSSize(width: 8, height: 8)
  var lineFragmentPadding: CGFloat = 0
  var isLoading = false

  init(
    attributedString: NSAttributedString,
    selectionAnchorLocations: [String: Int] = [:],
    followsTail: Bool = false,
    textContainerInset: NSSize = NSSize(width: 8, height: 8),
    lineFragmentPadding: CGFloat = 0,
    isLoading: Bool = false
  ) {
    self.attributedString = attributedString
    self.selectionAnchorLocations = selectionAnchorLocations
    self.followsTail = followsTail
    self.textContainerInset = textContainerInset
    self.lineFragmentPadding = lineFragmentPadding
    self.isLoading = isLoading
  }

  init(
    text: String,
    font: NSFont,
    isLoading: Bool = false
  ) {
    self.init(
      attributedString: NSAttributedString(
        string: text,
        attributes: [
          .font: font,
          .foregroundColor: NSColor.textColor,
        ]),
      isLoading: isLoading)
  }

  var body: some View {
    if isLoading {
      ProgressView()
        .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .center)
    } else {
      AppKitTextView(
        attributedString: attributedString,
        selectionAnchorLocations: selectionAnchorLocations,
        followsTail: followsTail,
        textContainerInset: textContainerInset,
        lineFragmentPadding: lineFragmentPadding)
    }
  }
}

private struct AppKitTextView: NSViewRepresentable {
  let attributedString: NSAttributedString
  let selectionAnchorLocations: [String: Int]
  let followsTail: Bool
  let textContainerInset: NSSize
  let lineFragmentPadding: CGFloat

  func makeCoordinator() -> Coordinator {
    Coordinator()
  }

  func makeNSView(context: Context) -> NSScrollView {
    let scrollView = NSScrollView()
    scrollView.hasVerticalScroller = true
    scrollView.autohidesScrollers = true
    scrollView.borderType = .noBorder
    scrollView.drawsBackground = true
    scrollView.backgroundColor = .textBackgroundColor
    scrollView.findBarPosition = .aboveContent

    let textView = AppKitLiveSelectableTextView(frame: .zero)
    textView.isEditable = false
    textView.isSelectable = true
    textView.isRichText = true
    textView.allowsUndo = false
    textView.usesFindPanel = false
    textView.usesFindBar = true
    textView.isIncrementalSearchingEnabled = true
    textView.drawsBackground = true
    textView.backgroundColor = .textBackgroundColor
    textView.textContainerInset = textContainerInset
    textView.textContainer?.lineFragmentPadding = lineFragmentPadding
    textView.minSize = .zero
    textView.maxSize = NSSize(
      width: CGFloat.greatestFiniteMagnitude,
      height: CGFloat.greatestFiniteMagnitude)
    textView.isVerticallyResizable = true
    textView.isHorizontallyResizable = false
    textView.autoresizingMask = [.width]
    textView.textContainer?.containerSize = NSSize(
      width: scrollView.contentSize.width,
      height: CGFloat.greatestFiniteMagnitude)
    textView.textContainer?.widthTracksTextView = true

    scrollView.documentView = textView
    context.coordinator.textView = textView
    context.coordinator.scrollView = scrollView

    return scrollView
  }

  func updateNSView(_ scrollView: NSScrollView, context: Context) {
    context.coordinator.update(
      attributedString: attributedString,
      selectionAnchorLocations: selectionAnchorLocations,
      followsTail: followsTail)
  }

  @MainActor
  final class Coordinator {
    weak var textView: NSTextView?
    weak var scrollView: NSScrollView?

    private var lastAttributedString: NSAttributedString?
    private var lastSelectionAnchorLocations: [String: Int] = [:]
    private var followsTail = false
    private var updateGeneration = 0

    func update(
      attributedString: NSAttributedString,
      selectionAnchorLocations: [String: Int],
      followsTail: Bool
    ) {
      self.followsTail = followsTail

      if let lastAttributedString,
        lastAttributedString.isEqual(to: attributedString)
      {
        return
      }

      apply(
        attributedString,
        selectionAnchorLocations: selectionAnchorLocations)
    }

    private func apply(
      _ attributedString: NSAttributedString,
      selectionAnchorLocations: [String: Int]
    ) {
      guard let textView,
        let scrollView
      else {
        return
      }

      let visibleOrigin = scrollView.contentView.bounds.origin
      let selectedRanges = adjustedSelectedRanges(
        textView.selectedRanges,
        oldAnchorLocations: lastSelectionAnchorLocations,
        newAnchorLocations: selectionAnchorLocations,
        newLength: attributedString.length)
      let hasSelection = selectedRanges.contains { $0.rangeValue.length > 0 }

      textView.textStorage?.setAttributedString(attributedString)
      textView.selectedRanges = selectedRanges

      lastAttributedString = attributedString
      lastSelectionAnchorLocations = selectionAnchorLocations
      updateGeneration += 1

      if followsTail && !hasSelection {
        scheduleLayoutAndScroll(.bottom, generation: updateGeneration)
      } else {
        scheduleLayoutAndScroll(.position(visibleOrigin), generation: updateGeneration)
      }
    }

    private func scheduleLayoutAndScroll(
      _ action: ScrollAction,
      generation: Int
    ) {
      DispatchQueue.main.async { [weak self] in
        guard let self,
          generation == updateGeneration,
          let textView,
          let scrollView
        else {
          return
        }

        scrollView.layoutSubtreeIfNeeded()

        if let textContainer = textView.textContainer,
          let layoutManager = textView.layoutManager
        {
          textView.setFrameSize(
            NSSize(
              width: scrollView.contentSize.width,
              height: textView.frame.height))
          layoutManager.ensureLayout(for: textContainer)

          let textHeight = ceil(
            layoutManager.usedRect(for: textContainer).height
              + textView.textContainerInset.height * 2)
          textView.setFrameSize(
            NSSize(
              width: scrollView.contentSize.width,
              height: max(scrollView.contentSize.height, textHeight)))
        }

        switch action {
        case .bottom:
          textView.scrollToEndOfDocument(nil)
        case .position(let origin):
          let clipView = scrollView.contentView

          // The saved origin may be outside the new document bounds if an update makes the text
          // shorter. Restoring it as-is can leave the visible area blank until the user scrolls.
          let proposedBounds = NSRect(origin: origin, size: clipView.bounds.size)
          clipView.scroll(to: clipView.constrainBoundsRect(proposedBounds).origin)
        }

        scrollView.reflectScrolledClipView(scrollView.contentView)
        textView.layoutManager?.invalidateDisplay(
          forCharacterRange: NSRange(location: 0, length: textView.string.utf16.count))
        textView.setNeedsDisplay(textView.visibleRect)
        scrollView.contentView.needsDisplay = true
        scrollView.needsDisplay = true
        textView.displayIfNeeded()
        scrollView.displayIfNeeded()
      }
    }

    private func adjustedSelectedRanges(
      _ selectedRanges: [NSValue],
      oldAnchorLocations: [String: Int],
      newAnchorLocations: [String: Int],
      newLength: Int
    ) -> [NSValue] {
      // Updates may remove text from the beginning. Find an anchor that exists in both snapshots
      // and use the change in its location to move the selection by the same amount. Finally,
      // clamp both ends because part or all of the selection may have been removed.
      let locationDelta =
        oldAnchorLocations.lazy.compactMap { id, oldLocation in
          newAnchorLocations[id].map { $0 - oldLocation }
        }.first ?? 0

      return selectedRanges.map { value in
        let range = value.rangeValue
        let location = min(max(0, range.location + locationDelta), newLength)
        let end = min(max(0, range.location + range.length + locationDelta), newLength)

        return NSValue(
          range: NSRange(
            location: location,
            length: max(0, end - location)))
      }
    }

    private enum ScrollAction {
      case bottom
      case position(NSPoint)
    }
  }
}

private final class AppKitLiveSelectableTextView: NSTextView {
  // Opening the find bar requires an NSTextView that is attached to a window. Use the AppKit
  // lifecycle instead of SwiftUI's onAppear so that the window is available without bridging the
  // NSTextView back into SwiftUI.
  override func viewDidMoveToWindow() {
    super.viewDidMoveToWindow()

    guard window != nil else {
      return
    }

    DispatchQueue.main.async { [weak self] in
      guard let self, self.window != nil else {
        return
      }

      // The find bar is opened automatically rather than by an explicit user action, so it should
      // not inherit the previous query from the shared Find pasteboard. Clear it before showing the
      // bar so that the search field starts empty.
      let findPasteboard = NSPasteboard(name: .find)
      findPasteboard.clearContents()
      findPasteboard.setString("", forType: .string)

      let menuItem = NSMenuItem()
      menuItem.tag = Int(NSFindPanelAction.showFindPanel.rawValue)
      self.performFindPanelAction(menuItem)
    }
  }

  override func keyDown(with event: NSEvent) {
    let modifiers = event.modifierFlags
      .intersection(.deviceIndependentFlagsMask)
      .subtracting([.capsLock, .function, .numericPad])

    // A non-editable NSTextView still handles arrow keys as caret movement, which can fight with
    // the scroll-position restoration performed during content updates and result in jerky
    // scrolling. Treat unmodified vertical arrow keys as scrolling instead. Modified arrow keys
    // are passed through so that selection operations such as Shift+Arrow continue to work.
    if modifiers.isEmpty,
      let scrollView = enclosingScrollView,
      let specialKey = event.specialKey
    {
      switch specialKey {
      case .downArrow:
        scrollVertically(by: scrollView.verticalLineScroll)
        return

      case .upArrow:
        scrollVertically(by: -scrollView.verticalLineScroll)
        return

      default:
        break
      }
    }

    super.keyDown(with: event)
  }

  private func scrollVertically(by offset: CGFloat) {
    guard let scrollView = enclosingScrollView else { return }

    let clipView = scrollView.contentView
    var proposedBounds = clipView.bounds
    proposedBounds.origin.y += offset
    clipView.scroll(to: clipView.constrainBoundsRect(proposedBounds).origin)
    scrollView.reflectScrolledClipView(clipView)
  }
}
