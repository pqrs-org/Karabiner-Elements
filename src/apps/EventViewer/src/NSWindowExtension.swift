import AppKit

extension NSWindow {
    public func centerToOtherWindow(_ other: NSWindow) {
        setFrame(
            NSMakeRect(
                other.frame.origin.x + (other.frame.size.width / 2) - (frame.size.width / 2),
                other.frame.origin.y + (other.frame.size.height / 2) - (frame.size.height / 2),
                frame.size.width,
                frame.size.height
            ),
            display: false
        )
    }
}
