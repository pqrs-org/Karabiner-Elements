import AppKit

extension NSWindow {
  public func centerToOtherWindow(_ other: NSWindow) {
    setFrame(
      NSRect(
        x: other.frame.origin.x + (other.frame.size.width / 2) - (frame.size.width / 2),
        y: other.frame.origin.y + (other.frame.size.height / 2) - (frame.size.height / 2),
        width: frame.size.width,
        height: frame.size.height
      ),
      display: false
    )
  }
}
